#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Foundation/Math/Statistics.hpp>

TEST_CASE("StatisticsByName - compile and construct", "[utility][statistics]")
{
    StatisticsByName stats;
    stats.Count("test_item", 5);
    stats.Count("test_item", 3);
    stats.Count("other_item", 1);
    REQUIRE(stats.NSamples() == 0);
    stats.Sample(1);
    REQUIRE(stats.NSamples() == 1);
    SUCCEED();
}
