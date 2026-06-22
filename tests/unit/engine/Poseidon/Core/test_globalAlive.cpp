#include <catch2/catch_test_macros.hpp>

#include <Poseidon/Foundation/Framework/GlobalAlive.hpp>

TEST_CASE("GlobalAlive - Basic smoke test", "[core][progress][globalalive]")
{
    Poseidon::Foundation::GlobalAlive();
    Poseidon::Foundation::GlobalAlive();
    Poseidon::Foundation::GlobalAlive();

    REQUIRE(true);
}
