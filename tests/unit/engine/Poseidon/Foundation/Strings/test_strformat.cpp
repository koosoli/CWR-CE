#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Foundation/Strings/StrFormat.hpp>
#include <string.h>
#include <Poseidon/Foundation/Strings/RString.hpp>

TEST_CASE("FormatNumber", "[utility][strFormat]")
{
    SECTION("positive number")
    {
        RString result = FormatNumber(42);
        REQUIRE(strcmp((const char*)result, "42") == 0);
    }

    SECTION("zero")
    {
        RString result = FormatNumber(0);
        REQUIRE(strcmp((const char*)result, "0") == 0);
    }

    SECTION("negative number")
    {
        RString result = FormatNumber(-123);
        REQUIRE(strcmp((const char*)result, "-123") == 0);
    }
}

TEST_CASE("Format", "[utility][strFormat]")
{
    SECTION("basic formatting")
    {
        RString result = Format("hello %d", 42);
        REQUIRE(strcmp((const char*)result, "hello 42") == 0);
    }

    SECTION("string and float")
    {
        RString result = Format("%s: %.1f", "val", 3.5);
        REQUIRE(strcmp((const char*)result, "val: 3.5") == 0);
    }
}
