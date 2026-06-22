#include <catch2/catch_test_macros.hpp>

#include <Poseidon/Graphics/Rendering/Frame/Buffers.hpp>

// Phase E.1 — pin the buffer-usage / map-flag compatibility contract
// (invariant I-21, catalog B-028).
//
// Historical bug: NVIDIA demoted GL_STATIC_DRAW buffers to host memory
// when mapped with GL_MAP_INVALIDATE_BUFFER_BIT, spamming perf
// warnings.  Each new buffer write site had to remember this — the
// previous fix (1632048e) was at one specific call site.
//
// The typed wrappers make the mismatch unrepresentable: callers pass a typed
// BufferUsage tag to `RequireValidMapFlags<U>(flags)`, which is
// false for the invalid combinations.  Tests verify the matrix
// of (usage, flags) outcomes.

namespace v2 = Poseidon::render::frame;

TEST_CASE("Frame/I-21: Dynamic buffers accept InvalidateBuffer", "[render-frame][invariants][I-21][buffers]")
{
    constexpr bool ok = Poseidon::render::frame::MapFlagsValid<Poseidon::render::frame::BufferUsage::Dynamic>(
        Poseidon::render::frame::MapFlag::Write | Poseidon::render::frame::MapFlag::InvalidateBuffer);
    static_assert(ok, "Dynamic + InvalidateBuffer must be allowed");
    REQUIRE(ok);
}

TEST_CASE("Frame/I-21: Static buffers reject InvalidateBuffer", "[render-frame][invariants][I-21][buffers]")
{
    // The exact NVIDIA-demote class of bug.
    constexpr bool bad = Poseidon::render::frame::MapFlagsValid<Poseidon::render::frame::BufferUsage::Static>(
        Poseidon::render::frame::MapFlag::Write | Poseidon::render::frame::MapFlag::InvalidateBuffer);
    static_assert(!bad, "Static + InvalidateBuffer must be rejected");
    REQUIRE_FALSE(bad);
}

TEST_CASE("Frame/I-21: Static buffers reject Unsynchronized", "[render-frame][invariants][I-21][buffers]")
{
    // Unsynchronized only makes sense for streaming dynamic buffers.
    constexpr bool bad = Poseidon::render::frame::MapFlagsValid<Poseidon::render::frame::BufferUsage::Static>(
        Poseidon::render::frame::MapFlag::Write | Poseidon::render::frame::MapFlag::Unsynchronized);
    static_assert(!bad);
    REQUIRE_FALSE(bad);
}

TEST_CASE("Frame/I-21: Static buffers accept plain Read or Write", "[render-frame][invariants][I-21][buffers]")
{
    constexpr bool wr = Poseidon::render::frame::MapFlagsValid<Poseidon::render::frame::BufferUsage::Static>(
        Poseidon::render::frame::MapFlag::Write);
    constexpr bool rd = Poseidon::render::frame::MapFlagsValid<Poseidon::render::frame::BufferUsage::Static>(
        Poseidon::render::frame::MapFlag::Read);
    static_assert(wr);
    static_assert(rd);
    REQUIRE(wr);
    REQUIRE(rd);
}

TEST_CASE("Frame/I-21: Dynamic buffers accept all valid flag combinations", "[render-frame][invariants][I-21][buffers]")
{
    constexpr bool a = Poseidon::render::frame::MapFlagsValid<Poseidon::render::frame::BufferUsage::Dynamic>(
        Poseidon::render::frame::MapFlag::Write);
    constexpr bool b = Poseidon::render::frame::MapFlagsValid<Poseidon::render::frame::BufferUsage::Dynamic>(
        Poseidon::render::frame::MapFlag::Write | Poseidon::render::frame::MapFlag::InvalidateBuffer);
    constexpr bool c = Poseidon::render::frame::MapFlagsValid<Poseidon::render::frame::BufferUsage::Dynamic>(
        Poseidon::render::frame::MapFlag::Write | Poseidon::render::frame::MapFlag::Unsynchronized);
    constexpr bool d = Poseidon::render::frame::MapFlagsValid<Poseidon::render::frame::BufferUsage::Dynamic>(
        Poseidon::render::frame::MapFlag::Write | Poseidon::render::frame::MapFlag::InvalidateBuffer |
        Poseidon::render::frame::MapFlag::Unsynchronized);
    static_assert(a);
    static_assert(b);
    static_assert(c);
    static_assert(d);
    REQUIRE(a);
    REQUIRE(b);
    REQUIRE(c);
    REQUIRE(d);
}

TEST_CASE("Frame/I-21: MapFlag bitwise-or composes flags correctly", "[render-frame][invariants][I-21][buffers]")
{
    constexpr auto combined =
        Poseidon::render::frame::MapFlag::Write | Poseidon::render::frame::MapFlag::InvalidateBuffer;
    static_assert(Poseidon::render::frame::has(combined, Poseidon::render::frame::MapFlag::Write));
    static_assert(Poseidon::render::frame::has(combined, Poseidon::render::frame::MapFlag::InvalidateBuffer));
    static_assert(!Poseidon::render::frame::has(combined, Poseidon::render::frame::MapFlag::Read));
    REQUIRE(Poseidon::render::frame::has(combined, Poseidon::render::frame::MapFlag::Write));
    REQUIRE(Poseidon::render::frame::has(combined, Poseidon::render::frame::MapFlag::InvalidateBuffer));
    REQUIRE_FALSE(Poseidon::render::frame::has(combined, Poseidon::render::frame::MapFlag::Read));
}
