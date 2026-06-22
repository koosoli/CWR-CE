
#include <Poseidon/Core/Application.hpp>
#include <Poseidon/World/Terrain/Landscape.hpp>
#include <Poseidon/World/Terrain/Roads.hpp>
#include <float.h>
#include <Poseidon/Foundation/Common/FltOpts.hpp>
#include <Poseidon/Foundation/Containers/Array2D.hpp>
#include <Poseidon/Foundation/Containers/StreamArray.hpp>
#include <Poseidon/Foundation/Framework/Log.hpp>
#include <Poseidon/Foundation/Math/MathOpt.hpp>
#include <Poseidon/Foundation/Memory/MemAlloc.hpp>

namespace Poseidon
{
using namespace Foundation;

DEFINE_FAST_ALLOCATOR(RoadLink)

DEFINE_FAST_ALLOCATOR(RoadListFull)

RoadLink::RoadLink(Object* object, Vector3* pos, int c) : _object(object), _locks(0), _nCon(c)
{
    PoseidonAssert(c <= NCon);
    for (int i = 0; i < c; i++)
    {
        _pos[i] = pos[i];
    }
    for (int i = 0; i < NCon; i++)
    {
        _con[i] = nullptr;
    }
}

bool RoadLink::IsInside(Vector3Val pos, float size) const
{
    Object* obj = _object;
    if (!obj)
    {
        return false;
    }
    float minDist2 = 1e10;
    LODShape* lShape = obj->GetShape();
    Shape* shape = lShape->RoadwayLevel();
    if (!shape)
    {
        RptF("No roadway in road %s", obj->GetShape()->Name());
        return false;
    }
    float centerDist2 = (pos - obj->Position()).SquareSizeXZ();
    if (centerDist2 > Square(lShape->BoundingSphere() + size))
    {
        return false;
    }
    Matrix4 invTrans = obj->GetInvTransform();
    Vector3 rPos = invTrans.FastTransform(pos);
    for (Offset f = shape->BeginFaces(); f < shape->EndFaces(); shape->NextFace(f))
    {
        const Poly& face = shape->Face(f);
        float dist2 = face.DistanceFromTop(*shape, rPos);
        saturateMin(minDist2, dist2);
    }
    return minDist2 < Square(size);
}

inline RoadList& RoadNet::SelectRoadList(float x, float z)
{
    int xx = toIntFloor(x * InvLandGrid);
    int zz = toIntFloor(z * InvLandGrid);
    if (!InRange(xx, zz))
    {
        // find nearest in-range square and use it
        if (xx < 0)
        {
            xx = 0;
        }
        else if (xx > LandRangeMask)
        {
            xx = LandRangeMask;
        }
        if (zz < 0)
        {
            zz = 0;
        }
        else if (zz > LandRangeMask)
        {
            zz = LandRangeMask;
        }
        PoseidonAssert(InRange(xx, zz));
    }
    return _roads(xx, zz);
}

// build road net information

RoadNet::RoadNet() = default;
RoadNet::~RoadNet() = default;

void RoadNet::Scan(Landscape* land)
{
    _roads.Dim(LandRange, LandRange);
    // insert all road objects into the road list
    int x, z;
    for (x = 0; x < LandRange; x++)
    {
        for (z = 0; z < LandRange; z++)
        {
            const ObjectList& list = land->GetObjects(z, x);
            for (int o = 0; o < list.Size(); o++)
            {
                Object* obj = list[o];
                if (obj->GetType() != Network)
                {
                    continue;
                }
                // consider only networks
                // scan cross point name pairs
                const int MaxPos = 4;
                static const char* pairs[MaxPos * 2] = {
                    "LB", "PB", "LE", "PE", // normal
                    "LD", "LH", "PD", "PH", // crossing
                };
                Shape* shape = obj->GetShape()->MemoryLevel();
                Vector3 pos[MaxPos];
                Vector3 avg = obj->Position();
                // float avgDistLR=0;
                int c = 0;
                if (!shape)
                {
                    // synthetise LB, PB, LE, PE points from nothing if possible
                    // use geometry min-max
                    Shape* geom = obj->GetShape()->GeometryLevel();
                    if (!geom)
                    {
                        continue;
                    }
                    LOG_DEBUG(World, "Road shape {} missing connection points", obj->GetShape()->Name());
                    Vector3Val min = geom->Min();
                    Vector3Val max = geom->Max();
                    Vector3Val cnt = (min + max) * 0.5;

                    Vector3 sposB = Vector3(cnt.X(), max.Y(), min.Z());
                    Vector3 sposE = Vector3(cnt.X(), max.Y(), max.Z());
                    obj->PositionModelToWorld(sposB, sposB);
                    obj->PositionModelToWorld(sposE, sposE);
                    pos[0] = sposB;
                    pos[1] = sposE;
                    c = 2;
                    // Vector3Val rPos=shape->NamedPosition(r);
                }
                else
                {
                    for (c = 0; c < sizeof(pairs) / sizeof(*pairs) / 2; c++)
                    {
                        const char *l = pairs[c * 2], *r = pairs[c * 2 + 1];
                        if (shape->FindNamedSel(l) < 0)
                        {
                            break;
                        }
                        if (shape->FindNamedSel(r) < 0)
                        {
                            break;
                        }
                        Vector3Val lPos = shape->NamedPosition(l);
                        Vector3Val rPos = shape->NamedPosition(r);
                        // float distLR=lPos.Distance(rPos);
                        // avgDistLR+=distLR;
                        Vector3 spos = (lPos + rPos) * 0.5;
                        obj->PositionModelToWorld(spos, spos);
                        pos[c] = spos;
                    }
                    // avgDistLR/=c;
                }
                RoadLink* elem = nullptr;
                if (c <= 4)
                {
                    elem = new RoadLink(obj, pos, c);
                }
                if (!elem)
                {
                    Fail("Bad road element.");
                    continue;
                }

                // insert into corresponding square
                RoadList& list = SelectRoadList(avg.X(), avg.Z());
                list.Add(elem);
            }
        }
    }
}

Vector3 RoadLink::GetCenter() const
{
    Vector3 center = VZero;
    for (int i = 0; i < _nCon; i++)
    {
        center += _pos[i];
    }
    center *= 1.0 / _nCon;

    return center;
}

float RoadLink::NearestConnectionDist2(Vector3Val pos) const
{
    float dist2Min = 1e10;
    for (int i = 0; i < _nCon; i++)
    {
        float dist2 = (_pos[i] - pos).SquareSizeXZ();
        if (dist2 < dist2Min)
        {
            dist2Min = dist2;
        }
    }
    return dist2Min;
}

// copy and add connection

void RoadLink::AddConnection(Vector3Par pos, RoadLink* con)
{
    if (_nCon >= NCon)
    {
        Fail("No more slot to add connection");
        return;
    }
    _con[_nCon] = con;
    _pos[_nCon] = pos;
    _nCon++;
}

static bool IsConnected(const RoadLink* tItem, const RoadLink* item, int maxDepth)
{
    --maxDepth;
    for (int jj = 0; jj < tItem->NConnections(); jj++)
    {
        const RoadLink* cItem = tItem->Connections()[jj];
        if (cItem == item)
        {
            return true;
        }
        if (maxDepth > 0 && cItem)
        {
            if (IsConnected(cItem, item, maxDepth))
            {
                return true;
            }
        }
    }
    return false;
}

#define DIAGS 0
#define DIAG_COUNT 0
void RoadNet::Connect()
{
#if DIAG_COUNT
    int count[4];
    memset(count, 0, sizeof(count));
#endif
    // connect as necessary
    const float maxDistCon = 1.0;
    for (int x = 0; x < LandRange; x++)
    {
        for (int z = 0; z < LandRange; z++)
        {
            RoadList& list = _roads(x, z);
            for (int o = 0; o < list.Size(); o++)
            {
                RoadLink* road = list[o];
                int nCon = road->NConnections();
#if DIAG_COUNT
                count[nCon - 1]++;
#endif
                const Vector3* pos = road->PosConnections();
                RoadLink* const* conn = road->Connections();
                // if there is already some connection, skip it
                for (int i = 0; i < nCon; i++)
                {
                    if (!conn[i])
                    {
                        Vector3Val val = pos[i];
                        // search all other roads for same position
                        // there should be max. one present
                        for (int xx = x - 1; xx <= x + 1; xx++)
                        {
                            for (int zz = z - 1; zz <= z + 1; zz++)
                            {
                                if (InRange(xx, zz))
                                {
                                    RoadList& search = _roads(xx, zz);
                                    for (int si = 0; si < search.Size(); si++)
                                    {
                                        RoadLink* sRoad = search[si];
                                        if (sRoad == road)
                                        {
                                            continue;
                                        }
                                        int sCon = sRoad->NConnections();
                                        const Vector3* sPos = sRoad->PosConnections();
                                        for (int ii = 0; ii < sCon; ii++)
                                        {
                                            float dist2 = (sPos[ii] - val).SquareSizeXZ();
                                            if (dist2 < Square(maxDistCon))
                                            {
#if DIAGS
                                                LOG_DEBUG(World, "Connect {:x}:{} - {:x}:{}", (uintptr_t)road, i,
                                                          (uintptr_t)sRoad, ii);
#endif
                                                // connection found - mark it

                                                // connect both together
                                                road->SetConnection(i, sRoad);
                                                sRoad->SetConnection(ii, road);
                                                goto ConnFound;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    ConnFound:;
                    }
                }
            }
        }
    }
#if DIAG_COUNT
    {
        // some diagnostic information
        for (int i = 0; i < 4; i++)
        {
            LOG_DEBUG(World, "Road {} dirs: {}", i + 1, count[i]);
        }
    }
#endif
    // second pass: try to connect road unconnected points to something

    const float maxDistConPass2 = 7.0;
    for (int x = 0; x < LandRange; x++)
    {
        for (int z = 0; z < LandRange; z++)
        {
            RoadList& list = _roads(x, z);

            int n = list.Size();
            for (int i = 0; i < n; i++)
            {
                RoadLink* item = list[i];
                for (int j = 0; j < item->NConnections(); j++)
                {
                    if (!item->Connections()[j])
                    {
                        // something not connected
                        // try to find some connection point
                        // scan all near road points (+-1)
                        Vector3 pos = item->PosConnections()[j];
                        for (int xx = x - 1; xx <= x + 1; xx++)
                        {
                            for (int zz = z - 1; zz <= z + 1; zz++)
                            {
                                if (!InRange(xx, zz))
                                {
                                    continue;
                                }
                                //
                                const RoadList& tList = _roads(xx, zz);
                                int nn = tList.Size();
                                for (int ii = 0; ii < nn; ii++)
                                {
                                    // check connection with all connection points of tItem
                                    RoadLink* tItem = tList[ii];
                                    if (tItem == item)
                                    {
                                        continue;
                                    }
                                    // skip item
                                    // skip items that are directly connected
                                    if (IsConnected(tItem, item, 1))
                                    {
#if DIAGS
                                        char buf[256];
                                        PositionToAA11(pos, buf);
                                        LOG_DEBUG(World, "{}: Already Connected {} to {}", buf, item->GetObject()->ID(),
                                                  tItem->GetObject()->ID());
#endif
                                        continue;
                                    }

                                    float nearestDist2 = 1e10;
                                    int nearestJJ = -1;
                                    Vector3 nearestPos;
                                    for (int jj = 0; jj < tItem->NConnections(); jj++)
                                    {
                                        // check distance to item point
                                        Vector3Val tPos = tItem->PosConnections()[jj];
                                        float dist2 = pos.Distance2Inline(tPos);
                                        if (dist2 < nearestDist2)
                                        {
                                            nearestDist2 = dist2;
                                            nearestJJ = jj;
                                            nearestPos = tPos;
                                        }
                                    } // for (jj)
                                    if (nearestDist2 < Square(maxDistConPass2))
                                    {
// connect with nearestJJ
#if DIAGS
                                        char buf[256];
                                        PositionToAA11(pos, buf);
                                        LOG_DEBUG(World, "{}: Connect {} with {} dist {:.2f}", buf,
                                                  item->GetObject()->ID(), tItem->GetObject()->ID(),
                                                  sqrt(nearestDist2));
#endif
                                        if (!tItem->Connections()[nearestJJ])
                                        {
                                            // connection is empty
                                            item->SetConnection(j, tItem);
                                            tItem->SetConnection(nearestJJ, item);
                                        }
                                        else
                                        {
                                            // connection not empty - we need to add another one
                                            // LOG_DEBUG(World, "  not empty");
                                            item->SetConnection(j, tItem);
                                            tItem->AddConnection(nearestPos, item);
                                            // create back connection
                                        }
                                        goto Connected;
                                    }
                                    else
                                    {
#if DIAGS
                                        if (nearestDist2 < Square(20))
                                        {
                                            char buf[256];
                                            PositionToAA11(pos, buf);
                                            LOG_DEBUG(World, "{}: No Connect {} to {} dist {:.2f}", buf,
                                                      item->GetObject()->ID(), tItem->GetObject()->ID(),
                                                      sqrt(nearestDist2));
                                        }
#endif
                                    }
                                } // for (ii)
                            } // for(xx,zz)
                        }
                    Connected:;
                    }
                } // for (j)
            } // for(i)
        } // for(x,z)
    }
}

void RoadNet::Optimize()
{
    // merge small straight elements
}
void RoadNet::Compact()
{
    // optimize memory image
}

struct RoadNode
{
    RoadNode* parent;
    RoadLink* road;
    Vector3 pos;
    float cost;
    float heur;
    bool open;

    USE_FAST_ALLOCATOR
};

DEFINE_FAST_ALLOCATOR(RoadNode);

bool operator<(RoadNode& a, RoadNode& b)
{
    return a.cost + a.heur < b.cost + b.heur;
}

bool operator<=(RoadNode& a, RoadNode& b)
{
    return a.cost + a.heur <= b.cost + b.heur;
}

typedef HeapArray<RoadNode*, MemAllocSS> RoadOpenList;

class RoadNodeContainer : public AutoArray<SRef<RoadNode>>
{
  public:
    RoadNode* Find(RoadLink* link) const;
};

RoadNode* RoadNodeContainer::Find(RoadLink* link) const
{
    int i, n = Size();
    for (i = 0; i < n; i++)
    {
        RoadNode* node = Get(i);
        if (node->road == link)
        {
            return node;
        }
    }
    return nullptr;
}

inline float RoadCost(Vector3Par from, Vector3Par to)
{
    return (to - from).Size();
}

inline float RoadHeur(Vector3Par from, Vector3Par to)
{
    return (to - from).Size() * 0.5;
}
#define MAX_ITER 100
#define COST_NO_ROAD 10.0f

#undef DIAGS
#define DIAGS 0

bool RoadNet::SearchPath(Vector3Par from, Vector3Par to, RoadPathArray& path, float prec) const
{
    // maintain "open" list
    // (close list?)

    // search from...to given coordinates
    // consider - tolerance for from and to
    //	const float TO_TOLERANCE = 15.0;

    RoadNodeContainer container;
    RoadOpenList openList;

    static StaticStorage<RoadNode*> openListStorage;
    // use static storage to contain typical searches
    openList.SetStorage(openListStorage.Init(256));

    RoadNode *best, *cur;

#if DIAGS
    LOG_DEBUG(World, "Searching road path from {:.0f}, {:.0f} to {:.0f}, {:.0f}, precision {:.1f}", from.X(), from.Z(),
              to.X(), to.Z(), prec);
    LOG_DEBUG(World, " from on road: {}", IsOnRoad(from, prec) ? "Yes" : "No");
    LOG_DEBUG(World, " to   on road: {}", IsOnRoad(to, prec) ? "Yes" : "No");
#endif

    // 1. find nodes with distance from to < TO_TOLERANCE and place it to open list
    int xi, xx = toIntFloor(to.X() * InvLandGrid);
    int zi, zz = toIntFloor(to.Z() * InvLandGrid);
#if DIAGS
    int sumItems = 0;
#endif
    for (zi = zz - 1; zi <= zz + 1; zi++)
    {
        for (xi = xx - 1; xi <= xx + 1; xi++)
        {
            if (!InRange(xi, zi))
            {
                continue;
            }
            const RoadList& roadList = _roads(xi, zi);
#if DIAGS
            sumItems += roadList.Size();
#endif
            for (int i = 0; i < roadList.Size(); i++)
            {
                RoadLink* item = roadList[i];
                if (item->IsLocked())
                {
#if DIAGS
                    Object* obj = item->GetObject();
                    LOG_DEBUG(World, "Locked item {} ({:.1f},{:.1f})", (const char*)obj->GetDebugName(),
                              obj->Position().X(), obj->Position().Z());
#endif
                    continue;
                }
                if (item->IsInside(to, prec))
                {
                    best = new RoadNode;
                    container.Add(best);
                    best->parent = nullptr;
                    best->road = item;
                    best->pos = to;                                             // item->PosConnections()[jBest];
                    best->cost = item->GetCenter().Distance(to) * COST_NO_ROAD; // * sqrt(minDist2);
                    best->heur = RoadHeur(best->pos, from);
                    best->open = true;
                    openList.HeapInsert(best);
#if DIAGS >= 2
                    LOG_DEBUG(World, "Item {:.1f},{:.1f} open  from {}, to {}", item->GetCenter().X(),
                              item->GetCenter().Z(), item->IsInside(from, prec) ? "in " : "out",
                              item->IsInside(to, prec) ? "in " : "out");
#endif
                }
            }
        }
    }
#if DIAGS
    LOG_DEBUG(World, "  {} items placed at open list ({} candidates)", openList.Size(), sumItems);
#endif

    // 2. perform A*
    int iter = 0;
    while (true) // search cycle
    {
        bool ok = openList.HeapRemoveFirst(best);
        if (!ok || ++iter > MAX_ITER)
        {
            goto PathNotFound;
        }

        if (best->road->IsInside(from, 0))
        {
            // best is the first node of result path
            goto PathFound;
        }

        best->open = false;

        int i, n = best->road->NConnections();
        for (i = 0; i < n; i++) // generate successors
        {
            RoadLink* item = best->road->Connections()[i];
            if (!item)
            {
                continue;
            }
            if (item->IsLocked())
            {
#if DIAGS
                Object* obj = item->GetObject();
                LOG_DEBUG(World, "Locked item {} ({:.1f},{:.1f})", (const char*)obj->GetDebugName(),
                          obj->Position().X(), obj->Position().Z());
#endif
                continue;
            }

            Vector3Val pos = best->road->PosConnections()[i];
            float cost = best->cost + RoadCost(best->pos, pos);

            cur = container.Find(item);
            if (cur)
            {
                //			heuristic doesn't change
                if (cur->open && (cost < cur->cost))
                {
                    cur->parent = best;
                    cur->cost = cost;
                    openList.HeapUpdateUp(cur);
                }
                else
                {
                    // there is better path into item
                    continue;
                }
            }
            else
            {
                cur = new RoadNode;
                container.Add(cur);
                cur->parent = best;
                cur->road = item;
                cur->pos = pos;
                cur->cost = cost;
                cur->heur = RoadHeur(pos, from);
                cur->open = true;
                openList.HeapInsert(cur);
#if DIAGS >= 2
                LOG_DEBUG(World, "Item {:.1f},{:.1f} open  from {}, to {}", item->GetCenter().X(),
                          item->GetCenter().Z(), item->IsInside(from, prec) ? "in " : "out",
                          item->IsInside(to, prec) ? "in " : "out");
#endif
            }
        } // end of generate successors
    } // end of search cycle

// 3. build OperInfoResult list
PathNotFound:
    best = nullptr;
    {
        float minHeur = FLT_MAX;
        for (int i = 0; i < container.Size(); i++)
        {
            cur = container[i];
            if (cur->road->IsInside(from, prec))
            {
                if (cur->heur < minHeur)
                {
                    minHeur = cur->heur;
                    best = cur;
                }
            }
        }
    }

    if (best)
    {
#if DIAGS
        LOG_DEBUG(World, "  alternate start found");
#endif
        goto PathFound;
    }

#if DIAGS
    LOG_DEBUG(World, "  path not found at {} iters", iter);
#endif
    return false;

PathFound:
    // best is the first node of result path
#if DIAGS
    LOG_DEBUG(World, "  path found at {} iters:", iter);
#endif
    path.Clear();
    // check if from should be added to path
    RoadNode* next = best->parent;
    if (!next)
    {
        path.Add(from);
    }
    else
    {
        // check angle with first section of road (from..best..next)
        Vector3 nPos = next->pos;
        Vector3 bPos = best->pos;
        Vector3 nbPos = nPos - bPos;
        float cosAlpha = nbPos * (from - bPos);
        if (cosAlpha < 0)
        {
            path.Add(from);
        }
    }
    cur = best;
    while (cur)
    {
        path.Add(cur->pos);
#if DIAGS
        LOG_DEBUG(World, "  - {:.0f}, {:.0f}", cur->pos.X(), cur->pos.Z());
#endif
        cur = cur->parent;
    }
    return true;
}

const RoadLink* RoadNet::IsOnRoad(Vector3Par pos, float size) const
{
    int xx = toIntFloor(pos.X() * InvLandGrid);
    int zz = toIntFloor(pos.Z() * InvLandGrid);
    for (int zi = zz - 1; zi <= zz + 1; zi++)
    {
        for (int xi = xx - 1; xi <= xx + 1; xi++)
        {
            if (!InRange(xi, zi))
            {
                continue;
            }
            const RoadList& roadList = _roads(xi, zi);
            for (int i = 0; i < roadList.Size(); i++)
            {
                const RoadLink* roadLink = roadList[i];
                if (roadLink->IsInside(pos, size))
                {
                    return roadLink;
                }
            }
        }
    }
    return nullptr;
}

Vector3 RoadNet::GetNearestRoadPoint(Vector3Par pos) const
{
    // scan all roads near pos
    // check 1-1 connections
    float maxDist2 = Square(20);
    Vector3 best = pos;
    int xx = toIntFloor(pos.X() * InvLandGrid);
    int zz = toIntFloor(pos.Z() * InvLandGrid);
    for (int zi = zz - 1; zi <= zz + 1; zi++)
    {
        for (int xi = xx - 1; xi <= xx + 1; xi++)
        {
            if (!InRange(xi, zi))
            {
                continue;
            }
            const RoadList& roadList = _roads(xi, zi);
            for (int i = 0; i < roadList.Size(); i++)
            {
                const RoadLink* roadLink = roadList[i];
                // check distance from main connection (0..1)
                int nConn = roadLink->NConnections();
                const Vector3* pConn = roadLink->PosConnections();
                if (nConn < 2)
                {
                    continue;
                }
                Vector3Val beg = pConn[0];
                Vector3Val e = pConn[1] - beg;
                Vector3Val p = pos - beg;

                float tPrev = (e * p) / e.SquareSize();
                saturate(tPrev, 0, 1);
                Vector3 nearest = beg + tPrev * e;
                float dist2 = nearest.Distance2Inline(pos);
                if (dist2 < maxDist2)
                {
                    maxDist2 = dist2;
                    best = nearest;
                }
            }
        }
    }
    return best;
}

bool RoadNet::IsLocked(Vector3Par pos, float size) const
{
    int xx = toIntFloor(pos.X() * InvLandGrid);
    int zz = toIntFloor(pos.Z() * InvLandGrid);
    for (int zi = zz - 1; zi <= zz + 1; zi++)
    {
        for (int xi = xx - 1; xi <= xx + 1; xi++)
        {
            if (!InRange(xi, zi))
            {
                continue;
            }
            const RoadList& roadList = _roads(xi, zi);

            for (int i = 0; i < roadList.Size(); i++)
            {
                const RoadLink* roadLink = roadList[i];
                if (!roadLink->IsLocked())
                {
                    continue;
                }
                if (roadLink->NearestConnectionDist2(pos) < Square(size))
                {
                    return true;
                }
            }
        }
    }
    return false;
}

void RoadNet::Build(Landscape* land)
{
    Scan(land);
    Connect();
    Optimize();
    Compact();
}

SRef<RoadNet> GRoadNet; // global instance

} // namespace Poseidon
