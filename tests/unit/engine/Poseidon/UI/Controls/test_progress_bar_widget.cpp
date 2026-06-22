#include <Poseidon/UI/Controls/ProgressBarWidget.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <string>

using Catch::Matchers::WithinAbs;
using Poseidon::FormatProgressPercent;
using Poseidon::ProgressBarLayout;
using Poseidon::ProgressFillGeom;
using Poseidon::ProgressFillRect;

TEST_CASE("ProgressFillRect scales width by fraction, passes the rest through", "[progress][widget]")
{
    ProgressBarLayout layout{0.40f, 0.30f, 0.50f, 0.018f};

    SECTION("empty")
    {
        ProgressFillGeom g = ProgressFillRect(layout, 0.0f);
        CHECK_THAT(g.w, WithinAbs(0.0f, 1e-6f));
        CHECK_THAT(g.x, WithinAbs(0.40f, 1e-6f));
        CHECK_THAT(g.y, WithinAbs(0.30f, 1e-6f));
        CHECK_THAT(g.h, WithinAbs(0.018f, 1e-6f));
    }
    SECTION("half")
    {
        CHECK_THAT(ProgressFillRect(layout, 0.5f).w, WithinAbs(0.25f, 1e-6f));
    }
    SECTION("full")
    {
        CHECK_THAT(ProgressFillRect(layout, 1.0f).w, WithinAbs(0.50f, 1e-6f));
    }
}

TEST_CASE("ProgressFillRect clamps out-of-range fractions to the track", "[progress][widget]")
{
    ProgressBarLayout layout{0.0f, 0.0f, 0.80f, 0.02f};
    // Negative (e.g. an uninitialised fraction) never paints left of the track.
    CHECK_THAT(ProgressFillRect(layout, -0.5f).w, WithinAbs(0.0f, 1e-6f));
    // >1 (e.g. a byte count that overshoots a stale total) never overflows it.
    CHECK_THAT(ProgressFillRect(layout, 1.7f).w, WithinAbs(0.80f, 1e-6f));
}

TEST_CASE("FormatProgressPercent rounds to whole percents and clamps", "[progress][widget]")
{
    CHECK(FormatProgressPercent(0.0f) == "0%");
    CHECK(FormatProgressPercent(0.5f) == "50%");
    CHECK(FormatProgressPercent(1.0f) == "100%");
    CHECK(FormatProgressPercent(0.426f) == "43%"); // rounds, not truncates
    CHECK(FormatProgressPercent(-0.1f) == "0%");
    CHECK(FormatProgressPercent(2.0f) == "100%");
}
