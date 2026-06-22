// Unit tests for floating point optimization functions from Poseidon/Foundation/Common/FltOpts.hpp

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <Poseidon/Foundation/Framework/DebugLog.hpp> // For Assert macro
#include <Poseidon/Foundation/Common/FltOpts.hpp>
#include <limits>
#include <cmath>
#include <catch2/catch_message.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <utility>

TEST_CASE("floatMax returns maximum of two floats", "[fltopts]")
{
    SECTION("Positive numbers")
    {
        REQUIRE(floatMax(5.0f, 3.0f) == 5.0f);
        REQUIRE(floatMax(3.0f, 5.0f) == 5.0f);
    }

    SECTION("Negative numbers")
    {
        REQUIRE(floatMax(-5.0f, -3.0f) == -3.0f);
        REQUIRE(floatMax(-3.0f, -5.0f) == -3.0f);
    }

    SECTION("Mixed signs")
    {
        REQUIRE(floatMax(-5.0f, 3.0f) == 3.0f);
        REQUIRE(floatMax(5.0f, -3.0f) == 5.0f);
    }

    SECTION("Equal values")
    {
        REQUIRE(floatMax(5.0f, 5.0f) == 5.0f);
    }

    SECTION("Zero")
    {
        REQUIRE(floatMax(0.0f, 5.0f) == 5.0f);
        REQUIRE(floatMax(-5.0f, 0.0f) == 0.0f);
    }
}

TEST_CASE("floatMin returns minimum of two floats", "[fltopts]")
{
    SECTION("Positive numbers")
    {
        REQUIRE(floatMin(5.0f, 3.0f) == 3.0f);
        REQUIRE(floatMin(3.0f, 5.0f) == 3.0f);
    }

    SECTION("Negative numbers")
    {
        REQUIRE(floatMin(-5.0f, -3.0f) == -5.0f);
        REQUIRE(floatMin(-3.0f, -5.0f) == -5.0f);
    }

    SECTION("Mixed signs")
    {
        REQUIRE(floatMin(-5.0f, 3.0f) == -5.0f);
        REQUIRE(floatMin(5.0f, -3.0f) == -3.0f);
    }

    SECTION("Equal values")
    {
        REQUIRE(floatMin(5.0f, 5.0f) == 5.0f);
    }

    SECTION("Zero")
    {
        REQUIRE(floatMin(0.0f, 5.0f) == 0.0f);
        REQUIRE(floatMin(-5.0f, 0.0f) == -5.0f);
    }
}

TEST_CASE("saturateMin clamps value to maximum", "[fltopts]")
{
    SECTION("Float saturation")
    {
        float val = 10.0f;
        saturateMin(val, 5.0f);
        REQUIRE(val == 5.0f);

        val = 3.0f;
        saturateMin(val, 5.0f);
        REQUIRE(val == 3.0f);
    }

    SECTION("Integer saturation")
    {
        int val = 10;
        saturateMin(val, 5);
        REQUIRE(val == 5);

        val = 3;
        saturateMin(val, 5);
        REQUIRE(val == 3);
    }
}

TEST_CASE("saturateMax clamps value to minimum", "[fltopts]")
{
    SECTION("Float saturation")
    {
        float val = 3.0f;
        saturateMax(val, 5.0f);
        REQUIRE(val == 5.0f);

        val = 10.0f;
        saturateMax(val, 5.0f);
        REQUIRE(val == 10.0f);
    }

    SECTION("Integer saturation")
    {
        int val = 3;
        saturateMax(val, 5);
        REQUIRE(val == 5);

        val = 10;
        saturateMax(val, 5);
        REQUIRE(val == 10);
    }
}

TEST_CASE("saturate clamps value to range", "[fltopts]")
{
    SECTION("Float saturation - below min")
    {
        float val = 1.0f;
        saturate(val, 5.0f, 10.0f);
        REQUIRE(val == 5.0f);
    }

    SECTION("Float saturation - above max")
    {
        float val = 15.0f;
        saturate(val, 5.0f, 10.0f);
        REQUIRE(val == 10.0f);
    }

    SECTION("Float saturation - within range")
    {
        float val = 7.0f;
        saturate(val, 5.0f, 10.0f);
        REQUIRE(val == 7.0f);
    }

    SECTION("Integer saturation - below min")
    {
        int val = 1;
        saturate(val, 5, 10);
        REQUIRE(val == 5);
    }

    SECTION("Integer saturation - above max")
    {
        int val = 15;
        saturate(val, 5, 10);
        REQUIRE(val == 10);
    }

    SECTION("Integer saturation - within range")
    {
        int val = 7;
        saturate(val, 5, 10);
        REQUIRE(val == 7);
    }
}

TEST_CASE("Square returns squared value", "[fltopts]")
{
    SECTION("Positive integers")
    {
        REQUIRE(Square(5) == 25);
        REQUIRE(Square(10) == 100);
    }

    SECTION("Negative integers")
    {
        REQUIRE(Square(-5) == 25);
        REQUIRE(Square(-10) == 100);
    }

    SECTION("Floats")
    {
        REQUIRE_THAT(Square(3.5f), Catch::Matchers::WithinRel(12.25f, 0.0001f));
        REQUIRE_THAT(Square(-2.5f), Catch::Matchers::WithinRel(6.25f, 0.0001f));
    }

    SECTION("Zero")
    {
        REQUIRE(Square(0) == 0);
        REQUIRE(Square(0.0f) == 0.0f);
    }
}

TEST_CASE("fastRound rounds float to nearest integer", "[fltopts][round]")
{
    // The x86 inline-assembly path uses FPU round-to-nearest-even (via
    // Snapper); the portable x64 path must match those semantics so
    // fastFloor stays correct.  An earlier portable implementation
    // returned `(int)x` (truncate-toward-zero), which silently broke
    // `fastFloor(x) = fastRound(x - 0.5)` for positive non-integer
    // inputs.  In Poseidon that surfaced as Czech lip animation
    // terminating ~0.4s into every voice line because `GetPhase`'s
    // `floor` landed one frame behind.

    SECTION("Positive non-integers round to nearest")
    {
        REQUIRE(fastRound(0.4f) == 0.0f);
        REQUIRE(fastRound(0.6f) == 1.0f);
        REQUIRE(fastRound(5.2f) == 5.0f);
        REQUIRE(fastRound(5.8f) == 6.0f);
        REQUIRE(fastRound(8.675f) == 9.0f); // the canonical Czech-lip case
    }

    SECTION("Negative non-integers round to nearest")
    {
        REQUIRE(fastRound(-0.4f) == 0.0f);
        REQUIRE(fastRound(-0.6f) == -1.0f);
        REQUIRE(fastRound(-5.2f) == -5.0f);
        REQUIRE(fastRound(-5.8f) == -6.0f);
    }

    SECTION("Integer inputs are unchanged")
    {
        REQUIRE(fastRound(0.0f) == 0.0f);
        REQUIRE(fastRound(5.0f) == 5.0f);
        REQUIRE(fastRound(-5.0f) == -5.0f);
    }
}

TEST_CASE("fastFloor returns the mathematical floor", "[fltopts][round]")
{
    // `fastFloor(x) = fastRound(x - 0.5)` only behaves like floor when
    // fastRound is round-to-nearest.  Pin the actual floor semantics
    // here so any regression of the round path surfaces immediately.

    SECTION("Positive values floor down")
    {
        REQUIRE(fastFloor(0.1f) == 0.0f);
        REQUIRE(fastFloor(0.5f) == 0.0f);
        REQUIRE(fastFloor(0.9f) == 0.0f);
        REQUIRE(fastFloor(5.2f) == 5.0f);
        REQUIRE(fastFloor(5.9f) == 5.0f);
        // The exact offset/_invFrame value that triggered the Czech
        // lip cleanup at offset=0.367s -> 9.175 -> must floor to 9, not 8.
        REQUIRE(fastFloor(9.175f) == 9.0f);
    }

    SECTION("Negative values floor down (toward -inf)")
    {
        REQUIRE(fastFloor(-0.1f) == -1.0f);
        REQUIRE(fastFloor(-5.2f) == -6.0f);
    }

    // Note: `fastFloor(x) = fastRound(x - 0.5)` is APPROXIMATE around
    // exact integer inputs — `fastFloor(N.0)` reduces to
    // `fastRound(N - 0.5)`, which lands on the .5 tie and depends on
    // rounding mode (banker's rounds to even neighbour).  Callers must
    // not rely on exact-integer boundary behaviour; that's by design
    // and unrelated to the round-to-nearest fix for non-integer inputs.
}

TEST_CASE("fastRound sweep matches std::nearbyint across [-100, 100]", "[fltopts][round][sweep]")
{
    // Walk a dense grid of non-half-tie values and pin every output
    // against `std::nearbyint` so any regression in the rounding mode
    // (truncate, banker's vs half-away-from-zero) surfaces immediately.
    // Step 0.07 avoids landing on .5 ties; those are tested separately.
    int mismatches = 0;
    for (float x = -100.0f; x <= 100.0f; x += 0.07f)
    {
        const float actual = fastRound(x);
        const float expected = std::nearbyint(x);
        if (actual != expected)
        {
            ++mismatches;
            INFO("x=" << x << " fastRound=" << actual << " nearbyint=" << expected);
            CHECK(actual == expected);
        }
    }
    REQUIRE(mismatches == 0);
}

TEST_CASE("fastFloor sweep matches std::floor for non-integer inputs", "[fltopts][round][sweep]")
{
    // Sweep values whose fractional part is bounded away from {0, 0.5, 1}
    // so neither the `x - 0.5` formula nor the .5 tie tripping breaks
    // the comparison.  Catches the original truncate-vs-round bug
    // (fastFloor(N + small_fraction) returning N-1 instead of N).
    int mismatches = 0;
    for (float x = -50.0f; x <= 50.0f; x += 0.13f)
    {
        const float actual = fastFloor(x);
        const float expected = std::floor(x);
        if (actual != expected)
        {
            ++mismatches;
            INFO("x=" << x << " fastFloor=" << actual << " std::floor=" << expected);
            CHECK(actual == expected);
        }
    }
    REQUIRE(mismatches == 0);
}

TEST_CASE("fastFloor pinned values from the Czech lip-sync regression", "[fltopts][round][regression]")
{
    // The exact `offset/_invFrame` values produced by `ManLipInfo::GetPhase`
    // for the Czech demo intro `s02v_101.Czech.lip` were:
    //   offset / 0.04 = 0.7..2.5..9.175 across the timeline.
    // The portable x64 fallback returned `(int)x` from `fastRound`, which
    // dropped these to `floor(x)-1` for any positive input with fractional
    // part < 0.5 — producing the wrong window start and ultimately
    // `lipPhase = -0.125` mid-stream.

    // Bug-triggering case from the actual user-shared log:
    REQUIRE(fastFloor(9.175f) == 9.0f);
    // Other positive fractional-part-<0.5 inputs that previously broke:
    REQUIRE(fastFloor(1.1f) == 1.0f);
    REQUIRE(fastFloor(1.2f) == 1.0f);
    REQUIRE(fastFloor(1.4f) == 1.0f);
    REQUIRE(fastFloor(8.1f) == 8.0f);
    REQUIRE(fastFloor(8.4f) == 8.0f);
    REQUIRE(fastFloor(25.3f) == 25.0f);
    // Positive fractional-part->0.5 inputs (these worked even with the
    // truncate-bug — pin them to catch any swing in the other direction):
    REQUIRE(fastFloor(1.6f) == 1.0f);
    REQUIRE(fastFloor(1.9f) == 1.0f);
    REQUIRE(fastFloor(9.7f) == 9.0f);
}

TEST_CASE("toInt converts float to int with rounding", "[fltopts]")
{
    SECTION("Positive values")
    {
        REQUIRE(toInt(5.2f) == 5);
        REQUIRE(toInt(5.8f) == 6);
    }

    SECTION("Negative values")
    {
        REQUIRE(toInt(-5.2f) == -5);
        REQUIRE(toInt(-5.8f) == -6);
    }

    SECTION("Zero")
    {
        REQUIRE(toInt(0.0f) == 0);
    }

    SECTION("Half values round to nearest-even (banker's)")
    {
        // The x86 FPU default and std::nearbyint both resolve .5 ties to the
        // even neighbour.  These literal args are constant-folded by the
        // compiler; the genuine runtime path is pinned separately below.
        REQUIRE(toInt(0.5f) == 0);
        REQUIRE(toInt(1.5f) == 2);
        REQUIRE(toInt(2.5f) == 2);
        REQUIRE(toInt(5.5f) == 6);
    }

    SECTION("Double to int")
    {
        REQUIRE(toInt(5.6) == 6);
        REQUIRE(toInt(-5.6) == -6);
    }
}

TEST_CASE("toInt rounds exact half ties to nearest-even at runtime", "[fltopts][round][regression]")
{
    // Two compounding bugs made ColorP::B8() of a 0.5 channel return 127
    // instead of 128.  (1) Poseidon::toInt(int) in Core/Types.hpp HID the
    // global toInt(float)/toInt(double) overloads, so an unqualified
    // toInt(<float>) inside namespace Poseidon bound to the int overload and
    // truncated the argument.  (2) toInt(float) itself used the Kaipetsky
    // `*(int*)&fval` bit-trick, which is strict-aliasing UB on clang x64.
    // The fixes: `using ::toInt;` in Types.hpp, and toInt(float) ->
    // (int)std::nearbyint (round-to-nearest-even, the x86/DX8 reference
    // behaviour).  `volatile` defeats constant folding so the genuine runtime
    // path runs; literal-arg calls fold to the right answer and hid the bug.
    // Without the fix toInt(127.5) reports 127.
    volatile float v;
    v = 127.5f;
    REQUIRE(toInt(v) == 128); // the ColorP::B8 case; 128 is the even neighbour
    v = 1.5f;
    REQUIRE(toInt(v) == 2); // odd->even tie rounds up
    v = 3.5f;
    REQUIRE(toInt(v) == 4);
    v = 5.5f;
    REQUIRE(toInt(v) == 6);
    v = 2.5f;
    REQUIRE(toInt(v) == 2); // even neighbour: stays 2, not 3
    v = 0.5f;
    REQUIRE(toInt(v) == 0);
    v = -1.5f;
    REQUIRE(toInt(v) == -2);
    v = -2.5f;
    REQUIRE(toInt(v) == -2); // even neighbour
    // non-tie runtime values are unaffected
    v = 5.2f;
    REQUIRE(toInt(v) == 5);
    v = 5.8f;
    REQUIRE(toInt(v) == 6);
}

TEST_CASE("toLargeInt and to64bInt round to nearest-even matching x86 fistp", "[fltopts][round][regression]")
{
    // The original DX8 32-bit build converted float->int with x87 `fld;fistp`,
    // which rounds per the FPU control word (round-to-nearest-even).  The x64
    // portable path used `(int)f`/`(__int64)f`, truncating toward zero.  Values
    // verified against a 32-bit x87 fistp stub: fistp gives 0.6->1, 1.5->2,
    // 2.5->2, 5.5->6, 127.5->128, -1.5->-2 -- all identical to std::nearbyint.
    // Without the fix toLargeInt(5.5) truncates to 5.
    // `volatile` defeats constant folding so the runtime path is exercised.
    volatile float v;
    v = 0.5f;
    REQUIRE(toLargeInt(v) == 0);
    v = 0.6f;
    REQUIRE(toLargeInt(v) == 1);
    v = 1.5f;
    REQUIRE(toLargeInt(v) == 2);
    v = 2.5f;
    REQUIRE(toLargeInt(v) == 2); // ties-to-even
    v = 3.5f;
    REQUIRE(toLargeInt(v) == 4);
    v = 5.5f;
    REQUIRE(toLargeInt(v) == 6);
    v = 5.8f;
    REQUIRE(toLargeInt(v) == 6);
    v = 127.5f;
    REQUIRE(toLargeInt(v) == 128);
    v = -0.6f;
    REQUIRE(toLargeInt(v) == -1);
    v = -1.5f;
    REQUIRE(toLargeInt(v) == -2);
    v = -2.5f;
    REQUIRE(toLargeInt(v) == -2); // ties-to-even
    v = -5.5f;
    REQUIRE(toLargeInt(v) == -6);

    v = 0.6f;
    REQUIRE(to64bInt(v) == 1);
    v = 1.5f;
    REQUIRE(to64bInt(v) == 2);
    v = 2.5f;
    REQUIRE(to64bInt(v) == 2);
    v = 127.5f;
    REQUIRE(to64bInt(v) == 128);
    v = -1.5f;
    REQUIRE(to64bInt(v) == -2);
}

TEST_CASE("toIntFloor rounds down to nearest int", "[fltopts]")
{
    SECTION("Positive values")
    {
        REQUIRE(toIntFloor(5.2f) == 5);
        REQUIRE(toIntFloor(5.8f) == 5);
    }

    SECTION("Negative values")
    {
        REQUIRE(toIntFloor(-5.2f) == -6);
        REQUIRE(toIntFloor(-5.8f) == -6);
    }

    SECTION("Exact integers")
    {
        // toIntFloor subtracts 0.5 then rounds
        REQUIRE(toIntFloor(5.0f) == 4);   // 5.0 - 0.5 = 4.5 -> rounds to 4
        REQUIRE(toIntFloor(-5.0f) == -6); // -5.0 - 0.5 = -5.5 -> rounds to -6
    }
}

TEST_CASE("toIntCeil rounds up to nearest int", "[fltopts]")
{
    SECTION("Positive values")
    {
        REQUIRE(toIntCeil(5.2f) == 6);
        REQUIRE(toIntCeil(5.8f) == 6);
    }

    SECTION("Negative values")
    {
        REQUIRE(toIntCeil(-5.2f) == -5);
        REQUIRE(toIntCeil(-5.8f) == -5);
    }

    SECTION("Exact integers")
    {
        // toIntCeil adds 0.5 then rounds
        REQUIRE(toIntCeil(5.0f) == 6);   // 5.0 + 0.5 = 5.5 -> rounds to 6
        REQUIRE(toIntCeil(-5.0f) == -4); // -5.0 + 0.5 = -4.5 -> rounds to -4
    }
}

TEST_CASE("myLower converts character to lowercase", "[fltopts]")
{
    SECTION("Uppercase letters")
    {
        REQUIRE(myLower('A') == 'a');
        REQUIRE(myLower('Z') == 'z');
        REQUIRE(myLower('M') == 'm');
    }

    SECTION("Lowercase letters - no change")
    {
        REQUIRE(myLower('a') == 'a');
        REQUIRE(myLower('z') == 'z');
        REQUIRE(myLower('m') == 'm');
    }

    SECTION("Non-letters - no change")
    {
        REQUIRE(myLower('0') == '0');
        REQUIRE(myLower('9') == '9');
        REQUIRE(myLower(' ') == ' ');
        REQUIRE(myLower('!') == '!');
    }
}

TEST_CASE("myUpper converts character to uppercase", "[fltopts]")
{
    SECTION("Lowercase letters")
    {
        REQUIRE(myUpper('a') == 'A');
        REQUIRE(myUpper('z') == 'Z');
        REQUIRE(myUpper('m') == 'M');
    }

    SECTION("Uppercase letters - no change")
    {
        REQUIRE(myUpper('A') == 'A');
        REQUIRE(myUpper('Z') == 'Z');
        REQUIRE(myUpper('M') == 'M');
    }

    SECTION("Non-letters - no change")
    {
        REQUIRE(myUpper('0') == '0');
        REQUIRE(myUpper('9') == '9');
        REQUIRE(myUpper(' ') == ' ');
        REQUIRE(myUpper('!') == '!');
    }
}

TEST_CASE("swap exchanges two values", "[fltopts]")
{
    SECTION("Integer swap")
    {
        int a = 5, b = 10;
        swap(a, b);
        REQUIRE(a == 10);
        REQUIRE(b == 5);
    }

    SECTION("Float swap")
    {
        float a = 3.14f, b = 2.71f;
        swap(a, b);
        REQUIRE(a == 2.71f);
        REQUIRE(b == 3.14f);
    }

    SECTION("Swap with equal values")
    {
        int a = 7, b = 7;
        swap(a, b);
        REQUIRE(a == 7);
        REQUIRE(b == 7);
    }
}

TEST_CASE("FastInv approximates reciprocal", "[fltopts]")
{
    SECTION("Positive values")
    {
        float val = 2.0f;
        float inv = FastInv(val);
        // Should be close to 0.5, with ~6-bit precision
        REQUIRE_THAT(inv, Catch::Matchers::WithinAbs(0.5f, 0.01f));
    }

    SECTION("Small positive values")
    {
        float val = 0.5f;
        float inv = FastInv(val);
        // Should be close to 2.0
        REQUIRE_THAT(inv, Catch::Matchers::WithinAbs(2.0f, 0.02f));
    }

    SECTION("Large values")
    {
        float val = 10.0f;
        float inv = FastInv(val);
        // Should be close to 0.1
        REQUIRE_THAT(inv, Catch::Matchers::WithinAbs(0.1f, 0.01f));
    }

    SECTION("Value near 1")
    {
        float val = 1.0f;
        float inv = FastInv(val);
        // Should be close to 1.0
        REQUIRE_THAT(inv, Catch::Matchers::WithinAbs(1.0f, 0.01f));
    }
}
