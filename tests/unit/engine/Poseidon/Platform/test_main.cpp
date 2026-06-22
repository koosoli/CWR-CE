#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Core/Version.hpp>

TEST_CASE("version helpers: dotted version parses to integer", "[platform][main]")
{
    REQUIRE(Poseidon::VersionToInt("3.00") == 3000);
}
