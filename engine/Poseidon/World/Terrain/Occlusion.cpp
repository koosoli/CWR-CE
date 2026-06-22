#include <Poseidon/World/Terrain/Occlusion.hpp>

#include <Poseidon/Dev/Debug/DebugWin.hpp>
#include <string.h>
#include <cmath>
#include <utility>
#include <Poseidon/Foundation/Common/FltOpts.hpp>
#include <Poseidon/Foundation/Containers/StaticArray.hpp>
#include <Poseidon/Foundation/Containers/StreamArray.hpp>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Poseidon/Foundation/Math/Fixed.hpp>
#include <Poseidon/Foundation/Memory/MemAlloc.hpp>

using namespace Poseidon;
namespace Poseidon
{
using namespace Poseidon::Dev;
using Poseidon::Foundation::fixed;
using Poseidon::Foundation::Fixed0;
using Poseidon::Foundation::MStorage;
} // namespace Poseidon
#include <Poseidon/Input/InputSubsystem.hpp>
#include <Poseidon/World/Scene/Scene.hpp>
#include <Poseidon/World/Scene/Camera/Camera.hpp>
#include <SDL3/SDL_scancode.h>

#include <Poseidon/Graphics/Rendering/Primitives/Edges.hpp>
#pragma optimize("at", on) // optimize for time, assume no aliasing

#define DETAIL_COUNTERS 0

#pragma warning(default : 4244; default : 4305) // warn if __int64 to int conversion takes place

void OcclusionPoly::Copy(const OcclusionPoly& src)
{
    memcpy(this, &src, sizeof(_n) + sizeof(*_v) * src._n);
}

// some operations require that inside test is performed, not pre-calculated
bool OcclusionPoly::Clip(OcclusionPoly& res, Vector3Par normal, Coord d) const
{
    // initialize resulting polygon
    res._n = 0;
    if (_n < 3)
    {
        return true;
    }

    // search for first vertex inside clipping half-space
    int i;
    const Vector3* pVertex; // previous vertex
    const Vector3* aVertex; // actual vertex

    pVertex = _v + _n - 1;
    bool pOut = normal * (*pVertex) + d < 0;

    bool change = false;

    for (i = 0; i < _n; i++)
    {
        aVertex = _v + i;
        bool aOut = normal * (*aVertex) + d < 0;
        // four possible situations
        if (aOut != pOut)
        {
            Vector3Val in = *aVertex;
            Vector3Val out = *pVertex;

            float t = (in * normal + d) / (normal * (in - out));

            res.Add(in + (out - in) * t);
            change = true;
        }
        if (!aOut)
        {
            // point in
            res.Add(*aVertex);
        }
        else
        {
            change = true;
        }
        pVertex = aVertex;
        pOut = aOut;
    }

    PoseidonAssert(res._n <= MaxPoly);
    if (res._n < 3)
    {
        res._n = 0; // polygon completely out
    }
    return change;
}

void OcclusionPoly::Clip(float cNear, float cFar, ClipFlags clip)
{
    // clip to all clip planes
    OcclusionPoly tempResult;
    OcclusionPoly* source = this;
    OcclusionPoly* result = &tempResult;
    if (clip & ClipFront)
    {
        if (source->Clip(*result, Vector3(0, 0, +1), -cNear))
        {
            swap(source, result);
        }
    }
    if (clip & ClipBack)
    {
        if (source->Clip(*result, Vector3(0, 0, -1), cFar))
        {
            swap(source, result);
        }
    }

    if (clip & ClipLeft)
    {
        if (source->Clip(*result, Vector3(+1, 0, +1), 0))
        {
            swap(source, result);
        }
    }
    if (clip & ClipRight)
    {
        if (source->Clip(*result, Vector3(-1, 0, +1), 0))
        {
            swap(source, result);
        }
    }
    if (clip & ClipTop)
    {
        if (source->Clip(*result, Vector3(0, +1, +1), 0))
        {
            swap(source, result);
        }
    }
    if (clip & ClipBottom)
    {
        if (source->Clip(*result, Vector3(0, -1, +1), 0))
        {
            swap(source, result);
        }
    }

    if (source != this)
    {
        *this = *source;
    }
}

void OcclusionPoly::Perspective()
{
    // float nearZ = GScene->GetCamera()->ClipNear();
    for (int i = 0; i < _n; i++)
    {
        Vector3& d = _v[i];
        float invZ = 1 / d.Z();
        d[0] *= invZ;
        d[1] *= invZ;
        // d[2]=invZ;
    }
}

Occlusion::Occlusion(int w, int h)
{
    _w = w;
    _h = h;
    _w2 = float(w / 2);
    _h2 = float(h / 2);
    _data.Realloc(w * h);
}

Occlusion::~Occlusion()
{
    _data.Free();
}

inline Vector3 MinMaxCorner(const Vector3* minMax, int x, int y, int z)
{
    return Vector3(minMax[x][0], minMax[y][1], minMax[z][2]);
}

void OcclusionPoly::Transform(const Matrix4& trans)
{
    // transform all vertices
    for (int i = 0; i < _n; i++)
    {
        _v[i] = trans.FastTransform(_v[i]);
    }
}

const float ro = 0.79370052598f; // 2 ^ (-1/3) = 4^(1/3)/2 = 0.5^(1/3)

static void SumXYVolume(float& sumVolume, Vector3& sumCov, const Vector3* a, const Vector3* b, const Vector3* c)
{
    // sort vertices so that a.Z()<=b.Z()<=c.Z()
    float sign = +1;
    if (a->Z() > b->Z())
    {
        swap(a, b), sign = -sign;
    }
    if (b->Z() > c->Z())
    {
        swap(b, c), sign = -sign;
        if (a->Z() > b->Z())
        {
            swap(a, b), sign = -sign;
        }
    }
    // sum volume and center of volume*volume
    // calculate space between tri and plane (xy)
    // calculate xy area of triangle abc
    Vector3 ba(b->X() - a->X(), b->Y() - a->Y(), 0);
    Vector3 ca(c->X() - a->X(), c->Y() - a->Y(), 0);
    // cross product of ba and ca has only z non-zero
    // float xyArea = ba.CrossProduct(ca).Z();
    float xyArea = (ba.X() * ca.Y() - ba.Y() * ca.X()) * sign;

    // depending on area sign volume will be added or subtracted
    // volume is:

    // float volume = (a->Z()+b->Z()+c->Z())*(1.0f/6)*xyArea;

    //  cov calculation
    // note: might be optimized later
    // A segment
    Vector3 tA = ((Vector3(a->X(), a->Y(), 0) + Vector3(b->X(), b->Y(), 0) + Vector3(c->X(), c->Y(), 0)) * (1.0f / 3) +
                  Vector3(0, 0, a->Z() * 0.5f));
    float vA = xyArea * 0.5f * a->Z();
    // B segment
    Vector3 tB = ((1 - ro) * (*a) + ro * (1.0f / 4) *
                                        (Vector3(b->X(), b->Y(), b->Z()) + Vector3(c->X(), c->Y(), b->Z()) +
                                         Vector3(b->X(), b->Y(), a->Z()) + Vector3(c->X(), c->Y(), a->Z())));
    float vB = xyArea * (1.0f / 3) * (b->Z() - a->Z());
    // C segment
    Vector3 tC = ((1 - ro) * (*a) + ro * (1.0f / 3) *
                                        (Vector3(b->X(), b->Y(), b->Z()) + Vector3(c->X(), c->Y(), b->Z()) +
                                         Vector3(c->X(), c->Y(), c->Z())));
    float vC = xyArea * (1.0f / 6) * (c->Z() - b->Z());

    // note: vA+vB+vC is equal to volume
    // sumVolume += volume;
    sumVolume += vA + vB + vC;
    sumCov += vA * tA + vB * tB + vC * tC;
}

void OcclusionPoly::SumXYVolume(float& volume, Vector3& cov) const
{
    // sum volume and center of volume*volume
    // calculate space between poly and plane (xy)
    Vector3Val ip = _v[0];
    for (int i = 2; i < _n; i++)
    {
        Vector3Val lp = _v[i - 1];
        Vector3Val rp = _v[i];
        ::SumXYVolume(volume, cov, &ip, &lp, &rp);
    }
}

void OcclusionPoly::SumCrossProducts(Vector3& sum) const
{
    // note: area is half of sum of crossproducts
    Vector3Val ip = _v[0];
    for (int i = 2; i < _n; i++)
    {
        Vector3Val lp = _v[i - 1];
        Vector3Val rp = _v[i];
        Vector3Val lmi = (lp - ip);
        sum += lmi.CrossProduct(rp - ip);
    }
}

void OcclusionPoly::SumPositions(Vector3& sum, int& count) const
{
    for (int i = 0; i < _n; i++)
    {
        sum += _v[i];
        count++;
    }
}

// project, clip and test bbox
bool Occlusion::TestBBox(const Matrix4& trans, const Vector3* minMax, ClipFlags clip) const
{
    Matrix4 camTransform = GScene->ScaledInvTransform() * trans;

    // first of all check center of object (VZero)
    // if this is not occluded, whole object must be checked/rendered

    // calculate 8 bbox corners in camera coordinates

    const float minZRequired = GScene->GetCamera()->ClipNear() * 10;

    Vector3 min(+1e10, +1e10, +1e10), max(-1e10, -1e10, -1e10);
    Vector3 cCorners[2][2][2];
    Vector3 sCorners[2][2][2];
    for (int lr = 0; lr < 2; lr++)
    {
        for (int ud = 0; ud < 2; ud++)
        {
            for (int fb = 0; fb < 2; fb++)
            {
                // get bbox corner in model coordinates
                Vector3 mCorner = MinMaxCorner(minMax, lr, ud, fb);
                Vector3& cCorner = cCorners[lr][ud][fb];
                Vector3& sCorner = sCorners[lr][ud][fb];

                // convert to camera coordinates
                cCorner.SetFastTransform(camTransform, mCorner);
                // some point is very close - we have to draw it
                if (cCorner.Z() < minZRequired)
                {
                    return true;
                }

                // calculate screen space
                float invZ = 1 / cCorner.Z();
                sCorner[0] = cCorner.X() * invZ;
                sCorner[1] = cCorner.Y() * invZ;
                sCorner[2] = cCorner.Z();

                saturateMin(min[0], sCorner[0]), saturateMax(max[0], sCorner[0]);
                saturateMin(min[1], sCorner[1]), saturateMax(max[1], sCorner[1]);
                saturateMin(min[2], sCorner[2]); // we are not interested in min. z
                                                 // saturateMax(max[2],sCorner[2]);
            }
        }
    }
    // check rectangle min.x,min.y .. max.x,maxy , with z min.z
    // check
    // render all 6 faces - will check only 3 faces (back face culling)
    // if any point is visible, whole object must be rendered
    // calculate screen space conservative minmax
    // use minimal z, get minmax x,y

    return TestProjectedBBox(min, max);
}

// render projected triangle to debug window
void Occlusion::DebugPoly(const OcclusionPoly& poly, DebugPixel color) const
{
#if _ENABLE_CHEATS
    int n = poly.N();
    int p = n - 1;
    for (int v = 0; v < n; v++)
    {
        Vector3Val pp = poly[p];
        Vector3Val pv = poly[v];

        _debugWin->Line(toIntFloor(pp.X() * _w / 2) + _w / 2, toIntFloor(pp.Y() * _h / 2) + _h / 2,
                        toIntFloor(pv.X() * _w / 2) + _w / 2, toIntFloor(pv.Y() * _h / 2) + _h / 2, color);

        p = v;
    }
#endif
}

#define FX(c, dim) toIntFloor((c) * (dim)) // convert to screen fixed point
#define FY(c, dim) ((c) * (dim))           // convert to screen float

#define SX(f, i, dim) ((f) * (dim) - (i)) // calculate how much have been skipped

class HPoint
{
  public:
    Fixed x, y;

    HPoint() = default; // default constructors - uninitialized polygon data
    HPoint(float xx, float yy)
    {
        x = fixed(xx);
        y = fixed(yy);
    }
};

inline float Invert(Fixed a)
{
    if (a != Fixed0)
    {
        return 1.0f / fxToFloat(a);
    }
    return 0;
}

#define CalcZHDelta(beg, end, invDist) fixed(fxToFloat(end - beg) * invDist)

#if _COMPILER_CAN_MMX
#define OPTIMIZE_FOR_MMX 1
#endif

#if SIMD || SIMD2
// unsigned 8b SSE occlusions
#define OccZToFloat(x) ((OccZTypeFar - (x)) * (1.0f / 256)) // for debugger
#define OccZTypeFar 0xff
#define OccZTypeNear 0
#define floatToOccZ(x) (OccZTypeFar - toInt(x * 0.25f))

#define OccZTypeFarFillByte 0

#else
// signed 8b MMX occlusions
#define OccZToFloat(x) ((OccZTypeFar - (x)) * (1.0f / 256)) // for debugger
#define OccZTypeFar 0x7f
#define OccZTypeNear -0x80
#define floatToOccZ(x) (OccZTypeFar - toInt(x * 0.25f))

#define OccZTypeFarFillByte 0x80
#endif

inline OccZType FloatToOccZ(float x)
{
    int xi = floatToOccZ(x);
    saturate(xi, OccZTypeNear, OccZTypeFar);
    return xi;
}

#if OPTIMIZE_FOR_MMX

// MMX optimization of
#include <mmintrin.h>
namespace Poseidon
{

#if SIMD
} // namespace Poseidon
#include <xmmintrin.h>
#endif
#if SIMD2
#include <emmintrin.h>
#endif

void Occlusion::RenderSpan(OccZType* tgt, int yBeg, int height, OccZType z)
{
    tgt += yBeg;
    if (height >= 16) // avoid setup overhead
    {
// perform horizontal fill
// align tgt
#if SIMD2
        const int alignLog = 4;
#else
        const int alignLog = 3;
#endif
        const int alignData = 1 << alignLog;
        const int alignMask = alignData - 1;
        int align = (alignData - yBeg) & alignMask;
        if (align <= height)
        {
            height -= align;
            while (--align >= 0)
            {
                if ((*tgt) < z)
                {
                    (*tgt) = z;
                }
                tgt++;
            }
        }
        int nUnroll = height >> alignLog;
        height &= alignMask;
        if (nUnroll > 0)
        {
// 4 shorts a time
#if SIMD2
            __m128i zQ = _mm_set1_epi8(z);
            __m128i* tgtQ = (__m128i*)tgt;
#else
            // SIMD / MMX
            __m64 zQ = _mm_set1_pi8(z);
            __m64* tgtQ = (__m64*)tgt;
#endif
            --nUnroll;
            do
            {
#if SIMD2                                        // PIV version
                *tgtQ = _mm_max_epu8(*tgtQ, zQ); // take bigger from both
#elif SIMD                                       // PIII version
                //__m64 tQ = *tgtQ;
                *tgtQ = _mm_max_pu8(*tgtQ, zQ); // take bigger from both
#else
                __m64 tQ = *tgtQ;
                // MMX version
                __m64 mask = _mm_cmpgt_pi8(zQ, tQ);
                tQ = _mm_andnot_si64(mask, tQ);
                mask = _mm_and_si64(mask, zQ);
                *tgtQ = _mm_or_si64(tQ, mask); // take bigger from both
#endif
                tgtQ++;
            } while (--nUnroll >= 0);
            tgt = (OccZType*)tgtQ;
        }
    }
    while (--height >= 0)
    {
        if ((*tgt) < z)
        {
            (*tgt) = z;
        }
        tgt++;
    }
}

#else

// No MMX code in non-optimized version - C4799 warning is false positive
#pragma warning(push)
#pragma warning(disable : 4799)
void Occlusion::RenderSpan(OccZType* tgt, int yBeg, int height, OccZType z)
{
    // perform horizontal fill
    // note: this could be well acomplished with SIMD2 intructions
    tgt += yBeg;
    int nUnroll = height >> 2;
    height &= 3;
    while (--nUnroll >= 0)
    {
        if (tgt[0] < z)
            tgt[0] = z;
        if (tgt[1] < z)
            tgt[1] = z;
        if (tgt[2] < z)
            tgt[2] = z;
        if (tgt[3] < z)
            tgt[3] = z;
        // height-=4;
        tgt += 4;
    }
    while (--height >= 0)
    {
        if ((*tgt) < z)
            (*tgt) = z;
        tgt++;
    }
}
#pragma warning(pop)

#endif

bool Occlusion::TestRect(const OccZType* col, int w, int h, OccZType maxz) const
{
    int skipCol = _h - h;
    // note: w and h is guaranteed to be >0
    {
        --w;
        --h;
        do
        {
            int hh = h;
            do
            {
                OccZType z = *col++;
                if (z <= maxz)
                {
                    return true;
                }
            } while (--hh >= 0);
            col += skipCol;
        } while (--w >= 0);
    }
    return false;
}

static DebugPixel ColorOccluded = DebugMemWindow::DColor(Color(0, 0, 1));
static DebugPixel ColorNotOccluded = DebugMemWindow::DColor(Color(0, 1, 0));

bool Occlusion::TestProjectedBBox(const Vector3& min, const Vector3& max) const
{
#if DETAIL_COUNTERS
#endif

    float minFX = min.X() * _w2 + _w2;
    float maxFX = max.X() * _w2 + _w2;
    float minFY = min.Y() * _h2 + _h2;
    float maxFY = max.Y() * _h2 + _h2;
    float wf = (float)_w;
    float hf = (float)_h;
    saturate(minFX, 0, wf);
    saturate(maxFX, 0, wf);
    saturate(minFY, 0, hf);
    saturate(maxFY, 0, hf);
    int minx = toIntFloor(minFX);
    int miny = toIntFloor(minFY);
    int maxx = toIntCeil(maxFX);
    int maxy = toIntCeil(maxFY);

    // Fixed fxz=fixed(HPointZScale(max.Z()));

    OccZType nearestZ = FloatToOccZ(min.Z());
    const OccZType* col = &_data[minx * _h + miny];
    int xw = maxx - minx;
    int yh = maxy - miny;
    if (yh <= 0)
    {
        return false;
    }
    if (xw <= 0)
    {
        return false;
    }

    bool ret = TestRect(col, xw, yh, nearestZ);
#if _ENABLE_CHEATS
    if (_debugWin)
    {
        // draw nonoccluded bbox with lines
        DebugPixel color = ret ? ColorNotOccluded : ColorOccluded;
        _debugWin->Line(minx, miny, maxx, miny, color);
        _debugWin->Line(maxx, miny, maxx, maxy, color);
        _debugWin->Line(maxx, maxy, minx, maxy, color);
        _debugWin->Line(minx, maxy, minx, miny, color);
    }
#endif
    return ret;
}

void Occlusion::RenderProjectedPoly(const OcclusionPoly& poly)
{
    int n = poly.N();
    if (n < 3)
    {
        return;
    }
    // render poly to occlusion z-buffer
    //
    // direct rendering

    // search for leftmost vertex
    // scan for furthest (max.) z

#define HP(v) HPoint((v)->X() * _w2 + _w2, (v)->Y() * _h2 + _h2);

    float farZ = 0;
    int leftI = 0;
    float leftX = 1e10;
    // LOG_DEBUG(World, "Vertices: {}",n);
    for (int i = 0; i < n; i++)
    {
        Vector3Val v = poly[i];

        HPoint vp = HP(&poly[i]);

        saturateMax(farZ, v.Z());
        if (leftX > v.X())
        {
            leftX = v.X(), leftI = i;
        }
        // LOG_DEBUG(World, "  {}: {:.2f},{:.2f}",i,fxToFloat(vp.x),fxToFloat(vp.y));
    }

    // int tCur = leftI, bCur = leftI;
    int tNxt = leftI - 1, bNxt = leftI + 1; // top and bottom current point
    if (tNxt < 0)
    {
        tNxt = n - 1;
    }
    if (bNxt >= n)
    {
        bNxt = 0;
    }

    // minZ is minimal z
    // 0 is far
    OccZType minZOcc = FloatToOccZ(farZ);

    HPoint tp = HP(&poly[leftI]);
    HPoint bp = tp;

    // LOG_DEBUG(World, "LFT {}, {:.2f},{:.2f}",leftI,fxToFloat(tp.x),fxToFloat(tp.y));

    // calculate next point (top and bottom)

    HPoint tnp = HP(&poly[tNxt]);
    HPoint bnp = HP(&poly[bNxt]);

    // calculate invariant delta Y

    Fixed yT, dxT(Fixed0); // actual positions
    Fixed yB, dxB(Fixed0);

    // L R - does not mean left or right in terms of x coord
    // rather in clockwise/counterclockwise sense

    int curC = fxToIntCeil(tp.x);

    // LOG_DEBUG(World, "TNP {}, {:.2f},{:.2f}",tNxt,fxToFloat(tnp.x),fxToFloat(tnp.y));
    // LOG_DEBUG(World, "BNP {}, {:.2f},{:.2f}",bNxt,fxToFloat(bnp.x),fxToFloat(bnp.y));

    OccZType* tgt = &Set(curC, 0);

    int vertRest = n - 2;

    bool recalcT = true, recalcB = true;
    while (vertRest >= 0)
    {
        // LOG_DEBUG(World, "vertRest {}",vertRest);
        //  process next vertex
        int nxtCT = fxToIntCeil(tnp.x);
        int nxtCB = fxToIntCeil(bnp.x);
        if (nxtCT > curC && recalcT)
        {
            recalcT = false;
            Fixed skip = fixed(curC) - tp.x;
            // some drawing will be done based on yR,zR - calculate correct deltas
            float invD = Invert(tnp.x - tp.x);
            dxT = CalcZHDelta(tp.y, tnp.y, invD);
            // correct difference between curL and aktX
            yT = tp.y + skip * dxT;
            // LOG_DEBUG(World, "next dxT {:.3f}",fxToFloat(dxT));
        }

        if (nxtCB > curC && recalcB)
        {
            recalcB = false;
            Fixed skip = fixed(curC) - bp.x;
            // some drawing will be done based on yL,zL - calculate correct deltas
            float invD = Invert(bnp.x - bp.x);
            dxB = CalcZHDelta(bp.y, bnp.y, invD);
            // correct difference between curL and aktX
            yB = bp.y + skip * dxB;
            // LOG_DEBUG(World, "next dxB {:.3f}",fxToFloat(dxB));
        }

        // calculat how much can be processed without recalculating dx
        int restT = nxtCT - curC;
        int restB = nxtCB - curC;
        int restTotal = _h - curC;

        // calculate how many rest can be simply drawn without any tests
        int restSure = restT;
        if (restSure > restB)
        {
            restSure = restB;
        }
        if (restSure > restTotal)
        {
            restSure = restTotal;
        }

        for (int cnt = restSure; --cnt >= 0;)
        {
            if (curC >= 0)
            {
                int beg = fxToIntCeil(yB);
                int end = fxToIntCeil(yT);

                // int beg=fxToIntCeil(yT);
                // int end=fxToIntCeil(yB);

                // LOG_DEBUG(World, "{}: Span {}..{}",curC,beg,end);

                // perform simple clipping (for a few pixels)
                if (beg < 0)
                {
                    beg = 0;
                }
                if (end > _h)
                {
                    end = _h;
                }

                if (end > beg)
                {
                    // correct difference between xBeg and yL

                    // perform linear interpolated fill
                    RenderSpan(tgt, beg, end - beg, minZOcc);
                }
            }
            // advance to next line
            curC++;
            tgt += _h;
            yT += dxT;
            yB += dxB;
        }
        restT -= restSure;
        restB -= restSure;
        restTotal -= restSure;

#if OPTIMIZE_FOR_MMX
        _mm_empty();
#endif

        if (restTotal <= 0)
        {
            break; // bottom end reached - nothing to draw
        }
        if (restT == 0)
        {
            // top edge done - move to next point
            tp = tnp;

            tNxt--;
            if (tNxt < 0)
            {
                tNxt = n - 1;
            }
            vertRest--;

            tnp = HP(&poly[tNxt]);
            // LOG_DEBUG(World, "TNP next {}, {:.2f},{:.2f}",tNxt,fxToFloat(tnp.x),fxToFloat(tnp.y));
            recalcT = true;
        }
        if (restB == 0)
        {
            // right edge done - move to next point
            bp = bnp;

            bNxt++;
            if (bNxt >= n)
            {
                bNxt = 0;
            }
            vertRest--;

            bnp = HP(&poly[bNxt]);
            // LOG_DEBUG(World, "BNP next {}, {:.2f},{:.2f}",bNxt,fxToFloat(bnp.x),fxToFloat(bnp.y));
            recalcB = true;
        }
    }
}

void Occlusion::RenderPoly(const OcclusionPoly& poly, ClipFlags clip)
{
    // clip polygon
    OcclusionPoly temp = poly;
    // check clipping
    if (clip)
    {
#if DETAIL_COUNTERS
#endif
        float cNear = 0.01f;
        float cFar = GScene->GetCamera()->ClipFar();
        temp.Clip(cNear, cFar, clip);
        if (temp.N() < 3)
        {
            return;
        }
    }

    temp.Perspective();

    RenderProjectedPoly(temp);
}

bool Occlusion::TestPoint(Vector3Par pos) const
{
    // point coordinates in clipping space
    // unable to check if point is outside clipping region
    if (pos.Z() <= 0)
    {
        return false;
    }
    if (fabs(pos.X()) > pos.Z())
    {
        return false;
    }
    if (fabs(pos.Y()) > pos.Z())
    {
        return false;
    }
    // project point
    // check if pixel is visible

    float invZ = 1 / pos.Z();
    int x = toInt(pos.X() * invZ * _w2 + _w2);
    int y = toInt(pos.Y() * invZ * _h2 + _h2);
    if (x < 0 || x >= _w)
    {
        return false;
    }
    if (y < 0 || y >= _h)
    {
        return false;
    }

// mark tested points as pink
#if _ENABLE_CHEATS
    if (_debugWin)
    {
        _debugWin->Plot(x, y, _debugWin->DColor(Color(1, 0, 1)));
    }
#endif

    OccZType nearestZ = FloatToOccZ(pos.Z());
    OccZType z = Get(x, y);
    // if point is nearer, is has bigger z
    return nearestZ >= z;
}

bool Occlusion::TestPointWSpace(Vector3Par pos) const
{
    // convert to clipping coordinates
    Vector3 clipPos;
    clipPos.SetFastTransform(GScene->ScaledInvTransform(), pos);
    return TestPoint(clipPos);
}

float Occlusion::TestSphereWSpace(Vector3Par pos, float radius) const
{
    // convert to clipping coordinates
    Vector3 clipPos;
    clipPos.SetFastTransform(GScene->ScaledInvTransform(), pos);

    if (clipPos.Z() <= 1e-20)
    {
        return false;
    }
    if (fabs(clipPos.X()) > clipPos.Z())
    {
        return false;
    }
    if (fabs(clipPos.Y()) > clipPos.Z())
    {
        return false;
    }
    // project point
    // check if pixel is visible

    float invZ = 1 / clipPos.Z();
    int x = toInt(clipPos.X() * invZ * _w2 + _w2);
    int y = toInt(clipPos.Y() * invZ * _h2 + _h2);
    // check some pixels around
    int sizeX = toIntFloor(radius * invZ * _w2);
    int sizeY = toIntFloor(radius * invZ * _h2);

    OccZType nearestZ = FloatToOccZ(clipPos.Z());
    return SampleCoverage(x, y, sizeX, sizeY, nearestZ);
}

float Occlusion::SampleCoverage(int x, int y, int sizeX, int sizeY, OccZType nearestZ) const
{
    int visible = 0, total = 0;
    int xMin = x - sizeX;
    int xMax = x + sizeX;
    int yMin = y - sizeY;
    int yMax = y + sizeY;
    saturateMax(xMin, 0);
    saturateMin(xMax, _w - 1); // inclusive loop: Get(_w, _h) would read one column/row past _data
    saturateMax(yMin, 0);
    saturateMin(yMax, _h - 1);
    for (int xx = xMin; xx <= xMax; xx++)
    {
        for (int yy = yMin; yy <= yMax; yy++)
        {
            total += 1;

            OccZType z = Get(xx, yy);
            visible += nearestZ >= z;

#if _ENABLE_CHEATS
            // mark tested points as pink
            if (_debugWin)
            {
                _debugWin->Plot(xx, yy, _debugWin->DColor(Color(0.5, 0, 0.5)));
            }
#endif
        }
    }

    // if point is nearer, is has bigger z
    if (visible <= 0)
    {
        return 0;
    }
    return visible * 1.0f / total;
}

struct Edge
{
    VertexIndex a, b;
    Edge() = default;
    Edge(VertexIndex aa, VertexIndex bb) : a(aa), b(bb) {}
};

// project, clip and render
void Occlusion::RenderComponent(const Matrix4& transform, Vector3Val camDir, Vector3Val camPos, Shape* shape,
                                ConvexComponent& component, ClipFlags clip)
{
    // scan convex component
    // select border edges (and list egde points)
    // we have normals of all planes
    // for each face:
    const FaceSelection& faces = component.FaceOffsets(shape);
    AUTO_STATIC_ARRAY(bool, frontFace, 256);
    // frontFace.Reserve(component.NPlanes());
    frontFace.Resize(component.NPlanes());

    for (int i = 0; i < component.NPlanes(); i++)
    {
        const Plane& plane = component.GetPlane(i);
        Offset o = faces[i];
        const Poly& face = shape->Face(o);
        PoseidonAssert(face.N() >= 3);
        // direction from camera
        Vector3Val fromCamDir = shape->Pos(face.GetVertex(0)) - camPos;
        // face normal
        Vector3Val normal = plane.Normal();
        // if face is backface
        frontFace[i] = normal * fromCamDir <= 0;
        // LOG_DEBUG(World, "  Front {} {}",i,frontFace[i]);
    }

    // use component edge neightbourgs information
    const ComponentEdges& cEdges = *component.GetEdges();

    StaticArray<Edge> buildEdges;
    static StaticStorage<Edge> buildEdgesS;
    buildEdges.SetStorage(buildEdgesS.Init(64));

    // select egdes between front and back faces
    for (int selI = 0; selI < component.NPlanes(); selI++)
    {
        if (!frontFace[selI])
        {
            continue; // check only front faces
        }
        // const Plane &plane = component.GetPlane(selI);
        // int faceI=component.Faces()[selI]; // face index in shape
        Offset faceO = faces[selI]; // face offset in shape
        const Poly& face = shape->Face(faceO);
        int n = face.N();
        int p = n - 1; // previous point
        // const Poly &overEdgesO = edges[faceI];
        const Poly& overEdges = cEdges[selI];
        for (int v = 0; v < n; v++)
        {
            // check edge p..v
            int overEdge = overEdges.GetVertex(v);
            if (overEdge < 0)
            {
                continue; // no neighbourgh
            }
            if (!frontFace[overEdge])
            {
                int vp = face.GetVertex(p); // vertex indices
                int vv = face.GetVertex(v);
                int pp = shape->VertexToPoint(vp); // point indices
                int pv = shape->VertexToPoint(vv);
                // we have egde between front and back faces
                // we add points into our builded polygon
                buildEdges.Add(Edge(pp, pv));
                // build.AddEdge(pp,pv);
            }
            p = v; // current becomes previous
        }
    }

    // multipass add edges to create continuous contour line
    BuildPoly build;
    build.SetN(0);
    {
    // scan buildEdges from end to begining (fast delete)
    // check first edge that can be added to
    NextEdge:
        for (int e = buildEdges.Size(); --e >= 0;)
        {
            const Edge& edge = buildEdges[e];
            if (build.AddEdge(edge.a, edge.b))
            {
                // able to add to continuous contour
                buildEdges.Delete(e);
                goto NextEdge;
            }
        }
    }

    OcclusionPoly poly;

    // we have to convert BuildPoly from points to vertices
    for (int i = 0; i < build.N(); i++)
    {
        int vp = build.GetVertex(i);
        int vv = shape->PointToVertex(vp);
        Vector3Val sPos = shape->Pos(vv);
        // transform point to normalized camera space (both x and y from -1 to 1)

        Vector3Val tPos = transform.FastTransform(sPos);

        poly.Add(tPos);
    }
    // now we can draw polygon
    RenderPoly(poly, clip);
}

void Occlusion::RenderShape(Matrix4& trans, Shape* shape, const ConvexComponents& components, ClipFlags clip)
{
    if (components.Size() <= 0)
    {
        return;
    }
    // draw only simple shapes
    // if( components.Size()!=1 ) return;

#if DETAIL_COUNTERS
#endif

    Matrix4Val invTrans = trans.InverseScaled();
    // convert camera to model space
    const Camera& cam = *GScene->GetCamera();
    Vector3Val cameraDir = invTrans.Rotate(cam.Direction());
    Vector3Val cameraPos = invTrans.FastTransform(cam.Position());
    shape->RecalculateNormalsAsNeeded();
    components.RecalculateAsNeeded(shape); // normal will be needed

    // calculate model to camera space transformation
    Matrix4 camTransform = GScene->ScaledInvTransform() * trans;

    // render all components
    for (int i = 0; i < components.Size(); i++)
    {
        RenderComponent(camTransform, cameraDir, cameraPos, shape, *components[i], clip);
    }
}

void Occlusion::OutputDebug()
{
#if _ENABLE_CHEATS
    DebugMemWindow* window = _debugWin;
    for (int x = 0; x < _w; x++)
        for (int y = 0; y < _h; y++)
        {
            // float zValue=Get(x,y)*(1.0/MaxOcclusionZ);
            // float zValue=OccZToFloat(Get(x,y))*(1.0/32);
            float zValue = OccZToFloat(Get(x, y));

            // draw only if pixel is black (i.e. no other diagnostics there)
            if (window->Get(x, y) == 0)
            {
                saturateMin(zValue, 0.8f);
                window->Set(x, y) = window->DColorGray(zValue);
            }
        }
#endif
}

void Occlusion::Clear()
{
    // reset z-buffer information
#if _ENABLE_CHEATS
    if (InputSubsystem::Instance().GetCheat2ToDo(SDL_SCANCODE_N))
    {
        if (!_debugWin)
            _debugWin = new DebugMemWindow("Occlusions", _w, _h);
    }
    if (_debugWin)
    {
        OutputDebug();       // draw z-buffer to background
        _debugWin->Update(); // send old frame data
        // reset output to black
        for (int x = 0; x < _w; x++)
            for (int y = 0; y < _h; y++)
            {
                _debugWin->Set(x, y) = 0;
            }
    }
#endif
    memset(_data, OccZTypeFarFillByte, sizeof(OccZType) * _w * _h);
}
