#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

// World.hpp owns DECL_ENUM(TitEffectName); TitEffects.hpp defines its values
// and the Create* signatures, so it must be included after the declaration —
// every other includer pulls World.hpp transitively, this standalone test did not.
#include <Poseidon/World/World.hpp>
#include <Poseidon/Game/TitEffects.hpp>
#include <string.h>
#include <Poseidon/Foundation/Containers/Array.hpp>
#include <Poseidon/Foundation/Strings/RString.hpp>

// Cutscene / in-game subtitle line splitting.
//
// Subtitles come from CfgSounds `titles[]`; DynSound feeds the text to
// CreateTitleEffect(TitPlainDown, ...) -> TitleEffectBasic::Init, which splits
// it into one drawn line per entry via SplitTitleLines. The line separator is
// the literal two-byte marker "\n" (0x5C 0x6E), NOT a raw newline (0x0A).
//
// The #29 bug was stringtable cells carrying a raw 0x0A: SplitTitleLines does
// not treat it as a break, so the whole cue stayed on one line and the 0x0A
// rendered as a missing-glyph square. The fix normalised the data to the "\n"
// marker (porting scripts) and the linter now flags raw 0x0A in non-HTML cells.
//
// In C++ source: "A\\nB" is the marker (backslash + n); "A\nB" is a raw 0x0A.

TEST_CASE("SplitTitleLines breaks subtitle text on the \\n marker", "[game][subtitle][titles]")
{
    SECTION("two lines")
    {
        AutoArray<RString> lines;
        SplitTitleLines("SUB LINE ONE\\nSUB LINE TWO", lines);
        REQUIRE(lines.Size() == 2);
        REQUIRE(strcmp(lines[0], "SUB LINE ONE") == 0);
        REQUIRE(strcmp(lines[1], "SUB LINE TWO") == 0);
    }

    SECTION("three lines")
    {
        AutoArray<RString> lines;
        SplitTitleLines("A\\nB\\nC", lines);
        REQUIRE(lines.Size() == 3);
        REQUIRE(strcmp(lines[0], "A") == 0);
        REQUIRE(strcmp(lines[1], "B") == 0);
        REQUIRE(strcmp(lines[2], "C") == 0);
    }

    SECTION("no marker stays a single line")
    {
        AutoArray<RString> lines;
        SplitTitleLines("ONE LONG LINE", lines);
        REQUIRE(lines.Size() == 1);
        REQUIRE(strcmp(lines[0], "ONE LONG LINE") == 0);
    }

    SECTION("a raw newline (0x0A) is NOT a separator")
    {
        // Broken state: a stringtable cell with a raw newline does not split,
        // so the cue stays on one line (the 0x0A then draws as a square in game).
        // The fix is to store the "\n" marker instead.
        AutoArray<RString> lines;
        SplitTitleLines("SUB LINE ONE\nSUB LINE TWO", lines);
        REQUIRE(lines.Size() == 1);
    }

    SECTION("empty text yields no lines")
    {
        AutoArray<RString> lines;
        SplitTitleLines("", lines);
        REQUIRE(lines.Size() == 0);
    }
}

// Subtitle drop-shadow offset. TitleEffectBasic::DrawText draws the
// black shadow at (x+offsetX, top+offsetY). The offset must drop an equal
// number of *pixels* in X and Y so the shadow is a clean diagonal, not a
// sideways smear.
//
// Broken state: a flat normalized offset (0.003 on both axes) gives
// offsetX*width != offsetY*height on any non-square viewport — on 16:9 the
// shadow lands 0.003*1920=5.76px right but only 0.003*1080=3.24px down (1.78x
// skew), doubling each glyph over the map. SubtitleShadowOffset scales the
// horizontal component by height/width so both products match, and trims the
// drop to 0.002 (the broken value was 0.003).
TEST_CASE("SubtitleShadowOffset drops equal pixels in X and Y", "[game][subtitle][titles][shadow]")
{
    SECTION("16:9 — horizontal offset is aspect-corrected to an equal pixel drop")
    {
        TitleShadowOffset s = SubtitleShadowOffset(1920, 1080);
        REQUIRE(s.x * 1920.0f == Catch::Approx(s.y * 1080.0f));
        REQUIRE(s.x < s.y); // wider than tall → smaller normalized X
    }

    SECTION("4:3 — same equal-pixel-drop invariant")
    {
        TitleShadowOffset s = SubtitleShadowOffset(1024, 768);
        REQUIRE(s.x * 1024.0f == Catch::Approx(s.y * 768.0f));
    }

    SECTION("square viewport — X and Y offsets match")
    {
        TitleShadowOffset s = SubtitleShadowOffset(1000, 1000);
        REQUIRE(s.x == Catch::Approx(s.y));
    }

    SECTION("vertical drop is the tightened 0.002 (was 0.003)")
    {
        TitleShadowOffset s = SubtitleShadowOffset(1920, 1080);
        REQUIRE(s.y == Catch::Approx(0.002f));
    }

    SECTION("degenerate zero width does not divide by zero")
    {
        TitleShadowOffset s = SubtitleShadowOffset(0, 1080);
        REQUIRE(s.x == Catch::Approx(s.y));
    }
}
