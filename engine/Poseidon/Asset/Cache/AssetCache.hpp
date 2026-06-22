// Generic versioned asset cache — keyed storage with generation-based Handle tokens,
// slot reuse, and observable hit/miss counters.
//
//   AssetCache<Texture, Ref<Texture>> tex;
//   auto h = tex.Insert("foo.paa", LoadTexture("foo.paa"));
//   Texture* t = tex.Get(h);     // O(1)
//   tex.Remove(h);
//   tex.Get(h);                  // nullptr — handle stale
//
// `T` is the asset type. `Storage` is the smart-pointer wrapper (default
// std::shared_ptr<T>); engine consumers using the intrusive Ref<T> pass that
// as the second arg. `Storage` contract:
//   - default-constructible to "empty"
//   - copy-constructible / copy-assignable
//   - comparable to nullptr via operator bool
//   - operator* and operator-> returning T& / T*
//
// KeyT defaults to StringKey (case-insensitive ASCII, matches BankArray).
// Engine consumers using RStringB pass that as KeyT directly.
//
// Eviction policy: not provided. Callers that need LRU or memory-budget
// iterate + call Remove(). A single policy abstraction was rejected during
// design — the existing TextBank's mip-level frame-LRU is too specific.
//
// Thread safety: not thread-safe. Callers serialise.

#pragma once

#include <Poseidon/Asset/Cache/Handle.hpp>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace Poseidon
{

// Case-insensitive ASCII string key. Matches existing BankArray semantics (strcmpi lookup).
struct StringKey
{
    std::string value;

    StringKey() = default;
    StringKey(const std::string& v) : value(v) {}
    StringKey(const char* v) : value(v ? v : "") {}

    bool operator==(const StringKey& o) const
    {
        if (value.size() != o.value.size())
            return false;
        for (size_t i = 0; i < value.size(); ++i)
        {
            const unsigned char a  = static_cast<unsigned char>(value[i]);
            const unsigned char b  = static_cast<unsigned char>(o.value[i]);
            const unsigned char la = (a >= 'A' && a <= 'Z') ? a + 32 : a;
            const unsigned char lb = (b >= 'A' && b <= 'Z') ? b + 32 : b;
            if (la != lb)
                return false;
        }
        return true;
    }
};

struct StringKeyHash
{
    // FNV-1a over lowercased bytes — case-insensitive hash matching the equality above.
    size_t operator()(const StringKey& k) const noexcept
    {
        size_t h = 14695981039346656037ULL;
        for (char c : k.value)
        {
            const unsigned char u  = static_cast<unsigned char>(c);
            const unsigned char lc = (u >= 'A' && u <= 'Z') ? u + 32 : u;
            h ^= static_cast<size_t>(lc);
            h *= 1099511628211ULL;
        }
        return h;
    }
};

struct CacheStats
{
    size_t hits      = 0;
    size_t misses    = 0;
    size_t inserts   = 0;
    size_t evictions = 0;
    size_t size      = 0;
};

template <typename T, typename Storage = std::shared_ptr<T>, typename KeyT = StringKey,
          typename KeyHash = StringKeyHash>
class AssetCache
{
  public:
    using HandleT  = ::Poseidon::Handle<T>;
    using StorageT = Storage;
    using KeyType  = KeyT;

    AssetCache() = default;
    ~AssetCache() = default;

    AssetCache(const AssetCache&)            = delete;
    AssetCache& operator=(const AssetCache&) = delete;
    AssetCache(AssetCache&&)                 = default;
    AssetCache& operator=(AssetCache&&)      = default;

    // Find by key. Returns invalid handle if not present (bumps misses).
    HandleT Find(const KeyT& key) const
    {
        const auto it = _keyIndex.find(key);
        if (it == _keyIndex.end())
        {
            ++_stats.misses;
            return HandleT::Invalid();
        }
        ++_stats.hits;
        const uint32_t slot = it->second;
        return HandleT(slot, _slots[slot].generation);
    }

    // Direct lookup by key, returning T*. Returns nullptr if not present.
    T* Lookup(const KeyT& key) const
    {
        const auto it = _keyIndex.find(key);
        if (it == _keyIndex.end())
        {
            ++_stats.misses;
            return nullptr;
        }
        ++_stats.hits;
        return GetRaw(_slots[it->second].value);
    }

    // Insert a new entry. If the key is already cached, returns the existing handle
    // and drops the new value — same idempotent semantics as BankArray::New. Callers
    // needing replace should call Remove(Find(key)) first.
    HandleT Insert(const KeyT& key, Storage value)
    {
        if (const auto it = _keyIndex.find(key); it != _keyIndex.end())
        {
            ++_stats.hits;
            const uint32_t slot = it->second;
            return HandleT(slot, _slots[slot].generation);
        }

        ++_stats.inserts;
        uint32_t slot;
        if (!_freeSlots.empty())
        {
            slot = _freeSlots.back();
            _freeSlots.pop_back();
        }
        else
        {
            slot = static_cast<uint32_t>(_slots.size());
            _slots.emplace_back();
        }

        Slot& s   = _slots[slot];
        s.key     = key;
        s.value   = std::move(value);
        s.occupied = true;
        // Generation 0 is reserved as "invalid" — skip it on wraparound
        do
        {
            ++s.generation;
        } while (s.generation == 0);

        _keyIndex[key] = slot;
        ++_stats.size;
        return HandleT(slot, s.generation);
    }

    // Resolve a handle to the stored T*. Returns nullptr if invalid, freed, or stale.
    T* Get(HandleT h) const
    {
        if (!h.IsValid())
            return nullptr;
        if (h.Slot() >= _slots.size())
            return nullptr;
        const Slot& s = _slots[h.Slot()];
        if (!s.occupied || s.generation != h.Generation())
            return nullptr;
        return GetRaw(s.value);
    }

    // Same as Get but returns the Storage so callers can keep the asset alive after eviction.
    const Storage& GetStorage(HandleT h) const
    {
        static const Storage empty{};
        if (!h.IsValid() || h.Slot() >= _slots.size())
            return empty;
        const Slot& s = _slots[h.Slot()];
        if (!s.occupied || s.generation != h.Generation())
            return empty;
        return s.value;
    }

    // Remove by handle. No-op if stale or invalid.
    void Remove(HandleT h)
    {
        if (!h.IsValid() || h.Slot() >= _slots.size())
            return;
        Slot& s = _slots[h.Slot()];
        if (!s.occupied || s.generation != h.Generation())
            return;
        _keyIndex.erase(s.key);
        s.value    = Storage{};
        s.occupied = false;
        s.key      = KeyT{};
        _freeSlots.push_back(h.Slot());
        ++_stats.evictions;
        --_stats.size;
    }

    // Remove by key — avoids double lookup compared to Remove(Find(key)).
    void Remove(const KeyT& key)
    {
        const auto it = _keyIndex.find(key);
        if (it == _keyIndex.end())
            return;
        const uint32_t slot = it->second;
        Slot&          s    = _slots[slot];
        _keyIndex.erase(it);
        s.value    = Storage{};
        s.occupied = false;
        s.key      = KeyT{};
        _freeSlots.push_back(slot);
        ++_stats.evictions;
        --_stats.size;
    }

    // Evict every entry. Stats counters retain lifetime totals.
    void Clear()
    {
        for (uint32_t slot = 0; slot < _slots.size(); ++slot)
        {
            if (_slots[slot].occupied)
            {
                _slots[slot].value    = Storage{};
                _slots[slot].occupied = false;
                _slots[slot].key      = KeyT{};
                _freeSlots.push_back(slot);
                ++_stats.evictions;
            }
        }
        _keyIndex.clear();
        _stats.size = 0;
    }

    // Iterate over live entries. Callback receives (handle, key, T&). Order is unspecified.
    template <typename F>
    void ForEach(F&& fn) const
    {
        for (uint32_t slot = 0; slot < _slots.size(); ++slot)
        {
            const Slot& s = _slots[slot];
            if (!s.occupied)
                continue;
            // Weak-storage adapters (e.g. Link<T>) may return nullptr if the
            // referenced object was destroyed since insertion — skip those.
            T* raw = GetRaw(s.value);
            if (!raw)
                continue;
            fn(HandleT(slot, s.generation), s.key, *raw);
        }
    }

    size_t            Size() const { return _stats.size; }
    const CacheStats& GetStats() const { return _stats; }
    void              ResetStats() { _stats = CacheStats{_stats.size, 0, 0, 0, 0}; }

    // C-string overloads — one implicit conversion in a chain is the C++ limit,
    // so engine sites passing RStringB (which converts to const char* but not directly
    // to StringKey) need this short-circuit.
    HandleT Find(const char* key) const { return Find(KeyT(key)); }
    T*      Lookup(const char* key) const { return Lookup(KeyT(key)); }
    HandleT Insert(const char* key, Storage value) { return Insert(KeyT(key), std::move(value)); }
    void    Remove(const char* key) { Remove(KeyT(key)); }

  private:
    struct Slot
    {
        KeyT     key;
        Storage  value;
        uint32_t generation = 0;
        bool     occupied   = false;
    };

    // Get raw T* from any Storage that exposes operator->. Both std::shared_ptr<T>
    // and engine's Ref<T> return T* or nullptr. operator-> gives a uniform
    // null-safe accessor without SFINAE / if-constexpr ceremony.
    static T* GetRaw(const Storage& s) { return s.operator->(); }

    std::vector<Slot>                           _slots;
    std::vector<uint32_t>                       _freeSlots;
    std::unordered_map<KeyT, uint32_t, KeyHash> _keyIndex;
    mutable CacheStats                          _stats;
};

} // namespace Poseidon
