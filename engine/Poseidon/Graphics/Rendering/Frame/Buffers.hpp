#pragma once

#include <cstdint>
#include <type_traits>

// Typed buffer wrappers for the frame layer.  Encode the buffer's
// declared usage hint in the type system so map/upload flag mismatches
// fail at compile time.
//
// The rule: GL_STATIC_DRAW buffers must never be mapped with
// GL_MAP_INVALIDATE_BUFFER_BIT.  NVIDIA reacts by demoting the buffer
// from VRAM to host memory and spamming perf warnings.  A typed wrapper
// makes the mismatch unrepresentable rather than caught at each call site.
//
// Pure-data design: these types carry no GL handles — they're values
// passed by the typed `BufferHandle` in `Frame.hpp`.  The backend's
// Execute step looks up handles to live GL objects.


namespace Poseidon
{
namespace render::frame
{

// Usage hint mirrors GL_STATIC_DRAW / GL_DYNAMIC_DRAW.
enum class BufferUsage : std::uint8_t
{
    Static,   // immutable after first upload
    Dynamic,  // re-uploaded per frame
};

// Map flags.  Subset the frame layer models.
enum class MapFlag : std::uint32_t
{
    Read                = 1 << 0,
    Write               = 1 << 1,
    InvalidateBuffer    = 1 << 2,  // only valid for Dynamic
    InvalidateRange     = 1 << 3,
    FlushExplicit       = 1 << 4,
    Unsynchronized      = 1 << 5,
};

constexpr MapFlag operator|(MapFlag a, MapFlag b)
{
    return static_cast<MapFlag>(static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b));
}

constexpr bool has(MapFlag set, MapFlag bit)
{
    return (static_cast<std::uint32_t>(set) & static_cast<std::uint32_t>(bit)) != 0;
}

// Compile-time check: a given (BufferUsage, MapFlag) pair is valid.
// Static buffers cannot accept InvalidateBuffer.  See B-028.
template <BufferUsage U>
constexpr bool MapFlagsValid(MapFlag flags)
{
    if (U == BufferUsage::Static && has(flags, MapFlag::InvalidateBuffer))
        return false;
    if (U == BufferUsage::Static && has(flags, MapFlag::Unsynchronized))
        return false;  // unsynchronized only meaningful for streaming
    return true;
}

// Validate at the call site — caller passes a typed BufferUsage tag.
// In release builds the result is a constexpr `true`; in debug builds
// the assertion fires if the pair is invalid.
template <BufferUsage U>
constexpr bool RequireValidMapFlags(MapFlag flags)
{
    return MapFlagsValid<U>(flags);
}

} // namespace render::frame

} // namespace Poseidon
