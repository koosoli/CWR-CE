#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Poseidon/Foundation/Math/MathOpt.hpp>
#include <Poseidon/Foundation/Math/MathDefs.hpp>
#include <cmath>

// Test optimized math functions defined in mathOpt.hpp
TEST_CASE("HDegree converts degrees to radians", "[math][mathopt][conversion]")
{
    SECTION("0 degrees is 0 radians")
    {
        REQUIRE(HDegree(0.0f) == Catch::Approx(0.0f));
    }

    SECTION("90 degrees is PI/2 radians")
    {
        float result = HDegree(90.0f);
        float expected = H_PI / 2.0f;
        REQUIRE(result == Catch::Approx(expected).epsilon(0.00001f));
    }

    SECTION("180 degrees is PI radians")
    {
        float result = HDegree(180.0f);
        REQUIRE(result == Catch::Approx(H_PI).epsilon(0.00001f));
    }

    SECTION("360 degrees is 2*PI radians")
    {
        float result = HDegree(360.0f);
        float expected = 2.0f * H_PI;
        REQUIRE(result == Catch::Approx(expected).epsilon(0.00001f));
    }

    SECTION("45 degrees is PI/4 radians")
    {
        float result = HDegree(45.0f);
        float expected = H_PI / 4.0f;
        REQUIRE(result == Catch::Approx(expected).epsilon(0.00001f));
    }

    SECTION("Negative angles")
    {
        float result = HDegree(-90.0f);
        float expected = -H_PI / 2.0f;
        REQUIRE(result == Catch::Approx(expected).epsilon(0.00001f));
    }
}

TEST_CASE("fSign returns floating-point sign", "[math][mathopt][sign]")
{
    SECTION("Positive number returns +1")
    {
        REQUIRE(fSign(1.0f) == 1.0f);
        REQUIRE(fSign(0.5f) == 1.0f);
        REQUIRE(fSign(100.0f) == 1.0f);
        REQUIRE(fSign(0.00001f) == 1.0f);
    }

    SECTION("Negative number returns -1")
    {
        REQUIRE(fSign(-1.0f) == -1.0f);
        REQUIRE(fSign(-0.5f) == -1.0f);
        REQUIRE(fSign(-100.0f) == -1.0f);
        REQUIRE(fSign(-0.00001f) == -1.0f);
    }

    SECTION("Zero returns 0")
    {
        REQUIRE(fSign(0.0f) == 0.0f);
    }
}

TEST_CASE("sign returns integer sign", "[math][mathopt][sign]")
{
    SECTION("Positive number returns +1")
    {
        REQUIRE(sign(1.0f) == 1);
        REQUIRE(sign(0.5f) == 1);
        REQUIRE(sign(100.0f) == 1);
        REQUIRE(sign(0.00001f) == 1);
    }

    SECTION("Negative number returns -1")
    {
        REQUIRE(sign(-1.0f) == -1);
        REQUIRE(sign(-0.5f) == -1);
        REQUIRE(sign(-100.0f) == -1);
        REQUIRE(sign(-0.00001f) == -1);
    }

    SECTION("Zero returns 0")
    {
        REQUIRE(sign(0.0f) == 0);
    }
}

TEST_CASE("Inv computes reciprocal", "[math][mathopt][reciprocal]")
{
    SECTION("Reciprocal of 1 is 1")
    {
        REQUIRE(Inv(1.0f) == Catch::Approx(1.0f));
    }

    SECTION("Reciprocal of 2 is 0.5")
    {
        REQUIRE(Inv(2.0f) == Catch::Approx(0.5f));
    }

    SECTION("Reciprocal of 0.5 is 2")
    {
        REQUIRE(Inv(0.5f) == Catch::Approx(2.0f));
    }

    SECTION("Reciprocal of 4 is 0.25")
    {
        REQUIRE(Inv(4.0f) == Catch::Approx(0.25f));
    }

    SECTION("Reciprocal of negative numbers")
    {
        REQUIRE(Inv(-2.0f) == Catch::Approx(-0.5f));
        REQUIRE(Inv(-0.5f) == Catch::Approx(-2.0f));
    }

    SECTION("Reciprocal accuracy compared to standard division")
    {
        float value = 3.14159f;
        float expected = 1.0f / value;
        float result = Inv(value);
        REQUIRE(result == Catch::Approx(expected).epsilon(INV_EPS));
    }
}

TEST_CASE("InvSqrt computes fast inverse square root", "[math][mathopt][invsqrt]")
{
    SECTION("InvSqrt of 1 is 1")
    {
        float result = InvSqrt(1.0f);
        REQUIRE(result == Catch::Approx(1.0f).epsilon(INVSQRT_EPS));
    }

    SECTION("InvSqrt of 4 is 0.5")
    {
        float result = InvSqrt(4.0f);
        REQUIRE(result == Catch::Approx(0.5f).epsilon(INVSQRT_EPS));
    }

    SECTION("InvSqrt of 9 is 1/3")
    {
        float result = InvSqrt(9.0f);
        float expected = 1.0f / 3.0f;
        REQUIRE(result == Catch::Approx(expected).epsilon(INVSQRT_EPS));
    }

    SECTION("InvSqrt of 2 is INVSQRT2")
    {
        float result = InvSqrt(2.0f);
        REQUIRE(result == Catch::Approx(H_INVSQRT2).epsilon(INVSQRT_EPS));
    }

    SECTION("InvSqrt accuracy compared to standard sqrt")
    {
        float values[] = {0.5f, 1.0f, 2.0f, 4.0f, 10.0f, 100.0f};
        for (float value : values)
        {
            float expected = 1.0f / std::sqrt(value);
            float result = InvSqrt(value);
            REQUIRE(result == Catch::Approx(expected).epsilon(INVSQRT_EPS));
        }
    }

    SECTION("InvSqrt for very small numbers")
    {
        float value = 0.01f;
        float expected = 1.0f / std::sqrt(value);
        float result = InvSqrt(value);
        REQUIRE(result == Catch::Approx(expected).epsilon(INVSQRT_EPS));
    }

    SECTION("InvSqrt for very large numbers")
    {
        float value = 10000.0f;
        float expected = 1.0f / std::sqrt(value);
        float result = InvSqrt(value);
        REQUIRE(result == Catch::Approx(expected).epsilon(INVSQRT_EPS));
    }
}

TEST_CASE("Square computes x*x", "[math][mathopt][square]")
{
    SECTION("Square of 0 is 0")
    {
        REQUIRE(Square(0.0f) == 0.0f);
    }

    SECTION("Square of 1 is 1")
    {
        REQUIRE(Square(1.0f) == 1.0f);
    }

    SECTION("Square of 2 is 4")
    {
        REQUIRE(Square(2.0f) == 4.0f);
    }

    SECTION("Square of 3 is 9")
    {
        REQUIRE(Square(3.0f) == 9.0f);
    }

    SECTION("Square of negative numbers is positive")
    {
        REQUIRE(Square(-2.0f) == 4.0f);
        REQUIRE(Square(-3.0f) == 9.0f);
        REQUIRE(Square(-5.0f) == 25.0f);
    }

    SECTION("Square of fractions")
    {
        REQUIRE(Square(0.5f) == Catch::Approx(0.25f));
        REQUIRE(Square(0.1f) == Catch::Approx(0.01f).epsilon(0.00001f));
        REQUIRE(Square(2.5f) == Catch::Approx(6.25f));
    }
}

TEST_CASE("MathOpt constants", "[math][mathopt][constants]")
{
    SECTION("INV_EPS is reasonable precision")
    {
        REQUIRE(INV_EPS == Catch::Approx(1e-4).epsilon(1e-10));
        REQUIRE(INV_EPS > 0.0);
        REQUIRE(INV_EPS < 0.001);
    }

    SECTION("INVSQRT_EPS is reasonable precision")
    {
        REQUIRE(INVSQRT_EPS == Catch::Approx(1e-3).epsilon(1e-10));
        REQUIRE(INVSQRT_EPS > 0.0);
        REQUIRE(INVSQRT_EPS < 0.01);
    }

    SECTION("INVSQRT_EPS is larger than INV_EPS")
    {
        // Fast inverse square root has lower precision than division
        REQUIRE(INVSQRT_EPS > INV_EPS);
    }
}

TEST_CASE("MathOpt common usage patterns", "[math][mathopt][usage]")
{
    SECTION("Normalize vector using InvSqrt")
    {
        float x = 3.0f, y = 4.0f, z = 0.0f;
        float lengthSquared = x * x + y * y + z * z;
        float invLength = InvSqrt(lengthSquared);

        float nx = x * invLength;
        float ny = y * invLength;
        float nz = z * invLength;

        float normalizedLength = std::sqrt(nx * nx + ny * ny + nz * nz);
        REQUIRE(normalizedLength == Catch::Approx(1.0f).epsilon(INVSQRT_EPS));
    }

    SECTION("Convert angle and compute trig")
    {
        float angleDegrees = 45.0f;
        float angleRadians = HDegree(angleDegrees);

        float cosValue = std::cos(angleRadians);
        float sinValue = std::sin(angleRadians);

        // cos(45�) = sin(45�) = sqrt(2)/2
        REQUIRE(cosValue == Catch::Approx(sinValue).epsilon(0.00001f));
        REQUIRE(cosValue == Catch::Approx(H_INVSQRT2).epsilon(0.00001f));
    }

    SECTION("Distance squared without sqrt")
    {
        float x1 = 1.0f, y1 = 2.0f, z1 = 3.0f;
        float x2 = 4.0f, y2 = 6.0f, z2 = 8.0f;

        float dx = x2 - x1;
        float dy = y2 - y1;
        float dz = z2 - z1;

        float distSquared = Square(dx) + Square(dy) + Square(dz);

        REQUIRE(distSquared == Catch::Approx(50.0f));
    }

    SECTION("Direction sign extraction")
    {
        float velocity = -5.0f;
        int direction = sign(velocity);
        float speedMagnitude = std::abs(velocity);

        REQUIRE(direction == -1);
        REQUIRE(speedMagnitude == 5.0f);

        // Reconstruct original
        float reconstructed = direction * speedMagnitude;
        REQUIRE(reconstructed == velocity);
    }
}

TEST_CASE("MathOpt edge cases", "[math][mathopt][edge]")
{
    SECTION("Very small angle conversions")
    {
        float tiny = 0.001f;
        float radians = HDegree(tiny);
        REQUIRE(radians > 0.0f);
        REQUIRE(radians < 0.0001f);
    }

    SECTION("Large angle conversions")
    {
        float large = 10000.0f;
        float radians = HDegree(large);
        // Should still work, even if result is large
        REQUIRE(std::isfinite(radians));
    }

    SECTION("Reciprocal of very small numbers")
    {
        float tiny = 0.001f;
        float recip = Inv(tiny);
        REQUIRE(recip == Catch::Approx(1000.0f).epsilon(INV_EPS));
    }

    SECTION("Square of very large numbers")
    {
        float large = 1000.0f;
        float squared = Square(large);
        REQUIRE(squared == Catch::Approx(1000000.0f));
    }
}
