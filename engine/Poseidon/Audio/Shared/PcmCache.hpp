#pragma once

#if defined _WIN32
    #include <Poseidon/Foundation/Common/Win.h>
#endif
#include <Poseidon/Audio/Core/Format/RiffFile.hpp>

#include <cctype>
#include <cstdint>
#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace Poseidon
{
namespace Audio
{

// Decoded-PCM LRU cache keyed on the resolved wave path. Holds the
// post-decode PCM of small non-streamed waves (WSS/WAV SFX and speech)
// so repeat plays and manifest-preloaded mission sounds skip the file
// read and the decode. OGG never enters — it rides the chunked
// streaming path. Thread-safe: decode runs on TaskPool workers.
class PcmCache
{
  public:
    struct Entry
    {
        WAVEFORMATEX fmt{};
        unsigned int uncompressedSize = 0;
        std::vector<uint8_t> pcm;
    };
    using EntryPtr = std::shared_ptr<const Entry>;

    // Per-entry cap keeps music-sized PCM out (a full track is ~20 MB —
    // those decode async at play and stream); total cap bounds the
    // session footprint.
    static constexpr size_t kMaxEntryBytes = 4u * 1024u * 1024u;
    static constexpr size_t kMaxTotalBytes = 64u * 1024u * 1024u;

    EntryPtr Find(const char* name)
    {
        std::string key = Key(name);
        std::lock_guard<std::mutex> lock(_mx);
        auto it = _map.find(key);
        if (it == _map.end())
        {
            ++_misses;
            return nullptr;
        }
        _lru.splice(_lru.begin(), _lru, it->second.lruIt);
        ++_hits;
        return it->second.entry;
    }

    // Existence probe for the preload path — no LRU bump, no miss count.
    bool Contains(const char* name) const
    {
        std::string key = Key(name);
        std::lock_guard<std::mutex> lock(_mx);
        return _map.find(key) != _map.end();
    }

    bool Insert(const char* name, EntryPtr entry)
    {
        if (!entry || entry->pcm.empty() || entry->pcm.size() > kMaxEntryBytes)
        {
            return false;
        }
        std::string key = Key(name);
        std::lock_guard<std::mutex> lock(_mx);
        if (_map.find(key) != _map.end())
        {
            return false; // first decode wins; identical content anyway
        }
        while (_bytes + entry->pcm.size() > kMaxTotalBytes && !_lru.empty())
        {
            const std::string& victim = _lru.back();
            auto vit = _map.find(victim);
            _bytes -= vit->second.entry->pcm.size();
            _map.erase(vit);
            _lru.pop_back();
            ++_evictions;
        }
        _bytes += entry->pcm.size();
        _lru.push_front(key);
        _map.emplace(std::move(key), Slot{std::move(entry), _lru.begin()});
        ++_inserts;
        return true;
    }

    void Clear()
    {
        std::lock_guard<std::mutex> lock(_mx);
        _map.clear();
        _lru.clear();
        _bytes = 0;
    }

    // Evict LRU entries until at least `bytes` are freed (or the cache empties).
    // Returns bytes actually freed. Thread-safe; pure CPU PCM, safe to call from
    // the global memory-pressure path on any thread.
    size_t Evict(size_t bytes)
    {
        std::lock_guard<std::mutex> lock(_mx);
        size_t freed = 0;
        while (freed < bytes && !_lru.empty())
        {
            auto vit = _map.find(_lru.back());
            const size_t sz = vit->second.entry->pcm.size();
            freed += sz;
            _bytes -= sz;
            _map.erase(vit);
            _lru.pop_back();
            ++_evictions;
        }
        return freed;
    }

    struct Stats
    {
        size_t entries = 0;
        size_t bytes = 0;
        uint64_t hits = 0;
        uint64_t misses = 0;
        uint64_t inserts = 0;
        uint64_t evictions = 0;
    };

    Stats GetStats() const
    {
        std::lock_guard<std::mutex> lock(_mx);
        return Stats{_map.size(), _bytes, _hits, _misses, _inserts, _evictions};
    }

  private:
    // Paths reach the audio system in mixed case and mixed separators
    // depending on the caller (config vs script vs mission dir prefix).
    static std::string Key(const char* name)
    {
        std::string key = name ? name : "";
        for (char& c : key)
        {
            if (c == '/')
                c = '\\';
            else
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        return key;
    }

    struct Slot
    {
        EntryPtr entry;
        std::list<std::string>::iterator lruIt;
    };

    mutable std::mutex _mx;
    std::list<std::string> _lru; // front = most recently used
    std::unordered_map<std::string, Slot> _map;
    size_t _bytes = 0;
    uint64_t _hits = 0;
    uint64_t _misses = 0;
    uint64_t _inserts = 0;
    uint64_t _evictions = 0;
};

} // namespace Audio
} // namespace Poseidon
