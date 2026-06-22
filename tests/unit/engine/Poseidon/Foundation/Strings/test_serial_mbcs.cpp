#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Security/Serial.hpp>
#include <Poseidon/Foundation/Strings/Mbcs.hpp>

TEST_CASE("serial and mbcs headers compile", "[utility][serial][mbcs]")
{
    [[maybe_unused]] Poseidon::CDKey key;
    SUCCEED();
}
