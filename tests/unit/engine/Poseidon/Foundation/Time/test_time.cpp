#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <Poseidon/Foundation/Time/Time.hpp>

using Poseidon::Foundation::Time;
using Poseidon::Foundation::UITime;

TEST_CASE("Time class basics", "[utility][time]")
{
    SECTION("default constructor is zero")
    {
        Time t;
        REQUIRE(t.toInt() == 0);
        REQUIRE(t.toFloat() == Catch::Approx(0.0f));
    }

    SECTION("explicit int constructor")
    {
        Time t(5000);
        REQUIRE(t.toInt() == 5000);
        REQUIRE(t.toFloat() == Catch::Approx(5.0f));
    }

    SECTION("MsFrac")
    {
        Time t(5123);
        REQUIRE(t.MsFrac() == 123);
    }

    SECTION("Floor strips milliseconds")
    {
        Time t(5123);
        Time floored = t.Floor();
        REQUIRE(floored.toInt() == 5000);
    }

    SECTION("AddMs")
    {
        Time t(1000);
        Time t2 = t.AddMs(500);
        REQUIRE(t2.toInt() == 1500);
    }

    SECTION("comparison operators")
    {
        Time a(1000), b(2000);
        REQUIRE(a < b);
        REQUIRE(b > a);
        REQUIRE(a <= b);
        REQUIRE(b >= a);
        REQUIRE(a != b);
        REQUIRE(a == Time(1000));
    }
}

TEST_CASE("UITime class basics", "[utility][time]")
{
    SECTION("default constructor is zero")
    {
        UITime t;
        REQUIRE(t.toInt() == 0);
    }

    SECTION("explicit int constructor")
    {
        UITime t(3000);
        REQUIRE(t.toInt() == 3000);
        REQUIRE(t.toFloat() == Catch::Approx(3.0f));
    }

    SECTION("comparison operators")
    {
        UITime a(1000), b(2000);
        REQUIRE(a < b);
        REQUIRE(a != b);
    }
}
