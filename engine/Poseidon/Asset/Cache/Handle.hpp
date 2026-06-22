// Typed asset handle — a cheap (two uint32_t), copyable, hashable token.
// Survives slot reuse via a generation counter: a stale handle returns nullptr
// from the cache's Get() even if the slot was reallocated for a different key.
//
//   Handle<Texture> h = cache.Find("foo.paa");
//   // ... cache evicts and reuses the slot for "bar.paa" ...
//   Texture* p = cache.Get(h);     // nullptr — generation mismatch
//
// The type parameter T is for compile-time safety only — Handle<Texture> and
// Handle<Shape> are not interchangeable despite sharing the same representation.

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>

namespace Poseidon
{

template <typename T>
class Handle
{
  public:
    Handle() = default;
    Handle(uint32_t slot, uint32_t generation) : _slot(slot), _generation(generation) {}

    // Generation 0 is reserved as the "never assigned" sentinel.
    bool IsValid() const { return _generation != 0; }

    uint32_t Slot() const { return _slot; }
    uint32_t Generation() const { return _generation; }

    bool operator==(const Handle& other) const
    {
        return _slot == other._slot && _generation == other._generation;
    }
    bool operator!=(const Handle& other) const { return !(*this == other); }

    static Handle Invalid() { return Handle(); }

  private:
    uint32_t _slot       = 0;
    uint32_t _generation = 0;
};

} // namespace Poseidon

// std::hash specialisation so Handle<T> can be used as a key in unordered containers.
// Packs slot and generation into a single 64-bit value.
namespace std
{
template <typename T>
struct hash<::Poseidon::Handle<T>>
{
    size_t operator()(const ::Poseidon::Handle<T>& h) const noexcept
    {
        const uint64_t packed = (static_cast<uint64_t>(h.Generation()) << 32) | static_cast<uint64_t>(h.Slot());
        return std::hash<uint64_t>{}(packed);
    }
};
} // namespace std
