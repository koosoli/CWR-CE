#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Poseidon/Foundation/Math/Math3D.hpp>
#include <Poseidon/Foundation/Math/MathDefs.hpp>
#include <cmath>
#include <float.h>
#include <Poseidon/Foundation/Math/Math3DP.hpp>

// Vector3 Construction Tests

TEST_CASE("Vector3 construction", "[math][vector3][construction]")
{
    SECTION("Default constructor (debug mode initializes to FLT_MAX)")
    {
        Vector3 v;
// In release mode, no initialization
// In debug mode, initialized to FLT_MAX
#ifndef NDEBUG
        REQUIRE(v[0] == FLT_MAX);
        REQUIRE(v[1] == FLT_MAX);
        REQUIRE(v[2] == FLT_MAX);
#endif
    }

    SECTION("Constructor with three coordinates")
    {
        Vector3 v(1.0f, 2.0f, 3.0f);
        REQUIRE(v.X() == Catch::Approx(1.0f));
        REQUIRE(v.Y() == Catch::Approx(2.0f));
        REQUIRE(v.Z() == Catch::Approx(3.0f));
    }

    SECTION("Copy constructor")
    {
        Vector3 v1(4.0f, 5.0f, 6.0f);
        Vector3 v2(v1);

        REQUIRE(v2.X() == Catch::Approx(4.0f));
        REQUIRE(v2.Y() == Catch::Approx(5.0f));
        REQUIRE(v2.Z() == Catch::Approx(6.0f));
    }

    SECTION("NoInit constructor leaves data uninitialized")
    {
        Vector3 v(NoInit);
        // No assertion - just verify it compiles and doesn't crash
        (void)v;
        REQUIRE(true);
    }

    SECTION("Array-style access")
    {
        Vector3 v(7.0f, 8.0f, 9.0f);
        REQUIRE(v[0] == Catch::Approx(7.0f));
        REQUIRE(v[1] == Catch::Approx(8.0f));
        REQUIRE(v[2] == Catch::Approx(9.0f));
    }
}

// Vector3 Assignment and Modification

TEST_CASE("Vector3 assignment and modification", "[math][vector3][assignment]")
{
    SECTION("Assignment operator")
    {
        Vector3 v1(1.0f, 2.0f, 3.0f);
        Vector3 v2(0.0f, 0.0f, 0.0f);

        v2 = v1;

        REQUIRE(v2.X() == Catch::Approx(1.0f));
        REQUIRE(v2.Y() == Catch::Approx(2.0f));
        REQUIRE(v2.Z() == Catch::Approx(3.0f));
    }

    SECTION("Array-style modification")
    {
        Vector3 v(0.0f, 0.0f, 0.0f);
        v[0] = 10.0f;
        v[1] = 20.0f;
        v[2] = 30.0f;

        REQUIRE(v.X() == Catch::Approx(10.0f));
        REQUIRE(v.Y() == Catch::Approx(20.0f));
        REQUIRE(v.Z() == Catch::Approx(30.0f));
    }
}

// Vector3 Arithmetic Operations

TEST_CASE("Vector3 arithmetic operations", "[math][vector3][arithmetic]")
{
    SECTION("Vector addition")
    {
        Vector3 v1(1.0f, 2.0f, 3.0f);
        Vector3 v2(4.0f, 5.0f, 6.0f);
        Vector3 result = v1 + v2;

        REQUIRE(result.X() == Catch::Approx(5.0f));
        REQUIRE(result.Y() == Catch::Approx(7.0f));
        REQUIRE(result.Z() == Catch::Approx(9.0f));
    }

    SECTION("Vector subtraction")
    {
        Vector3 v1(10.0f, 20.0f, 30.0f);
        Vector3 v2(1.0f, 2.0f, 3.0f);
        Vector3 result = v1 - v2;

        REQUIRE(result.X() == Catch::Approx(9.0f));
        REQUIRE(result.Y() == Catch::Approx(18.0f));
        REQUIRE(result.Z() == Catch::Approx(27.0f));
    }

    SECTION("Vector negation")
    {
        Vector3 v(1.0f, -2.0f, 3.0f);
        Vector3 result = -v;

        REQUIRE(result.X() == Catch::Approx(-1.0f));
        REQUIRE(result.Y() == Catch::Approx(2.0f));
        REQUIRE(result.Z() == Catch::Approx(-3.0f));
    }

    SECTION("Scalar multiplication")
    {
        Vector3 v(2.0f, 3.0f, 4.0f);
        Vector3 result = v * 2.5f;

        REQUIRE(result.X() == Catch::Approx(5.0f));
        REQUIRE(result.Y() == Catch::Approx(7.5f));
        REQUIRE(result.Z() == Catch::Approx(10.0f));
    }

    SECTION("Scalar division")
    {
        Vector3 v(10.0f, 20.0f, 30.0f);
        Vector3 result = v / 2.0f;

        REQUIRE(result.X() == Catch::Approx(5.0f));
        REQUIRE(result.Y() == Catch::Approx(10.0f));
        REQUIRE(result.Z() == Catch::Approx(15.0f));
    }

    SECTION("Compound addition")
    {
        Vector3 v(1.0f, 2.0f, 3.0f);
        v += Vector3(4.0f, 5.0f, 6.0f);

        REQUIRE(v.X() == Catch::Approx(5.0f));
        REQUIRE(v.Y() == Catch::Approx(7.0f));
        REQUIRE(v.Z() == Catch::Approx(9.0f));
    }

    SECTION("Compound subtraction")
    {
        Vector3 v(10.0f, 20.0f, 30.0f);
        v -= Vector3(1.0f, 2.0f, 3.0f);

        REQUIRE(v.X() == Catch::Approx(9.0f));
        REQUIRE(v.Y() == Catch::Approx(18.0f));
        REQUIRE(v.Z() == Catch::Approx(27.0f));
    }
}

// Vector3 Dot Product

TEST_CASE("Vector3 dot product", "[math][vector3][dot]")
{
    SECTION("Perpendicular vectors")
    {
        Vector3 v1(1.0f, 0.0f, 0.0f);
        Vector3 v2(0.0f, 1.0f, 0.0f);
        float dot = v1.DotProduct(v2);

        REQUIRE(dot == Catch::Approx(0.0f).epsilon(0.0001f));
    }

    SECTION("Parallel vectors")
    {
        Vector3 v1(1.0f, 0.0f, 0.0f);
        Vector3 v2(2.0f, 0.0f, 0.0f);
        float dot = v1.DotProduct(v2);

        REQUIRE(dot == Catch::Approx(2.0f));
    }

    SECTION("Opposite vectors")
    {
        Vector3 v1(1.0f, 0.0f, 0.0f);
        Vector3 v2(-1.0f, 0.0f, 0.0f);
        float dot = v1.DotProduct(v2);

        REQUIRE(dot == Catch::Approx(-1.0f));
    }

    SECTION("General dot product")
    {
        Vector3 v1(1.0f, 2.0f, 3.0f);
        Vector3 v2(4.0f, 5.0f, 6.0f);
        float dot = v1.DotProduct(v2);

        // 1*4 + 2*5 + 3*6 = 4 + 10 + 18 = 32
        REQUIRE(dot == Catch::Approx(32.0f));
    }

    SECTION("Dot product is commutative")
    {
        Vector3 v1(2.0f, 3.0f, 4.0f);
        Vector3 v2(5.0f, 6.0f, 7.0f);

        REQUIRE(v1.DotProduct(v2) == Catch::Approx(v2.DotProduct(v1)));
    }

    SECTION("Dot product using * operator")
    {
        Vector3 v1(1.0f, 2.0f, 3.0f);
        Vector3 v2(4.0f, 5.0f, 6.0f);
        float dot = v1 * v2;

        REQUIRE(dot == Catch::Approx(32.0f));
    }
}

// Vector3 Cross Product

TEST_CASE("Vector3 cross product", "[math][vector3][cross]")
{
    SECTION("Standard basis vectors")
    {
        Vector3 x(1.0f, 0.0f, 0.0f);
        Vector3 y(0.0f, 1.0f, 0.0f);
        Vector3 z = x.CrossProduct(y);

        REQUIRE(z.X() == Catch::Approx(0.0f).epsilon(0.0001f));
        REQUIRE(z.Y() == Catch::Approx(0.0f).epsilon(0.0001f));
        REQUIRE(z.Z() == Catch::Approx(1.0f));
    }

    SECTION("Right-hand rule")
    {
        Vector3 y(0.0f, 1.0f, 0.0f);
        Vector3 z(0.0f, 0.0f, 1.0f);
        Vector3 x = y.CrossProduct(z);

        REQUIRE(x.X() == Catch::Approx(1.0f));
        REQUIRE(x.Y() == Catch::Approx(0.0f).epsilon(0.0001f));
        REQUIRE(x.Z() == Catch::Approx(0.0f).epsilon(0.0001f));
    }

    SECTION("Cross product is anti-commutative")
    {
        Vector3 v1(1.0f, 2.0f, 3.0f);
        Vector3 v2(4.0f, 5.0f, 6.0f);

        Vector3 c1 = v1.CrossProduct(v2);
        Vector3 c2 = v2.CrossProduct(v1);

        REQUIRE(c1.X() == Catch::Approx(-c2.X()));
        REQUIRE(c1.Y() == Catch::Approx(-c2.Y()));
        REQUIRE(c1.Z() == Catch::Approx(-c2.Z()));
    }

    SECTION("Parallel vectors have zero cross product")
    {
        Vector3 v1(1.0f, 2.0f, 3.0f);
        Vector3 v2(2.0f, 4.0f, 6.0f); // v2 = 2*v1

        Vector3 cross = v1.CrossProduct(v2);

        REQUIRE(cross.X() == Catch::Approx(0.0f).epsilon(0.0001f));
        REQUIRE(cross.Y() == Catch::Approx(0.0f).epsilon(0.0001f));
        REQUIRE(cross.Z() == Catch::Approx(0.0f).epsilon(0.0001f));
    }

    SECTION("Cross product is perpendicular to both vectors")
    {
        Vector3 v1(1.0f, 2.0f, 3.0f);
        Vector3 v2(4.0f, 5.0f, 6.0f);
        Vector3 cross = v1.CrossProduct(v2);

        // cross � v1 should be 0
        REQUIRE(cross.DotProduct(v1) == Catch::Approx(0.0f).epsilon(0.0001f));

        // cross � v2 should be 0
        REQUIRE(cross.DotProduct(v2) == Catch::Approx(0.0f).epsilon(0.0001f));
    }
}

// Vector3 Magnitude/Length

TEST_CASE("Vector3 magnitude and normalization", "[math][vector3][magnitude]")
{
    SECTION("Size (magnitude) calculation")
    {
        Vector3 v(3.0f, 4.0f, 0.0f);
        float size = v.Size();

        // 3-4-5 triangle
        REQUIRE(size == Catch::Approx(5.0f));
    }

    SECTION("SquareSize calculation")
    {
        Vector3 v(3.0f, 4.0f, 0.0f);
        float squareSize = v.SquareSize();

        REQUIRE(squareSize == Catch::Approx(25.0f));
    }

    SECTION("Unit vector has size 1")
    {
        Vector3 v(1.0f, 0.0f, 0.0f);
        REQUIRE(v.Size() == Catch::Approx(1.0f));
    }

    SECTION("Normalization")
    {
        Vector3 v(3.0f, 4.0f, 0.0f);
        v.Normalize();

        REQUIRE(v.Size() == Catch::Approx(1.0f).epsilon(0.0001f));

        // Direction should be preserved
        REQUIRE(v.X() == Catch::Approx(0.6f).epsilon(0.0001f));
        REQUIRE(v.Y() == Catch::Approx(0.8f).epsilon(0.0001f));
    }

    SECTION("Zero vector size")
    {
        Vector3 v(0.0f, 0.0f, 0.0f);
        float size = v.Size();

        REQUIRE(size == Catch::Approx(0.0f));
    }
}

// Vector3 Distance Calculations

TEST_CASE("Vector3 distance calculations", "[math][vector3][distance]")
{
    SECTION("Distance between two points")
    {
        Vector3 p1(0.0f, 0.0f, 0.0f);
        Vector3 p2(3.0f, 4.0f, 0.0f);

        Vector3 diff = p2 - p1;
        float distance = diff.Size();

        REQUIRE(distance == Catch::Approx(5.0f));
    }

    SECTION("Square distance is faster to compute")
    {
        Vector3 p1(1.0f, 2.0f, 3.0f);
        Vector3 p2(4.0f, 6.0f, 8.0f);

        Vector3 diff = p2 - p1;
        float squareDistance = diff.SquareSize();

        // (4-1)^2 + (6-2)^2 + (8-3)^2 = 9 + 16 + 25 = 50
        REQUIRE(squareDistance == Catch::Approx(50.0f));
    }

    SECTION("2D distance (XZ plane)")
    {
        Vector3 v(3.0f, 100.0f, 4.0f); // Y is ignored
        float distanceXZ = v.SizeXZ();

        REQUIRE(distanceXZ == Catch::Approx(5.0f));
    }

    SECTION("Square distance XZ")
    {
        Vector3 v(3.0f, 100.0f, 4.0f);
        float squareDistanceXZ = v.SquareSizeXZ();

        REQUIRE(squareDistanceXZ == Catch::Approx(25.0f));
    }
}

// Vector3 Edge Cases

TEST_CASE("Vector3 edge cases", "[math][vector3][edge]")
{
    SECTION("Zero vector operations")
    {
        Vector3 zero(0.0f, 0.0f, 0.0f);
        Vector3 v(1.0f, 2.0f, 3.0f);

        // Adding zero doesn't change vector
        Vector3 result = v + zero;
        REQUIRE(result.X() == Catch::Approx(v.X()));
        REQUIRE(result.Y() == Catch::Approx(v.Y()));
        REQUIRE(result.Z() == Catch::Approx(v.Z()));

        // Dot with zero is zero
        REQUIRE(v.DotProduct(zero) == Catch::Approx(0.0f));

        // Cross with zero is zero
        Vector3 cross = v.CrossProduct(zero);
        REQUIRE(cross.Size() == Catch::Approx(0.0f).epsilon(0.0001f));
    }

    SECTION("Very small vectors")
    {
        Vector3 tiny(0.0001f, 0.0001f, 0.0001f);
        float size = tiny.Size();

        REQUIRE(size > 0.0f);
        REQUIRE(std::isfinite(size));
    }

    SECTION("Very large vectors")
    {
        float large = FLT_MAX * 0.1f;
        Vector3 huge(large, large, large);

        REQUIRE(std::isfinite(huge.X()));
        REQUIRE(std::isfinite(huge.Y()));
        REQUIRE(std::isfinite(huge.Z()));

        // SquareSize might overflow, but Size() should handle it or return inf
        float size = huge.Size();
        bool sizeIsValid = std::isfinite(size) || std::isinf(size);
        REQUIRE(sizeIsValid);
    }

    SECTION("Collinear vectors")
    {
        Vector3 v1(1.0f, 2.0f, 3.0f);
        Vector3 v2(2.0f, 4.0f, 6.0f);

        // Cross product should be zero
        Vector3 cross = v1.CrossProduct(v2);
        REQUIRE(cross.Size() == Catch::Approx(0.0f).epsilon(0.0001f));
    }
}

// Vector3 Comparison Operations

TEST_CASE("Vector3 comparison operations", "[math][vector3][comparison]")
{
    SECTION("Vectors with same values are equal")
    {
        Vector3 v1(1.0f, 2.0f, 3.0f);
        Vector3 v2(1.0f, 2.0f, 3.0f);

        REQUIRE(v1.X() == Catch::Approx(v2.X()));
        REQUIRE(v1.Y() == Catch::Approx(v2.Y()));
        REQUIRE(v1.Z() == Catch::Approx(v2.Z()));
    }

    SECTION("Component-wise comparison")
    {
        Vector3 v1(1.0f, 2.0f, 3.0f);
        Vector3 v2(1.1f, 2.1f, 3.1f);

        REQUIRE(v1.X() != v2.X());
        REQUIRE(v1.Y() != v2.Y());
        REQUIRE(v1.Z() != v2.Z());
    }
}

// Vector3 Common Patterns

TEST_CASE("Vector3 common usage patterns", "[math][vector3][patterns]")
{
    SECTION("Compute direction vector")
    {
        Vector3 from(1.0f, 2.0f, 3.0f);
        Vector3 to(4.0f, 6.0f, 8.0f);

        Vector3 direction = to - from;
        direction.Normalize();

        REQUIRE(direction.Size() == Catch::Approx(1.0f).epsilon(0.0001f));
    }

    SECTION("Compute midpoint")
    {
        Vector3 p1(0.0f, 0.0f, 0.0f);
        Vector3 p2(10.0f, 20.0f, 30.0f);

        Vector3 midpoint = (p1 + p2) * 0.5f;

        REQUIRE(midpoint.X() == Catch::Approx(5.0f));
        REQUIRE(midpoint.Y() == Catch::Approx(10.0f));
        REQUIRE(midpoint.Z() == Catch::Approx(15.0f));
    }

    SECTION("Compute surface normal from triangle")
    {
        Vector3 p1(0.0f, 0.0f, 0.0f);
        Vector3 p2(1.0f, 0.0f, 0.0f);
        Vector3 p3(0.0f, 1.0f, 0.0f);

        Vector3 edge1 = p2 - p1;
        Vector3 edge2 = p3 - p1;
        Vector3 normal = edge1.CrossProduct(edge2);
        normal.Normalize();

        // Should point in Z direction
        REQUIRE(normal.X() == Catch::Approx(0.0f).epsilon(0.0001f));
        REQUIRE(normal.Y() == Catch::Approx(0.0f).epsilon(0.0001f));
        REQUIRE(normal.Z() == Catch::Approx(1.0f));
    }

    SECTION("Angle between vectors using dot product")
    {
        Vector3 v1(1.0f, 0.0f, 0.0f);
        Vector3 v2(0.0f, 1.0f, 0.0f);

        float dot = v1.DotProduct(v2);
        float angle = std::acos(dot); // Both are unit vectors

        // 90 degrees = PI/2 radians
        REQUIRE(angle == Catch::Approx(H_PI / 2.0f).epsilon(0.0001f));
    }
}
