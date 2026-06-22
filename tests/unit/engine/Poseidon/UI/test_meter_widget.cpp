#include <Poseidon/UI/Options/MeterWidget.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace Poseidon;
TEST_CASE("MeterWidget rises toward the current target and updates peak", "[UI][MeterWidget]")
{
    float level = 0.0f;
    float peak = 0.0f;

    AdvanceMeterBar(1.0f, level, peak);

    CHECK(level == Catch::Approx(25.0f));
    CHECK(peak == Catch::Approx(25.0f));

    AdvanceMeterBar(1.0f, level, peak);

    CHECK(level == Catch::Approx(43.75f));
    CHECK(peak == Catch::Approx(43.75f));
}

TEST_CASE("MeterWidget decays peak and clamps invalid targets", "[UI][MeterWidget]")
{
    float level = 120.0f;
    float peak = 120.0f;

    AdvanceMeterBar(-1.0f, level, peak);

    CHECK(level == Catch::Approx(90.0f));
    CHECK(peak == Catch::Approx(119.4f));

    level = -5.0f;
    peak = 0.3f;
    AdvanceMeterBar(2.0f, level, peak);

    CHECK(level == Catch::Approx(21.25f));
    CHECK(peak == Catch::Approx(21.25f));
}
