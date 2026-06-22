#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Audio/Voice/VonBuffer.hpp>
#include <vector>
#include <cstring>
#include <numeric>
#include <stdint.h>

using namespace Poseidon;
static constexpr int FRAME = 320;

static void fillFrame(int16_t* buf, int16_t value)
{
    for (int i = 0; i < FRAME; ++i)
        buf[i] = value;
}

TEST_CASE("VoNJitterBuffer empty pull returns 0", "[VoN][buffer]")
{
    VoNJitterBuffer jb(4, FRAME);
    REQUIRE(jb.empty());
    int16_t out[FRAME];
    REQUIRE(jb.pull(out, FRAME) == 0); // not started
}

TEST_CASE("VoNJitterBuffer in-order push/pull", "[VoN][buffer]")
{
    VoNJitterBuffer jb(4, FRAME);

    int16_t f0[FRAME], f1[FRAME], f2[FRAME];
    fillFrame(f0, 100);
    fillFrame(f1, 200);
    fillFrame(f2, 300);

    jb.push(0, f0, FRAME);
    jb.push(FRAME, f1, FRAME);
    jb.push(FRAME * 2, f2, FRAME);
    REQUIRE(jb.buffered() == 3);

    int16_t out[FRAME];
    REQUIRE(jb.pull(out, FRAME) == FRAME);
    REQUIRE(out[0] == 100);

    REQUIRE(jb.pull(out, FRAME) == FRAME);
    REQUIRE(out[0] == 200);

    REQUIRE(jb.pull(out, FRAME) == FRAME);
    REQUIRE(out[0] == 300);
}

TEST_CASE("VoNJitterBuffer out-of-order reordering", "[VoN][buffer]")
{
    VoNJitterBuffer jb(4, FRAME);

    int16_t f0[FRAME], f1[FRAME], f2[FRAME];
    fillFrame(f0, 10);
    fillFrame(f1, 20);
    fillFrame(f2, 30);

    // Push out of order: frame2 first, then frame0, then frame1
    jb.push(FRAME * 2, f2, FRAME);
    jb.push(0, f0, FRAME);
    jb.push(FRAME, f1, FRAME);

    int16_t out[FRAME];
    REQUIRE(jb.pull(out, FRAME) == FRAME);
    REQUIRE(out[0] == 10); // frame 0

    REQUIRE(jb.pull(out, FRAME) == FRAME);
    REQUIRE(out[0] == 20); // frame 1

    REQUIRE(jb.pull(out, FRAME) == FRAME);
    REQUIRE(out[0] == 30); // frame 2
}

TEST_CASE("VoNJitterBuffer duplicate rejected", "[VoN][buffer]")
{
    VoNJitterBuffer jb(4, FRAME);

    int16_t f0[FRAME];
    fillFrame(f0, 42);

    jb.push(0, f0, FRAME);
    jb.push(0, f0, FRAME); // duplicate
    REQUIRE(jb.buffered() == 1);

    int16_t out[FRAME];
    REQUIRE(jb.pull(out, FRAME) == FRAME);
    REQUIRE(out[0] == 42);
}

TEST_CASE("VoNJitterBuffer gap inserts silence", "[VoN][buffer]")
{
    VoNJitterBuffer jb(8, FRAME);

    int16_t f0[FRAME], f2[FRAME];
    fillFrame(f0, 100);
    fillFrame(f2, 300);

    // Push frame 0 and frame 2 (skip frame 1)
    jb.push(0, f0, FRAME);
    jb.push(FRAME * 2, f2, FRAME);

    int16_t out[FRAME];
    REQUIRE(jb.pull(out, FRAME) == FRAME);
    REQUIRE(out[0] == 100); // frame 0

    REQUIRE(jb.pull(out, FRAME) == FRAME);
    REQUIRE(out[0] == 0); // frame 1 missing → silence

    REQUIRE(jb.pull(out, FRAME) == FRAME);
    REQUIRE(out[0] == 300); // frame 2
}

TEST_CASE("VoNJitterBuffer too-old packet discarded", "[VoN][buffer]")
{
    VoNJitterBuffer jb(4, FRAME);

    int16_t f[FRAME];
    fillFrame(f, 10);

    // Push and pull 5 frames to advance _nextOrigin well past capacity
    for (int i = 0; i < 5; ++i)
    {
        jb.push(FRAME * i, f, FRAME);
        int16_t out[FRAME];
        jb.pull(out, FRAME);
    }
    // _nextOrigin is now FRAME*5

    // Push origin=0, which is 5 frames behind (>= capacity of 4)
    jb.push(0, f, FRAME);
    REQUIRE(jb.buffered() == 0); // rejected as too old
}

TEST_CASE("VoNJitterBuffer overflow discarded", "[VoN][buffer]")
{
    VoNJitterBuffer jb(4, FRAME);

    int16_t f[FRAME];
    fillFrame(f, 1);

    jb.push(0, f, FRAME); // sets _nextOrigin = 0
    // Push frame way beyond capacity (slot 10, capacity 4)
    jb.push(FRAME * 10, f, FRAME);
    REQUIRE(jb.buffered() == 1); // only frame 0
}

// audio-invariants A-31 — every gap-fill emitted by pull() increments
// underrunGapFrames so callers / load regressions can observe network-
// loss events without screen-scraping logs.  reset() rewinds the
// counter.
TEST_CASE("VoNJitterBuffer underrun counter tracks gap-fill events", "[VoN][buffer][A-31]")
{
    VoNJitterBuffer jb(8, FRAME);
    REQUIRE(jb.underrunGapFrames() == 0);

    int16_t f0[FRAME];
    int16_t f3[FRAME];
    fillFrame(f0, 11);
    fillFrame(f3, 33);
    // Push frame 0 and frame 3 (skip 1 + 2).
    jb.push(0, f0, FRAME);
    jb.push(FRAME * 3, f3, FRAME);

    int16_t out[FRAME];
    // Frame 0 — real data, no underrun.
    REQUIRE(jb.pull(out, FRAME) == FRAME);
    CHECK(out[0] == 11);
    CHECK(jb.underrunGapFrames() == 0);

    // Frame 1 — gap, counter ticks.
    REQUIRE(jb.pull(out, FRAME) == FRAME);
    CHECK(out[0] == 0);
    CHECK(jb.underrunGapFrames() == 1);

    // Frame 2 — another gap.
    REQUIRE(jb.pull(out, FRAME) == FRAME);
    CHECK(out[0] == 0);
    CHECK(jb.underrunGapFrames() == 2);

    // Frame 3 — real data, counter does NOT increment.
    REQUIRE(jb.pull(out, FRAME) == FRAME);
    CHECK(out[0] == 33);
    CHECK(jb.underrunGapFrames() == 2);

    // reset() rewinds the counter (it's a session-cumulative probe
    // tied to the buffer's stream lifetime).
    jb.reset();
    CHECK(jb.underrunGapFrames() == 0);
}

TEST_CASE("VoNJitterBuffer reset", "[VoN][buffer]")
{
    VoNJitterBuffer jb(4, FRAME);

    int16_t f[FRAME];
    fillFrame(f, 99);
    jb.push(0, f, FRAME);
    REQUIRE(jb.buffered() == 1);

    jb.reset();
    REQUIRE(jb.empty());

    int16_t out[FRAME];
    REQUIRE(jb.pull(out, FRAME) == 0); // not started after reset
}
