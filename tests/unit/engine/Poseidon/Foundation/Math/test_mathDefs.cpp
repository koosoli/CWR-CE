#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <Poseidon/Foundation/Math/MathDefs.hpp>
#include <cmath>
#include <cfloat>

// Test math constants defined in mathDefs.hpp
TEST_CASE("Math constants are correct", "[math][mathdefs][constants]")
{
    SECTION("H_PI constant precision")
    {
        const double pi_reference = 3.14159265358979323846;
        REQUIRE(H_PI == Catch::Approx(pi_reference).epsilon(0.00001f));

        // Verify it's within float precision
        REQUIRE(std::abs(H_PI - pi_reference) < 0.000001);
    }

    SECTION("H_SQRT2 constant precision")
    {
        const double sqrt2_reference = 1.41421356237;
        REQUIRE(H_SQRT2 == Catch::Approx(sqrt2_reference).epsilon(0.00001f));

        // Verify by squaring
        REQUIRE(H_SQRT2 * H_SQRT2 == Catch::Approx(2.0f).epsilon(0.0001f));
    }

    SECTION("H_INVSQRT2 constant precision")
    {
        const double invsqrt2_reference = 0.70710678118;
        REQUIRE(H_INVSQRT2 == Catch::Approx(invsqrt2_reference).epsilon(0.00001f));

        // Verify relationship with H_SQRT2
        REQUIRE(H_INVSQRT2 * H_SQRT2 == Catch::Approx(1.0f).epsilon(0.0001f));

        // Verify relationship with 2
        REQUIRE(H_INVSQRT2 * H_INVSQRT2 * 2.0f == Catch::Approx(1.0f).epsilon(0.0001f));
    }
}

TEST_CASE("Coord type behaves correctly", "[math][mathdefs][coord]")
{
    SECTION("Coord is float typedef")
    {
        Coord value = 3.14f;
        REQUIRE(sizeof(Coord) == sizeof(float));
        REQUIRE(value == Catch::Approx(3.14f));
    }

    SECTION("COORD_MIN is minimum float value")
    {
        REQUIRE(COORD_MIN == -FLT_MAX);
        REQUIRE(COORD_MIN < 0.0f);
        REQUIRE(std::isfinite(COORD_MIN));
    }

    SECTION("COORD_MAX is maximum float value")
    {
        REQUIRE(COORD_MAX == FLT_MAX);
        REQUIRE(COORD_MAX > 0.0f);
        REQUIRE(std::isfinite(COORD_MAX));
    }

    SECTION("coord() macro converts to float")
    {
        REQUIRE(coord(1) == 1.0f);
        REQUIRE(coord(2.5) == 2.5f);
        REQUIRE(coord(100) == 100.0f);

        // Verify type is float
        REQUIRE(sizeof(coord(1)) == sizeof(float));
    }
}

TEST_CASE("Float precision edge cases", "[math][mathdefs][precision]")
{
    SECTION("Small numbers near zero")
    {
        float epsilon = 0.00001f;
        REQUIRE(epsilon > 0.0f);
        REQUIRE(epsilon < 0.0001f);

        // Test that small differences are detected
        float a = 1.0f;
        float b = 1.0f + epsilon;
        REQUIRE(a != b);
    }

    SECTION("Large numbers near float max")
    {
        float large = FLT_MAX * 0.5f;
        REQUIRE(large > 0.0f);
        REQUIRE(std::isfinite(large));

        // Verify arithmetic works
        float half = large * 0.5f;
        REQUIRE(std::isfinite(half));
    }

    SECTION("Denormal numbers")
    {
        float tiny = FLT_MIN;
        REQUIRE(tiny > 0.0f);
        REQUIRE(std::isfinite(tiny));

        // Verify operations with tiny numbers
        float result = tiny * 0.5f;
        REQUIRE(std::isfinite(result));
    }
}

TEST_CASE("Constructor enum placeholders exist", "[math][mathdefs][enums]")
{
    SECTION("Vector constructor enums")
    {
        // These should compile without errors
        _noInit noInitValue = NoInit;
        _vRotate rotateValue = VRotate;
        _vFastTransform fastTransformValue = VFastTransform;
        _vFastTransformA fastTransformAValue = VFastTransformA;
        _vPerspective perspectiveValue = VPerspective;
        _vMultiply multiplyValue = VMultiply;
        _vMultiplyLeft multiplyLeftValue = VMultiplyLeft;

        // Suppress unused warnings (these are compile-time tests)
        (void)noInitValue;
        (void)rotateValue;
        (void)fastTransformValue;
        (void)fastTransformAValue;
        (void)perspectiveValue;
        (void)multiplyValue;
        (void)multiplyLeftValue;

        REQUIRE(true); // Just verify compilation
    }

    SECTION("Matrix constructor enums")
    {
        // These should compile without errors
        _mTranslation translationValue = MTranslation;
        _mTilda tildaValue = MTilda;
        _mRotationX rotationXValue = MRotationX;
        _mRotationY rotationYValue = MRotationY;
        _mRotationZ rotationZValue = MRotationZ;
        _mScale scaleValue = MScale;
        _mPerspective perspectiveValue = MPerspective;
        _mDirection directionValue = MDirection;
        _mUpAndDirection upAndDirectionValue = MUpAndDirection;
        _mView viewValue = MView;
        _mMultiply multiplyValue = MMultiply;
        _mInverseRotation inverseRotationValue = MInverseRotation;
        _mInverseScaled inverseScaledValue = MInverseScaled;
        _mInverseGeneral inverseGeneralValue = MInverseGeneral;
        _mNormalTransform normalTransformValue = MNormalTransform;

        // Suppress unused warnings (these are compile-time tests)
        (void)translationValue;
        (void)tildaValue;
        (void)rotationXValue;
        (void)rotationYValue;
        (void)rotationZValue;
        (void)scaleValue;
        (void)perspectiveValue;
        (void)directionValue;
        (void)upAndDirectionValue;
        (void)viewValue;
        (void)multiplyValue;
        (void)inverseRotationValue;
        (void)inverseScaledValue;
        (void)inverseGeneralValue;
        (void)normalTransformValue;

        REQUIRE(true); // Just verify compilation
    }
}

TEST_CASE("Math constant relationships", "[math][mathdefs][relationships]")
{
    SECTION("PI relationships")
    {
        // 2*PI radians = 360 degrees
        float two_pi = H_PI * 2.0f;
        REQUIRE(two_pi == Catch::Approx(6.28318f).epsilon(0.0001f));

        // PI/2 radians = 90 degrees
        float half_pi = H_PI * 0.5f;
        REQUIRE(half_pi == Catch::Approx(1.57079f).epsilon(0.0001f));
    }

    SECTION("SQRT2 relationships")
    {
        // sqrt(2) is the diagonal of unit square
        float diagonal = std::sqrt(1.0f * 1.0f + 1.0f * 1.0f);
        REQUIRE(H_SQRT2 == Catch::Approx(diagonal).epsilon(0.0001f));
    }

    SECTION("INVSQRT2 relationships")
    {
        // 1/sqrt(2) = sqrt(2)/2
        float formula = H_SQRT2 / 2.0f;
        REQUIRE(H_INVSQRT2 == Catch::Approx(formula).epsilon(0.0001f));
    }
}

// Original simple tests (kept for compatibility)
TEST_CASE("Basic math operations", "[math][simple]")
{
    SECTION("Addition works")
    {
        REQUIRE(2 + 2 == 4);
    }

    SECTION("Pi constant is correct")
    {
        const float pi = 3.14159265358979323846f;
        REQUIRE(pi == Catch::Approx(3.14159).epsilon(0.00001));
    }

    SECTION("Square root is correct")
    {
        double sqrt2 = std::sqrt(2.0);
        REQUIRE(sqrt2 == Catch::Approx(1.4142135).epsilon(0.0001));
    }
}
