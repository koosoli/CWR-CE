// Terrain LOD and geometry computation — pure functions, no engine dependencies.
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <Poseidon/Asset/Cache/Handle.hpp>

#include <list>
#include <memory>
#include <unordered_map>

namespace Poseidon
{
constexpr int LandSegmentSizeLod = 8;

// LOD level computation

// Compute LOD level for a terrain segment based on its distance from the camera.
// Returns 0 (full detail), 1 (half), or 2 (quarter).
inline int ComputeSegmentLod(int segXBeg, int segZBeg, float camX, float camZ, float landGrid, int maxN)
{
    const float segWorldSize = LandSegmentSizeLod * landGrid;
    const float cacheRadius = std::sqrt(static_cast<float>(maxN) / 3.14159f) * segWorldSize;
    const float nearDist = cacheRadius * 0.33f;
    const float midDist = cacheRadius * 0.66f;

    float segCX = (segXBeg + LandSegmentSizeLod * 0.5f) * landGrid;
    float segCZ = (segZBeg + LandSegmentSizeLod * 0.5f) * landGrid;
    float dx = segCX - camX, dz = segCZ - camZ;
    float dist = std::sqrt(dx * dx + dz * dz);

    if (dist > midDist)
        return 2;
    if (dist > nearDist)
        return 1;
    return 0;
}

// Compute the LOD distance thresholds for a given configuration.
inline void ComputeLodThresholds(float landGrid, int maxN, float& nearDist, float& midDist)
{
    const float segWorldSize = LandSegmentSizeLod * landGrid;
    const float cacheRadius = std::sqrt(static_cast<float>(maxN) / 3.14159f) * segWorldSize;
    nearDist = cacheRadius * 0.33f;
    midDist = cacheRadius * 0.66f;
}

// LOD stride and vertex count computation (from GenerateSegmentInto)

// Compute the effective LOD stride — clamped to subdivision level.
// lodLevel: 0..2, subdivisionLevel: typically 1..4
inline int ComputeLodStride(int lodLevel, int subdivisionLevel)
{
    return 1 << (lodLevel < subdivisionLevel ? lodLevel : subdivisionLevel);
}

// Compute effective subdivision count after LOD reduction.
inline int ComputeEffectiveSubdivCount(int subdivisionLevel, int lodLevel)
{
    const int subdivisionCount = 1 << subdivisionLevel;
    const int lodStride = ComputeLodStride(lodLevel, subdivisionLevel);
    return subdivisionCount / lodStride;
}

// Compute the vertex count for a segment at a given LOD.
// segSize: number of land cells in segment (e.g. 8)
inline int ComputeVertexCount(int segSize, int subdivisionLevel, int lodLevel)
{
    const int eff = ComputeEffectiveSubdivCount(subdivisionLevel, lodLevel);
    const int countPerAxis = segSize * eff + 1;
    return countPerAxis * countPerAxis;
}

// Segment alignment (from Draw / Fill)

// Align a terrain coordinate down to segment boundary.
inline int AlignToSegmentBeg(int coord)
{
    return coord & ~(LandSegmentSizeLod - 1);
}

// Align a terrain coordinate up to next segment boundary.
inline int AlignToSegmentEnd(int coord)
{
    return (coord + (LandSegmentSizeLod - 1)) & ~(LandSegmentSizeLod - 1);
}

// Segment key hash (from landscape.hpp, duplicated for test independence)

struct LandSegKeyLod
{
    int x, z;
    bool operator==(const LandSegKeyLod& o) const { return x == o.x && z == o.z; }
};

struct LandSegKeyLodHash
{
    size_t operator()(const LandSegKeyLod& k) const
    {
        // Cast x to uint32_t before widening: left-shifting a negative signed value is UB.
        return std::hash<uint64_t>()((((uint64_t)(uint32_t)k.x << 32) | (uint32_t)k.z));
    }
};

// Cache sizing (from landscapeShared.hpp — engine-free version)

// Estimate cache size from view distance and grid spacing.
// horizontZ: view distance in world units, landGrid: grid cell size.
inline int EstimateCacheSize(float horizontZ, float landGrid)
{
    const float invLandGrid = 1.0f / landGrid;
    const float invSegSize = 1.0f / LandSegmentSizeLod;
    float cacheSize = (horizontZ * invLandGrid * invSegSize) * (horizontZ * invLandGrid * invSegSize) * 10.0f;
    int maxN = static_cast<int>(std::ceil(cacheSize)) + 8;
    if (maxN < 16)
        maxN = 16;
    const int maxReasonable = 512 * 512 / (LandSegmentSizeLod * LandSegmentSizeLod);
    if (maxN > maxReasonable)
        maxN = maxReasonable;
    return maxN;
}

// Normal computation (from GenerateSegmentInto — simplified for testing)

struct Vec3
{
    float x, y, z;
};

// Compute terrain normal from height differences at a point.
// xDelta: height(x+stride) - height(x-stride)
// zDelta: height(z+stride) - height(z-stride)
// terrainGrid: world distance between terrain samples
// lodStride: LOD stride multiplier
inline Vec3 ComputeTerrainNormal(float xDelta, float zDelta, float terrainGrid, int lodStride)
{
    // Cross product of (terrainGrid*lodStride, xDelta, 0) x (0, zDelta, terrainGrid*lodStride)
    float step = terrainGrid * lodStride;
    float nx = -(xDelta * step);                 // -(xDelta * step - 0)
    float ny = step * step;                       // step*step - 0
    float nz = -(zDelta * step);                  // -(0 - zDelta*step) -> wait...
    // Actually: offX = (step, xDelta, 0), offZ = (0, zDelta, step)
    // cross = (xDelta*step - 0, 0*0 - step*step, ... ) — NO
    // cross(a,b) = (a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x)
    // a = (step, xDelta, 0), b = (0, zDelta, step)
    nx = xDelta * step - 0;
    ny = 0 * 0 - step * step;
    nz = step * zDelta - xDelta * 0;
    // Engine actually uses: cp = offX.CrossProduct(offZ)
    // but convention may differ. Let's match exactly:
    // offX = Point3(terrainGrid*lodStride, xDelta, 0)
    // offZ = Point3(0, zDelta, terrainGrid*lodStride)
    // CrossProduct(a,b) = (a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x)
    nx = xDelta * step - 0 * zDelta;       // a.y*b.z - a.z*b.y
    ny = 0 * 0 - step * step;              // a.z*b.x - a.x*b.z
    nz = step * zDelta - xDelta * 0;       // a.x*b.y - a.y*b.x

    float len = std::sqrt(nx * nx + ny * ny + nz * nz);
    if (len > 0)
    {
        nx /= len;
        ny /= len;
        nz /= len;
    }
    return {nx, ny, nz};
}

// Border stretch (from GenerateSegmentInto)
constexpr float TerrainBorderStretch = 0.0001f;
constexpr float WaterBorderStretch = 0.001f;

// Height ↔ short conversion (from landscape.hpp ShortToHeight/HeightToShort)
constexpr float LandDataScale = 0.03f * 1.5f; // 0.045

inline float ShortToHeightLod(short val) { return val * LandDataScale; }
inline short HeightToShortLod(float val) { return static_cast<short>(std::floor(val * (1.0f / LandDataScale))); }

// Height interpolation (from Landscape::SurfaceY — pure version)

// Bilinear triangle interpolation of 4 corner heights.
// xIn, zIn: fractional position within the grid cell [0..1]
// y00, y01, y10, y11: heights at corners (x,z), (x+1,z), (x,z+1), (x+1,z+1)
inline float InterpolateHeight(float xIn, float zIn, float y00, float y01, float y10, float y11)
{
    if (xIn <= 1.0f - zIn)
    {
        // Lower-left triangle: 00, 01, 10
        float d1000 = y10 - y00;
        float d0100 = y01 - y00;
        return y00 + d1000 * zIn + d0100 * xIn;
    }
    else
    {
        // Upper-right triangle: 01, 10, 11
        float d1011 = y10 - y11;
        float d0111 = y01 - y11;
        return y10 + d0111 - d1011 * xIn - d0111 * zIn;
    }
}

// Compute terrain surface derivatives (slope) at a point.
// Returns (dX, dZ) derivatives scaled by invTerrainGrid.
inline void InterpolateHeightDerivatives(float xIn, float zIn, float y00, float y01, float y10, float y11,
                                         float invTerrainGrid, float& dX, float& dZ)
{
    if (xIn <= 1.0f - zIn)
    {
        float d1000 = y10 - y00;
        float d0100 = y01 - y00;
        dX = d0100 * invTerrainGrid;
        dZ = d1000 * invTerrainGrid;
    }
    else
    {
        float d1011 = y10 - y11;
        float d0111 = y01 - y11;
        dX = d1011 * -invTerrainGrid;
        dZ = d0111 * -invTerrainGrid;
    }
}

// Generic segment cache with LRU eviction and LOD invalidation
// Single implementation used by both production (LandCache) and unit tests.
// Entry must have public fields: int xBeg, zBeg, lodLevel; float lastUsed;

template <typename Entry>
class SegmentCache
{
public:
    struct LookupResult
    {
        Entry* entry;
        bool wasHit;       // true if cache hit with matching LOD
        bool wasLodEvict;  // true if cache hit but LOD mismatched → regenerated
    };

    explicit SegmentCache(int maxN) : _maxN(maxN) {}

    void SetMaxN(int maxN) { _maxN = maxN; }
    int GetMaxN() const { return _maxN; }
    size_t Size() const { return _index.size(); }

    // High-level cache lookup with LOD invalidation and time-based eviction.
    // generate: (int xBeg, int zBeg, int lodLevel, float time) → unique_ptr<Entry>
    template <typename GenFn>
    LookupResult Lookup(int xBeg, int zBeg, int requiredLod, float currentTime, GenFn&& generate)
    {
        LandSegKeyLod key{xBeg, zBeg};
        auto it = _index.find(key);

        if (it != _index.end())
        {
            auto lruIt = it->second;
            Entry* existing = lruIt->get();

            if (existing->lodLevel != requiredLod)
            {
                // LOD mismatch — evict stale entry and regenerate
                _lru.erase(lruIt);
                _index.erase(it);

                auto fresh = generate(xBeg, zBeg, requiredLod, currentTime);
                Entry* freshPtr = fresh.get();
                EvictIfOverCapacity();
                _lru.push_front(std::move(fresh));
                _index[key] = _lru.begin();
                return {freshPtr, false, true};
            }

            // Cache hit — promote to MRU
            _lru.splice(_lru.begin(), _lru, lruIt);
            existing->lastUsed = currentTime;
            return {existing, true, false};
        }

        // Evict entries older than 60 seconds (scan from LRU end)
        EvictExpired(currentTime, 60.0f);

        // Cache miss — generate new entry
        auto fresh = generate(xBeg, zBeg, requiredLod, currentTime);
        Entry* freshPtr = fresh.get();
        EvictIfOverCapacity();
        _lru.push_front(std::move(fresh));
        _index[key] = _lru.begin();
        return {freshPtr, false, false};
    }

    // Low-level access for batch operations (Fill)
    Entry* Find(int xBeg, int zBeg)
    {
        auto it = _index.find(LandSegKeyLod{xBeg, zBeg});
        if (it == _index.end()) return nullptr;
        return it->second->get();
    }

    void Remove(int xBeg, int zBeg)
    {
        auto it = _index.find(LandSegKeyLod{xBeg, zBeg});
        if (it == _index.end()) return;
        _lru.erase(it->second);
        _index.erase(it);
    }

    void TouchMRU(int xBeg, int zBeg, float time)
    {
        auto it = _index.find(LandSegKeyLod{xBeg, zBeg});
        if (it == _index.end()) return;
        _lru.splice(_lru.begin(), _lru, it->second);
        (*it->second)->lastUsed = time;
    }

    void Insert(std::unique_ptr<Entry> entry)
    {
        LandSegKeyLod key{entry->xBeg, entry->zBeg};
        _lru.push_front(std::move(entry));
        _index[key] = _lru.begin();
    }

    bool EvictLRU()
    {
        if (_lru.empty()) return false;
        auto& last = _lru.back();
        _index.erase(LandSegKeyLod{last->xBeg, last->zBeg});
        _lru.pop_back();
        return true;
    }

    Entry* MostRecent() const { return _lru.empty() ? nullptr : _lru.front().get(); }
    Entry* LeastRecent() const { return _lru.empty() ? nullptr : _lru.back().get(); }

    template <typename Fn>
    void ForEach(Fn&& fn)
    {
        for (auto& uptr : _lru)
            fn(*uptr);
    }

    template <typename Fn>
    void ForEach(Fn&& fn) const
    {
        for (auto& uptr : _lru)
            fn(*uptr);
    }

    void Clear()
    {
        _lru.clear();
        _index.clear();
    }

private:
    int _maxN;
    using LruList = std::list<std::unique_ptr<Entry>>;
    using LruIter = typename LruList::iterator;
    LruList _lru; // front = MRU, back = LRU
    std::unordered_map<LandSegKeyLod, LruIter, LandSegKeyLodHash> _index;

    void EvictIfOverCapacity()
    {
        while ((int)_index.size() >= _maxN && !_lru.empty())
        {
            auto& last = _lru.back();
            _index.erase(LandSegKeyLod{last->xBeg, last->zBeg});
            _lru.pop_back();
        }
    }

    void EvictExpired(float currentTime, float maxAge)
    {
        while (!_lru.empty())
        {
            auto& last = _lru.back();
            if (last->lastUsed < currentTime - maxAge)
            {
                _index.erase(LandSegKeyLod{last->xBeg, last->zBeg});
                _lru.pop_back();
            }
            else
            {
                break; // LRU ordering ensures remaining entries are newer
            }
        }
    }
};

// Test entry type and convenience wrapper for unit tests

struct SegmentCacheEntry
{
    int xBeg, zBeg;
    int lodLevel;
    float lastUsed;
    int generationId; // monotonic ID for tracking generate order in tests
};

// Backward-compatible wrapper — keeps the same API the tests already use.
class SegmentCacheLod : public SegmentCache<SegmentCacheEntry>
{
public:
    int generateCount = 0;

    using SegmentCache::SegmentCache;

    LookupResult Lookup(int xBeg, int zBeg, int requiredLod, float currentTime)
    {
        return SegmentCache::Lookup(xBeg, zBeg, requiredLod, currentTime,
            [this](int x, int z, int lod, float time) {
                auto e = std::make_unique<SegmentCacheEntry>();
                e->xBeg = x;
                e->zBeg = z;
                e->lodLevel = lod;
                e->lastUsed = time;
                e->generationId = generateCount++;
                return e;
            });
    }
};

}  // namespace Poseidon
