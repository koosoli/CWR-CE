#include <Poseidon/Core/Application.hpp>
#include <Poseidon/AI/Path/AITypes.hpp>
#include <Poseidon/IO/LockCache.hpp>
#include <Poseidon/Game/OperMap.hpp>
#include <Poseidon/World/Terrain/Landscape.hpp>

#include <Poseidon/Core/Global.hpp>
#include <string.h>
#include <Poseidon/Foundation/Containers/Array.hpp>
#include <Poseidon/Foundation/Containers/Array2D.hpp>
#include <Poseidon/Foundation/Containers/CacheList.hpp>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Poseidon/Foundation/Framework/Log.hpp>
#include <Poseidon/Foundation/Memory/FastAlloc.hpp>
#include <Poseidon/Foundation/Memory/MemFreeReq.hpp>
#include <Poseidon/Foundation/Time/Time.hpp>
#include <Poseidon/Foundation/Types/Memtype.h>
#include <Poseidon/Foundation/Types/Pointers.hpp>

#define SOLDIER_LOCK_MASK 0x0f
#define VEHICLE_LOCK_MASK 0xf0

namespace Poseidon
{
DEFINE_FAST_ALLOCATOR(LockField)

LockField::LockField(int x, int z)
{
    PoseidonAssert(InRange(z, x));
    memset(_locks, 0, sizeof(_locks));
    _next = _prev = nullptr;
    _x = (WORD)x;
    _z = (WORD)z;
    _lock = 1;
}

LockField::~LockField()
{
    if (_prev)
    {
        _prev->_next = _next;
    }
    if (_next)
    {
        _next->_prev = _prev;
    }
}

bool LockField::IsLocked(int x, int z, bool soldier)
{
    PoseidonAssert(x >= 0 && x < OperItemRange && z >= 0 && z < OperItemRange);
    return soldier ? (_locks[z][x] & VEHICLE_LOCK_MASK) > 0 : // soldier - soldier lock disabled
               _locks[z][x] > 0;
}

void LockField::Lock(int x, int z, bool soldier, bool lock)
{
    PoseidonAssert(x >= 0 && x < OperItemRange && z >= 0 && z < OperItemRange);
    if (soldier)
    {
        if (lock)
        {
            if ((_locks[z][x] & SOLDIER_LOCK_MASK) < 0x0f)
            {
                _locks[z][x]++;
            }
        }
        else
        {
            if ((_locks[z][x] & SOLDIER_LOCK_MASK) > 0)
            {
                _locks[z][x]--;
            }
        }
    }
    else
    {
        if (lock)
        {
            if ((_locks[z][x] & VEHICLE_LOCK_MASK) < 0xf0)
            {
                _locks[z][x] += 0x10;
            }
        }
        else
        {
            if ((_locks[z][x] & VEHICLE_LOCK_MASK) > 0)
            {
                _locks[z][x] -= 0x10;
            }
        }
    }
}

class LockCache : public ILockCache
{
  private:
    Array2D<LockField*> _lockFields;
    int _searchID;

  public:
    LockCache(Landscape* land)
    {
        _searchID = 0;
        Init(land);
    };
    ~LockCache() override { Destroy(); }

    int SearchID() override { return ++_searchID; }

    bool IsLocked(int x, int z, bool soldier) override;
    LockField* GetLockField(int x, int z) override;
    void ReleaseLockField(int x, int z) override;
    bool IsEmpty() const override;

    LockField* FindLockField(int x, int z) override; // use for diagnostics

  private:
    void Init(Landscape* land);
    void Destroy();
};

class OperCache : public IOperCache, public Foundation::MemoryFreeOnDemandHelper
{
  private:
    CLRefList<OperField> _operFields;
    int _count; // current count
    // fast operField searching
    Array2D<OperField*> _index; // 1/4 MB
                                // consider: operCache access well encapsulated
                                // we may work with pointers instead of links

  public:
    OperField* GetOperField(int x, int z, int mask) override;
    void RemoveField(OperField* fld) override;
    void RemoveField(int x, int z) override;
    Ref<OperField> RemoveOld(float limit, int untilSize); // return some removed entry for recycling

    OperCache(Landscape* land);
    ~OperCache() override;

    // implementation of MemoryFreeOnDemandHelper abstract interface
    size_t FreeOneItem() override;
    float Priority() override;

    // memory-budget observability (dev panel). No hard budget — paths are
    // cheap to recompute, so this domain is purely informational.
    const char* DomainName() const override { return "Terrain.OperPaths"; }
    size_t HeldBytes() const override { return (size_t)_count * sizeof(OperField); }

    int NFields() const override { return _operFields.Size(); }
};

ILockCache* CreateLockCache(Landscape* land)
{
    return new LockCache(land);
}
} // namespace Poseidon
IOperCache* CreateOperCache(Landscape* land)
{
    using namespace Poseidon;
    return new OperCache(land);
}
namespace Poseidon
{

bool LockCache::IsLocked(int x, int z, bool soldier)
{
    if (x < 0 || x >= OperItemRange * LandRange || z < 0 || z >= OperItemRange * LandRange)
    {
        return false;
    }

    int x1 = x / OperItemRange;
    int z1 = z / OperItemRange;
    int x0 = x1 / BigFieldSize;
    int z0 = z1 / BigFieldSize;
    LockField* fld = nullptr;
    LockField* p = _lockFields(x0, z0);
    while (p)
    {
        if ((p->_x == x1) && (p->_z == z1))
        {
            fld = p;
            break;
        }
        p = p->_next;
    }

    if (!fld)
    {
        return false;
    }

    return fld->IsLocked(x - x1 * OperItemRange, z - z1 * OperItemRange, soldier);
}

bool LockCache::IsEmpty() const
{
    bool empty = true;
    const int rngX = _lockFields.GetXRange();
    const int rngZ = _lockFields.GetYRange();
    for (int i = 0; i < rngZ; i++)
    {
        for (int j = 0; j < rngX; j++)
        {
            LockField* p = _lockFields(j, i);
            if (p)
            {
                empty = false;
                LOG_DEBUG(Core, "Lock field {},{} not empty", i, j);
            }
        }
    }
    return empty;
}

LockField* LockCache::GetLockField(int x, int z)
{
    if (!InRange(z, x))
    {
        return nullptr;
    }

    int x0 = x / BigFieldSize;
    int z0 = z / BigFieldSize;
    LockField* fld = nullptr;
    LockField* p = _lockFields(x0, z0);
    while (p)
    {
        if ((p->_x == x) && (p->_z == z))
        {
            fld = p;
            break;
        }
        p = p->_next;
    }

    if (fld)
    {
        fld->_lock++;
        return fld;
    }

    fld = new LockField(x, z);
    fld->_next = _lockFields(x0, z0);
    _lockFields(x0, z0) = fld;
    if (fld->_next)
    {
        fld->_next->_prev = fld;
    }

    return fld;
}

void LockCache::ReleaseLockField(int x, int z)
{
    if (!InRange(z, x))
    {
        return;
    }

    int x0 = x / BigFieldSize;
    int z0 = z / BigFieldSize;
    LockField* fld = nullptr;
    LockField* p = _lockFields(x0, z0);
    while (p)
    {
        if ((p->_x == x) && (p->_z == z))
        {
            fld = p;
            break;
        }
        p = p->_next;
    }

    if (fld == nullptr)
    {
        Fail("Cannot unlock field that was not locked.");
        return;
    }

    if (--fld->_lock <= 0)
    {
        if (fld == _lockFields(x0, z0))
        {
            _lockFields(x0, z0) = fld->_next;
        }
        delete fld;
    }
}

LockField* LockCache::FindLockField(int x, int z)
{
    if (!InRange(z, x))
    {
        return nullptr;
    }

    int x0 = x / BigFieldSize;
    int z0 = z / BigFieldSize;
    LockField* p = _lockFields(x0, z0);
    while (p)
    {
        if ((p->_x == x) && (p->_z == z))
        {
            return p;
        }
        p = p->_next;
    }

    return nullptr;
}

void LockCache::Init(Landscape* land)
{
    int fieldsRange = land->GetLandRange() / BigFieldSize;

    _lockFields.Dim(fieldsRange, fieldsRange);
    for (int i = 0; i < fieldsRange; i++)
    {
        for (int j = 0; j < fieldsRange; j++)
        {
            _lockFields(j, i) = 0;
        }
    }
}

void LockCache::Destroy()
{
    const int rngX = _lockFields.GetXRange();
    const int rngZ = _lockFields.GetYRange();
    LockField *p, *p2;
    for (int i = 0; i < rngZ; i++)
    {
        for (int j = 0; j < rngX; j++)
        {
            p = _lockFields(j, i);
            if (p)
            {
                LOG_DEBUG(Core, "Lock field {},{} not empty", i, j);
            }
            while (p)
            {
                p2 = p;
                p = p->_next;
                delete p2;
            }
            _lockFields(j, i) = 0;
        }
    }
}

// static (based on system memory)
// dynamic (based on free system memory)

const int OperCacheMinSize = 64;
const int OperCacheNormalSize = 256;
const int OperCacheMaxSize = 1024;

OperField* OperCache::GetOperField(int x, int z, int mask)
{
    if (!InRange(z, x))
    {
        return nullptr;
    }
    OperField* fld;
    fld = _index(x, z);
    if (fld)
    {
        PoseidonAssert(fld->_x == x);
        PoseidonAssert(fld->_z == z);
        Ref<OperField> temp = fld;
        _operFields.Delete(fld); // remove from current location
        _operFields.Insert(fld); // insert as first
        fld->_lastUsed = Glob.time;
        return fld;
    }

    // _data is sorted by time when it was used
    Ref<OperField> entry;
    if (_count >= OperCacheMinSize)
    {
        // we have more entries than we would like - remove only very old entries
        entry = RemoveOld(60, OperCacheMinSize);
        if (_count >= OperCacheNormalSize)
        {
            // we have to much entries - remove more recent entries
            entry = RemoveOld(5, OperCacheNormalSize);
            if (_count >= OperCacheMaxSize)
            {
                // we must release something - memory usage critical
                // we have to remove last field
                // try to reuse entry if possible
                // remove last entry
                entry = _operFields.Last();
                _index(entry->_x, entry->_z) = 0;
                _operFields.Delete(entry);
                _count--;
            }
        }
    }
    if (!entry)
    {
        entry = new OperField(x, z, mask);
    }
    else
    {
        entry->Init(x, z, mask);
    }

    // create a new entry
    _operFields.Insert(entry);
    _count++;
    _index(x, z) = entry;
    entry->_lastUsed = Glob.time;
    return entry;
}

void OperCache::RemoveField(OperField* fld)
{
    _index(fld->_x, fld->_z) = 0;
    _operFields.Delete(fld);
    _count--;
}

void OperCache::RemoveField(int x, int z)
{
    if (!InRange(z, x))
    {
        return;
    }
    OperField* fld = _index(x, z);
    if (!fld)
    {
        return;
    }
    RemoveField(fld);
}

Ref<OperField> OperCache::RemoveOld(float limit, int untilSize)
{
    Ref<OperField> ret;
    // last used fields are at the end of the list
    for (OperField *fld = _operFields.Last(), *prv; fld; fld = prv)
    {
        prv = _operFields.Prev(fld);
        // if we find some too new entry, we may be sure all other will be newer
        if (fld->_lastUsed >= Glob.time - limit)
        {
            break;
        }
        if (!ret)
        {
            ret = fld; // recycle first entry removed
        }
        RemoveField(fld);
        // check if we have reached the limit
        if (_count <= untilSize)
        {
            return ret;
        }
    }
    return ret;
}

OperCache::OperCache(Landscape* land)
{
    _count = 0; // current count
    const int landRange = land->GetLandRange();
    _index.Dim(landRange, landRange);
    for (int z = 0; z < landRange; z++)
    {
        for (int x = 0; x < landRange; x++)
        {
            _index(x, z) = 0;
        }
    }
    RegisterMemoryFreeOnDemand(this);
}

OperCache::~OperCache() = default;

// implementation of MemoryFreeOnDemandHelper abstract interface

const int OperFieldMemSize = sizeof(OperField);

size_t OperCache::FreeOneItem()
{
    OperField* fld = _operFields.Last();
    if (!fld)
    {
        return 0;
    }
    RemoveField(fld);
    return OperFieldMemSize;
}

float OperCache::Priority()
{
    // estimated time to create OperField (CPU cycles)
    const float itemTime = 100000;
    // estimated time per byte
    return itemTime / OperFieldMemSize;
}
} // namespace Poseidon
