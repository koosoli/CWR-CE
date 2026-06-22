#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <Poseidon/Foundation/Math/Fixed.hpp>
#include <climits>

TEST_CASE("Fixed point math", "[utility][fixed]")
{
    SECTION("integer construction")
    {
        Fixed f(3);
        REQUIRE(fxToInt(f) == 3);
        REQUIRE(fxToFloat(f) == Catch::Approx(3.0f));
    }

    SECTION("float construction")
    {
        Fixed f(1.5f);
        REQUIRE(fxToFloat(f) == Catch::Approx(1.5f));
    }

    SECTION("addition")
    {
        Fixed a(2), b(3);
        Fixed c = a + b;
        REQUIRE(fxToInt(c) == 5);
    }

    SECTION("subtraction")
    {
        Fixed a(5), b(3);
        Fixed c = a - b;
        REQUIRE(fxToInt(c) == 2);
    }

    SECTION("multiplication")
    {
        Fixed a(3), b(4);
        Fixed c = a * b;
        REQUIRE(fxToInt(c) == 12);
    }

    SECTION("comparison")
    {
        Fixed a(1), b(2);
        REQUIRE(a < b);
        REQUIRE(b > a);
        REQUIRE(a != b);
        REQUIRE(a == Fixed(1));
    }

    SECTION("negation")
    {
        Fixed a(5);
        Fixed b = -a;
        REQUIRE(fxToInt(b) == -5);
    }

    // Two's-complement wrapping must be well-defined (not UB). Without unsigned
    // arithmetic these would fire UBSan "signed integer overflow".
    // Broken-state delta: UBSan terminates; fixed: wraps as two's complement.
    SECTION("operator+ wraps on overflow — no UBSan")
    {
        Fixed a(Fixed::Fx, INT_MAX);
        Fixed b(Fixed::Fx, 100);
        Fixed c = a + b;
        REQUIRE(fxIntRaw(c) == INT_MIN + 99);
    }

    SECTION("operator+= wraps on overflow — no UBSan")
    {
        Fixed a(Fixed::Fx, INT_MAX);
        a += Fixed(Fixed::Fx, 1);
        REQUIRE(fxIntRaw(a) == INT_MIN);
    }

    SECTION("operator- wraps on overflow — no UBSan")
    {
        Fixed a(Fixed::Fx, INT_MIN);
        Fixed b(Fixed::Fx, 1);
        Fixed c = a - b;
        REQUIRE(fxIntRaw(c) == INT_MAX);
    }

    SECTION("unary minus wraps INT_MIN — no UBSan")
    {
        Fixed a(Fixed::Fx, INT_MIN);
        Fixed b = -a;
        REQUIRE(fxIntRaw(b) == INT_MIN);
    }

    SECTION("negative integer constructor — no UBSan")
    {
        Fixed a(-32768);
        REQUIRE(fxToInt(a) == -32768);
    }

    SECTION("operator<< on negative value — no UBSan")
    {
        Fixed a(Fixed::Fx, -1);
        Fixed b = a << 4;
        REQUIRE(fxIntRaw(b) == -16);
    }
}
