// test_legacy_logging.cpp - Tests for LOG_DEBUG and LOG_ERROR macros

#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Foundation/Framework/Log.hpp>

TEST_CASE("LOG_DEBUG compiles and runs with printf-style format", "[logging]")
{
    LOG_DEBUG(Core, "test message");
    LOG_DEBUG(Core, "value={} name={}", 42, "hello");
    REQUIRE(true);
}

TEST_CASE("LOG_ERROR compiles and runs with printf-style format", "[logging]")
{
    LOG_ERROR(Core, "error: code={}", 404);
    LOG_ERROR(Core, "error: {} at line {}", "null pointer", 123);
    REQUIRE(true);
}
