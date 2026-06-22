#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Poseidon/Foundation/Math/Math3DP.hpp>
#include <cmath>
#include <Poseidon/Foundation/Math/MathDefs.hpp>

// Test Matrix3P construction and basic operations
TEST_CASE("Matrix3 construction", "[math][matrix][matrix3]")
{
    SECTION("Default constructor")
    {
        Matrix3P mat;
        // In release mode, no initialization guarantees
        // Just verify it compiles
        REQUIRE(true);
    }

    SECTION("Identity matrix")
    {
        Matrix3P mat = M3IdentityP;

        // Diagonal should be 1
        REQUIRE(mat(0, 0) == Catch::Approx(1.0f));
        REQUIRE(mat(1, 1) == Catch::Approx(1.0f));
        REQUIRE(mat(2, 2) == Catch::Approx(1.0f));

        // Off-diagonal should be 0
        REQUIRE(mat(0, 1) == Catch::Approx(0.0f));
        REQUIRE(mat(0, 2) == Catch::Approx(0.0f));
        REQUIRE(mat(1, 0) == Catch::Approx(0.0f));
        REQUIRE(mat(1, 2) == Catch::Approx(0.0f));
        REQUIRE(mat(2, 0) == Catch::Approx(0.0f));
        REQUIRE(mat(2, 1) == Catch::Approx(0.0f));
    }

    SECTION("Zero matrix")
    {
        Matrix3P mat = M3ZeroP;

        for (int i = 0; i < 3; i++)
        {
            for (int j = 0; j < 3; j++)
            {
                REQUIRE(mat(i, j) == Catch::Approx(0.0f));
            }
        }
    }

    SECTION("Custom values constructor")
    {
        Matrix3P mat(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f);

        REQUIRE(mat(0, 0) == Catch::Approx(1.0f));
        REQUIRE(mat(0, 1) == Catch::Approx(2.0f));
        REQUIRE(mat(0, 2) == Catch::Approx(3.0f));
        REQUIRE(mat(1, 0) == Catch::Approx(4.0f));
        REQUIRE(mat(1, 1) == Catch::Approx(5.0f));
        REQUIRE(mat(1, 2) == Catch::Approx(6.0f));
        REQUIRE(mat(2, 0) == Catch::Approx(7.0f));
        REQUIRE(mat(2, 1) == Catch::Approx(8.0f));
        REQUIRE(mat(2, 2) == Catch::Approx(9.0f));
    }

    SECTION("NoInit constructor")
    {
        Matrix3P mat(NoInit);
        // Just verify it compiles
        REQUIRE(true);
    }
}

TEST_CASE("Matrix3 rotation constructors", "[math][matrix][matrix3][rotation]")
{
    const float angle = H_PI / 4.0f; // 45 degrees

    SECTION("Rotation around X axis")
    {
        Matrix3P mat(MRotationX, angle);

        // X axis should remain unchanged
        REQUIRE(mat(0, 0) == Catch::Approx(1.0f));
        REQUIRE(mat(1, 0) == Catch::Approx(0.0f));
        REQUIRE(mat(2, 0) == Catch::Approx(0.0f));

        // Check rotation elements
        float c = std::cos(angle);
        float s = std::sin(angle);
        REQUIRE(mat(1, 1) == Catch::Approx(c).epsilon(0.0001f));
        REQUIRE(mat(1, 2) == Catch::Approx(-s).epsilon(0.0001f));
        REQUIRE(mat(2, 1) == Catch::Approx(s).epsilon(0.0001f));
        REQUIRE(mat(2, 2) == Catch::Approx(c).epsilon(0.0001f));
    }

    SECTION("Rotation around Y axis")
    {
        Matrix3P mat(MRotationY, angle);

        // Y axis should remain unchanged
        REQUIRE(mat(0, 1) == Catch::Approx(0.0f));
        REQUIRE(mat(1, 1) == Catch::Approx(1.0f));
        REQUIRE(mat(2, 1) == Catch::Approx(0.0f));

        // Check rotation elements
        float c = std::cos(angle);
        float s = std::sin(angle);
        REQUIRE(mat(0, 0) == Catch::Approx(c).epsilon(0.0001f));
        REQUIRE(mat(0, 2) == Catch::Approx(-s).epsilon(0.0001f)); // Sign was wrong
        REQUIRE(mat(2, 0) == Catch::Approx(s).epsilon(0.0001f));
        REQUIRE(mat(2, 2) == Catch::Approx(c).epsilon(0.0001f));
    }

    SECTION("Rotation around Z axis")
    {
        Matrix3P mat(MRotationZ, angle);

        // Z axis should remain unchanged
        REQUIRE(mat(0, 2) == Catch::Approx(0.0f));
        REQUIRE(mat(1, 2) == Catch::Approx(0.0f));
        REQUIRE(mat(2, 2) == Catch::Approx(1.0f));

        // Check rotation elements
        float c = std::cos(angle);
        float s = std::sin(angle);
        REQUIRE(mat(0, 0) == Catch::Approx(c).epsilon(0.0001f));
        REQUIRE(mat(0, 1) == Catch::Approx(-s).epsilon(0.0001f));
        REQUIRE(mat(1, 0) == Catch::Approx(s).epsilon(0.0001f));
        REQUIRE(mat(1, 1) == Catch::Approx(c).epsilon(0.0001f));
    }

    SECTION("90 degree rotation creates known values")
    {
        Matrix3P matZ(MRotationZ, H_PI / 2.0f);

        // 90 degree rotation around Z: X -> Y, Y -> -X
        REQUIRE(matZ(0, 0) == Catch::Approx(0.0f).margin(0.0001f));
        REQUIRE(matZ(0, 1) == Catch::Approx(-1.0f).epsilon(0.0001f));
        REQUIRE(matZ(1, 0) == Catch::Approx(1.0f).epsilon(0.0001f));
        REQUIRE(matZ(1, 1) == Catch::Approx(0.0f).margin(0.0001f));
    }
}

TEST_CASE("Matrix3 scale constructor", "[math][matrix][matrix3][scale]")
{
    SECTION("Non-uniform scale")
    {
        Matrix3P mat(MScale, 2.0f, 3.0f, 4.0f);

        REQUIRE(mat(0, 0) == Catch::Approx(2.0f));
        REQUIRE(mat(1, 1) == Catch::Approx(3.0f));
        REQUIRE(mat(2, 2) == Catch::Approx(4.0f));

        // Off-diagonal should be 0
        REQUIRE(mat(0, 1) == Catch::Approx(0.0f));
        REQUIRE(mat(0, 2) == Catch::Approx(0.0f));
        REQUIRE(mat(1, 0) == Catch::Approx(0.0f));
        REQUIRE(mat(1, 2) == Catch::Approx(0.0f));
        REQUIRE(mat(2, 0) == Catch::Approx(0.0f));
        REQUIRE(mat(2, 1) == Catch::Approx(0.0f));
    }

    SECTION("Uniform scale")
    {
        Matrix3P mat(MScale, 2.5f);

        REQUIRE(mat(0, 0) == Catch::Approx(2.5f));
        REQUIRE(mat(1, 1) == Catch::Approx(2.5f));
        REQUIRE(mat(2, 2) == Catch::Approx(2.5f));
    }
}

TEST_CASE("Matrix3 direction constructors", "[math][matrix][matrix3][direction]")
{
    SECTION("Direction and up constructor")
    {
        Vector3P dir(0.0f, 0.0f, 1.0f);
        Vector3P up(0.0f, 1.0f, 0.0f);
        Matrix3P mat(MDirection, dir, up);

        // Direction should be Z axis
        REQUIRE(mat.Direction().Z() == Catch::Approx(1.0f).epsilon(0.001f));
        REQUIRE(mat.Direction().X() == Catch::Approx(0.0f).margin(0.001f));
        REQUIRE(mat.Direction().Y() == Catch::Approx(0.0f).margin(0.001f));

        // Up should be Y axis
        REQUIRE(mat.DirectionUp().Y() == Catch::Approx(1.0f).epsilon(0.001f));
        REQUIRE(mat.DirectionUp().X() == Catch::Approx(0.0f).margin(0.001f));
        REQUIRE(mat.DirectionUp().Z() == Catch::Approx(0.0f).margin(0.001f));
    }

    SECTION("Non-orthogonal direction and up gets orthogonalized")
    {
        Vector3P dir(1.0f, 0.0f, 0.0f);
        Vector3P up(0.5f, 1.0f, 0.0f); // Not perpendicular to dir
        Matrix3P mat(MDirection, dir, up);

        // Direction should remain normalized along X
        REQUIRE(mat.Direction().X() == Catch::Approx(1.0f).epsilon(0.001f));

        // Up should be perpendicular to direction (dot product = 0)
        float dot = mat.Direction().DotProduct(mat.DirectionUp());
        REQUIRE(std::abs(dot) < 0.001f);
    }
}

TEST_CASE("Matrix3 vector transformation", "[math][matrix][matrix3][transform]")
{
    SECTION("Identity matrix doesn't change vector")
    {
        Matrix3P mat = M3IdentityP;
        Vector3P v(1.0f, 2.0f, 3.0f);
        Vector3P result = mat * v;

        REQUIRE(result.X() == Catch::Approx(v.X()));
        REQUIRE(result.Y() == Catch::Approx(v.Y()));
        REQUIRE(result.Z() == Catch::Approx(v.Z()));
    }

    SECTION("Scale matrix scales vector")
    {
        Matrix3P mat(MScale, 2.0f, 3.0f, 4.0f);
        Vector3P v(1.0f, 2.0f, 3.0f);
        Vector3P result = mat * v;

        REQUIRE(result.X() == Catch::Approx(2.0f));
        REQUIRE(result.Y() == Catch::Approx(6.0f));
        REQUIRE(result.Z() == Catch::Approx(12.0f));
    }

    SECTION("90 degree Z rotation")
    {
        Matrix3P mat(MRotationZ, H_PI / 2.0f);
        Vector3P v(1.0f, 0.0f, 0.0f);
        Vector3P result = mat * v;

        // X axis rotates to Y axis
        REQUIRE(result.X() == Catch::Approx(0.0f).margin(0.0001f));
        REQUIRE(result.Y() == Catch::Approx(1.0f).epsilon(0.0001f));
        REQUIRE(result.Z() == Catch::Approx(0.0f).margin(0.0001f));
    }
}

TEST_CASE("Matrix3 multiplication", "[math][matrix][matrix3][multiply]")
{
    SECTION("Identity matrix multiplication")
    {
        Matrix3P mat(MScale, 2.0f, 3.0f, 4.0f);
        Matrix3P identity = M3IdentityP;
        Matrix3P result = mat * identity;

        REQUIRE(result(0, 0) == Catch::Approx(mat(0, 0)));
        REQUIRE(result(1, 1) == Catch::Approx(mat(1, 1)));
        REQUIRE(result(2, 2) == Catch::Approx(mat(2, 2)));
    }

    SECTION("Two rotation matrices combine")
    {
        Matrix3P rotX(MRotationX, H_PI / 4.0f);
        Matrix3P rotY(MRotationY, H_PI / 4.0f);
        Matrix3P combined = rotY * rotX;

        // Apply to a test vector
        Vector3P v(1.0f, 0.0f, 0.0f);
        Vector3P result1 = combined * v;
        Vector3P result2 = rotY * (rotX * v);

        // Both should give same result
        REQUIRE(result1.X() == Catch::Approx(result2.X()).epsilon(0.001f));
        REQUIRE(result1.Y() == Catch::Approx(result2.Y()).epsilon(0.001f));
        REQUIRE(result1.Z() == Catch::Approx(result2.Z()).epsilon(0.001f));
    }

    SECTION("Scalar multiplication")
    {
        Matrix3P mat(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f);
        Matrix3P result = mat * 2.0f;

        for (int i = 0; i < 3; i++)
        {
            for (int j = 0; j < 3; j++)
            {
                REQUIRE(result(i, j) == Catch::Approx(mat(i, j) * 2.0f));
            }
        }
    }
}

TEST_CASE("Matrix3 inverse operations", "[math][matrix][matrix3][inverse]")
{
    SECTION("Inverse of rotation is transpose")
    {
        Matrix3P rot(MRotationZ, H_PI / 6.0f);
        Matrix3P inv = rot.InverseRotation();
        Matrix3P product = rot * inv;

        // Should give identity
        REQUIRE(product(0, 0) == Catch::Approx(1.0f).epsilon(0.001f));
        REQUIRE(product(1, 1) == Catch::Approx(1.0f).epsilon(0.001f));
        REQUIRE(product(2, 2) == Catch::Approx(1.0f).epsilon(0.001f));
        REQUIRE(product(0, 1) == Catch::Approx(0.0f).margin(0.001f));
        REQUIRE(product(1, 0) == Catch::Approx(0.0f).margin(0.001f));
    }

    SECTION("Inverse of scale matrix")
    {
        Matrix3P scale(MScale, 2.0f, 3.0f, 4.0f);
        Matrix3P inv = scale.InverseScaled();
        Matrix3P product = scale * inv;

        // Should give identity
        REQUIRE(product(0, 0) == Catch::Approx(1.0f).epsilon(0.001f));
        REQUIRE(product(1, 1) == Catch::Approx(1.0f).epsilon(0.001f));
        REQUIRE(product(2, 2) == Catch::Approx(1.0f).epsilon(0.001f));
    }

    SECTION("Inverse cancels transformation")
    {
        Matrix3P rot(MRotationY, 0.5f);
        Matrix3P inv = rot.InverseRotation();

        Vector3P v(1.0f, 2.0f, 3.0f);
        Vector3P transformed = rot * v;
        Vector3P restored = inv * transformed;

        REQUIRE(restored.X() == Catch::Approx(v.X()).epsilon(0.001f));
        REQUIRE(restored.Y() == Catch::Approx(v.Y()).epsilon(0.001f));
        REQUIRE(restored.Z() == Catch::Approx(v.Z()).epsilon(0.001f));
    }
}

TEST_CASE("Matrix3 Tilda (cross product matrix)", "[math][matrix][matrix3][tilda]")
{
    SECTION("Tilda matrix represents cross product")
    {
        Vector3P a(1.0f, 2.0f, 3.0f);
        Vector3P b(4.0f, 5.0f, 6.0f);

        Matrix3P tilda(MTilda, a);
        Vector3P crossProduct = a.CrossProduct(b);
        Vector3P tildaProduct = tilda * b;

        REQUIRE(crossProduct.X() == Catch::Approx(tildaProduct.X()).epsilon(0.001f));
        REQUIRE(crossProduct.Y() == Catch::Approx(tildaProduct.Y()).epsilon(0.001f));
        REQUIRE(crossProduct.Z() == Catch::Approx(tildaProduct.Z()).epsilon(0.001f));
    }
}

TEST_CASE("Matrix4 construction", "[math][matrix][matrix4]")
{
    SECTION("Identity matrix")
    {
        Matrix4P mat = M4IdentityP;

        // 3x3 part diagonal should be 1
        REQUIRE(mat(0, 0) == Catch::Approx(1.0f));
        REQUIRE(mat(1, 1) == Catch::Approx(1.0f));
        REQUIRE(mat(2, 2) == Catch::Approx(1.0f));

        // Position should be 0
        REQUIRE(mat.Position().X() == Catch::Approx(0.0f));
        REQUIRE(mat.Position().Y() == Catch::Approx(0.0f));
        REQUIRE(mat.Position().Z() == Catch::Approx(0.0f));
    }

    SECTION("Zero matrix")
    {
        Matrix4P mat = M4ZeroP;

        for (int i = 0; i < 3; i++)
        {
            for (int j = 0; j < 3; j++)
            {
                REQUIRE(mat(i, j) == Catch::Approx(0.0f));
            }
        }

        REQUIRE(mat.Position().X() == Catch::Approx(0.0f));
        REQUIRE(mat.Position().Y() == Catch::Approx(0.0f));
        REQUIRE(mat.Position().Z() == Catch::Approx(0.0f));
    }

    SECTION("Translation constructor")
    {
        Vector3P offset(1.0f, 2.0f, 3.0f);
        Matrix4P mat(MTranslation, offset);

        REQUIRE(mat.Position().X() == Catch::Approx(1.0f));
        REQUIRE(mat.Position().Y() == Catch::Approx(2.0f));
        REQUIRE(mat.Position().Z() == Catch::Approx(3.0f));

        // Orientation should be identity
        REQUIRE(mat(0, 0) == Catch::Approx(1.0f));
        REQUIRE(mat(1, 1) == Catch::Approx(1.0f));
        REQUIRE(mat(2, 2) == Catch::Approx(1.0f));
    }

    SECTION("Custom values constructor")
    {
        Matrix4P mat(1.0f, 2.0f, 3.0f, 10.0f, 4.0f, 5.0f, 6.0f, 20.0f, 7.0f, 8.0f, 9.0f, 30.0f);

        REQUIRE(mat(0, 0) == Catch::Approx(1.0f));
        REQUIRE(mat(0, 1) == Catch::Approx(2.0f));
        REQUIRE(mat.Position().X() == Catch::Approx(10.0f));
        REQUIRE(mat.Position().Y() == Catch::Approx(20.0f));
        REQUIRE(mat.Position().Z() == Catch::Approx(30.0f));
    }
}

TEST_CASE("Matrix4 rotation constructors", "[math][matrix][matrix4][rotation]")
{
    SECTION("Rotation around X axis")
    {
        Matrix4P mat(MRotationX, H_PI / 4.0f);

        // Position should be zero
        REQUIRE(mat.Position().X() == Catch::Approx(0.0f));
        REQUIRE(mat.Position().Y() == Catch::Approx(0.0f));
        REQUIRE(mat.Position().Z() == Catch::Approx(0.0f));

        // X axis unchanged
        REQUIRE(mat(0, 0) == Catch::Approx(1.0f));
    }

    SECTION("Rotation around Y axis")
    {
        Matrix4P mat(MRotationY, H_PI / 4.0f);

        // Y axis unchanged
        REQUIRE(mat(1, 1) == Catch::Approx(1.0f));
    }

    SECTION("Rotation around Z axis")
    {
        Matrix4P mat(MRotationZ, H_PI / 4.0f);

        // Z axis unchanged
        REQUIRE(mat(2, 2) == Catch::Approx(1.0f));
    }
}

TEST_CASE("Matrix4 vector transformation", "[math][matrix][matrix4][transform]")
{
    SECTION("Translation moves point")
    {
        Vector3P offset(1.0f, 2.0f, 3.0f);
        Matrix4P mat(MTranslation, offset);
        Vector3P v(4.0f, 5.0f, 6.0f);
        Vector3P result = mat * v;

        REQUIRE(result.X() == Catch::Approx(5.0f));
        REQUIRE(result.Y() == Catch::Approx(7.0f));
        REQUIRE(result.Z() == Catch::Approx(9.0f));
    }

    SECTION("Rotate method only rotates (no translation)")
    {
        Vector3P offset(10.0f, 20.0f, 30.0f);
        Matrix4P mat(MTranslation, offset);
        mat.SetRotationZ(H_PI / 2.0f);

        Vector3P v(1.0f, 0.0f, 0.0f);
        Vector3P rotated = mat.Rotate(v);

        // Should rotate but not translate
        REQUIRE(rotated.X() == Catch::Approx(0.0f).margin(0.0001f));
        REQUIRE(rotated.Y() == Catch::Approx(1.0f).epsilon(0.001f));
        REQUIRE(rotated.Z() == Catch::Approx(0.0f).margin(0.0001f));
    }

    SECTION("FastTransform includes translation")
    {
        Vector3P offset(1.0f, 2.0f, 3.0f);
        Matrix4P mat(MTranslation, offset);
        Vector3P v(0.0f, 0.0f, 0.0f);
        Vector3P result = mat.FastTransform(v);

        REQUIRE(result.X() == Catch::Approx(1.0f));
        REQUIRE(result.Y() == Catch::Approx(2.0f));
        REQUIRE(result.Z() == Catch::Approx(3.0f));
    }
}

TEST_CASE("Matrix4 combined transformations", "[math][matrix][matrix4][transform]")
{
    SECTION("Translation then rotation")
    {
        Vector3P offset(0.0f, 0.0f, 5.0f);
        Matrix4P translate(MTranslation, offset);
        Matrix4P rotate(MRotationY, H_PI / 2.0f);

        Matrix4P combined = rotate * translate;

        Vector3P v(0.0f, 0.0f, 0.0f);
        Vector3P result = combined * v;

        // Origin translated by (0,0,5) then rotated 90� around Y
        // (0,0,5) -> (-5,0,0) due to rotation convention
        REQUIRE(result.X() == Catch::Approx(-5.0f).epsilon(0.001f));
        REQUIRE(result.Y() == Catch::Approx(0.0f).margin(0.001f));
        REQUIRE(result.Z() == Catch::Approx(0.0f).margin(0.001f));
    }

    SECTION("Scale and rotation")
    {
        Matrix4P scale(MScale, 2.0f, 2.0f, 2.0f);
        Matrix4P rotate(MRotationZ, H_PI / 4.0f);

        Matrix4P combined = rotate * scale;

        Vector3P v(1.0f, 0.0f, 0.0f);
        Vector3P result = combined * v;

        // First scaled to (2,0,0), then rotated 45�
        float expected = 2.0f * std::cos(H_PI / 4.0f);
        REQUIRE(result.X() == Catch::Approx(expected).epsilon(0.001f));
        REQUIRE(result.Y() == Catch::Approx(expected).epsilon(0.001f));
    }
}

TEST_CASE("Matrix4 inverse operations", "[math][matrix][matrix4][inverse]")
{
    SECTION("Inverse of translation")
    {
        Vector3P offset(1.0f, 2.0f, 3.0f);
        Matrix4P mat(MTranslation, offset);
        Matrix4P inv = mat.InverseRotation();

        Vector3P v(4.0f, 5.0f, 6.0f);
        Vector3P transformed = mat * v;
        Vector3P restored = inv * transformed;

        REQUIRE(restored.X() == Catch::Approx(v.X()).epsilon(0.001f));
        REQUIRE(restored.Y() == Catch::Approx(v.Y()).epsilon(0.001f));
        REQUIRE(restored.Z() == Catch::Approx(v.Z()).epsilon(0.001f));
    }

    SECTION("Inverse of rotation and translation")
    {
        Matrix4P mat(MRotationZ, H_PI / 3.0f);
        mat.SetPosition(Vector3P(5.0f, 10.0f, 15.0f));

        Matrix4P inv = mat.InverseRotation();
        Matrix4P product = mat * inv;

        // Position should be zero after round-trip
        Vector3P origin(0.0f, 0.0f, 0.0f);
        Vector3P result = product * origin;

        REQUIRE(result.X() == Catch::Approx(0.0f).margin(0.01f));
        REQUIRE(result.Y() == Catch::Approx(0.0f).margin(0.01f));
        REQUIRE(result.Z() == Catch::Approx(0.0f).margin(0.01f));
    }
}

TEST_CASE("Matrix4 view matrix", "[math][matrix][matrix4][view]")
{
    SECTION("View matrix looks at origin")
    {
        Vector3P eye(0.0f, 0.0f, 10.0f);
        Vector3P dir(0.0f, 0.0f, -1.0f);
        Vector3P up(0.0f, 1.0f, 0.0f);

        Matrix4P view(MView, eye, dir, up);

        // Camera is at (0,0,10) looking down -Z
        // Origin in world should be at (0,0,10) in view space (positive Z)
        Vector3P origin(0.0f, 0.0f, 0.0f);
        Vector3P viewSpace = view * origin;

        REQUIRE(viewSpace.Z() == Catch::Approx(10.0f).epsilon(0.001f));
    }
}

TEST_CASE("Matrix4 accessors", "[math][matrix][matrix4][accessors]")
{
    SECTION("Position accessor")
    {
        Matrix4P mat = M4IdentityP;
        mat.SetPosition(Vector3P(1.0f, 2.0f, 3.0f));

        REQUIRE(mat.Position().X() == Catch::Approx(1.0f));
        REQUIRE(mat.Position().Y() == Catch::Approx(2.0f));
        REQUIRE(mat.Position().Z() == Catch::Approx(3.0f));
    }

    SECTION("Direction vectors")
    {
        Matrix4P mat = M4IdentityP;

        // Default identity has standard basis
        REQUIRE(mat.Direction().Z() == Catch::Approx(1.0f));
        REQUIRE(mat.DirectionUp().Y() == Catch::Approx(1.0f));
        REQUIRE(mat.DirectionAside().X() == Catch::Approx(1.0f));
    }

    SECTION("Orientation accessor")
    {
        Matrix4P mat(MRotationZ, H_PI / 4.0f);
        const Matrix3P& orient = mat.Orientation();

        REQUIRE(orient(0, 0) == Catch::Approx(mat(0, 0)));
        REQUIRE(orient(1, 1) == Catch::Approx(mat(1, 1)));
        REQUIRE(orient(2, 2) == Catch::Approx(mat(2, 2)));
    }
}

TEST_CASE("Matrix edge cases", "[math][matrix][edge]")
{
    SECTION("Zero scale matrix")
    {
        Matrix3P mat(MScale, 0.0f, 0.0f, 0.0f);
        Vector3P v(1.0f, 2.0f, 3.0f);
        Vector3P result = mat * v;

        REQUIRE(result.X() == Catch::Approx(0.0f));
        REQUIRE(result.Y() == Catch::Approx(0.0f));
        REQUIRE(result.Z() == Catch::Approx(0.0f));
    }

    SECTION("Very small rotation angle")
    {
        Matrix3P mat(MRotationZ, 0.0001f);

        // Should be very close to identity
        REQUIRE(mat(0, 0) == Catch::Approx(1.0f).epsilon(0.001f));
        REQUIRE(mat(1, 1) == Catch::Approx(1.0f).epsilon(0.001f));
    }

    SECTION("Full 360 degree rotation")
    {
        Matrix3P mat(MRotationZ, 2.0f * H_PI);

        // Should be identity
        REQUIRE(mat(0, 0) == Catch::Approx(1.0f).epsilon(0.001f));
        REQUIRE(mat(1, 1) == Catch::Approx(1.0f).epsilon(0.001f));
        REQUIRE(mat(0, 1) == Catch::Approx(0.0f).margin(0.001f));
        REQUIRE(mat(1, 0) == Catch::Approx(0.0f).margin(0.001f));
    }
}

TEST_CASE("Matrix arithmetic operations", "[math][matrix][arithmetic]")
{
    SECTION("Matrix3 addition")
    {
        Matrix3P a(MScale, 1.0f, 2.0f, 3.0f);
        Matrix3P b(MScale, 4.0f, 5.0f, 6.0f);
        Matrix3P result = a + b;

        REQUIRE(result(0, 0) == Catch::Approx(5.0f));
        REQUIRE(result(1, 1) == Catch::Approx(7.0f));
        REQUIRE(result(2, 2) == Catch::Approx(9.0f));
    }

    SECTION("Matrix4 addition")
    {
        Matrix4P a(MTranslation, Vector3P(1.0f, 2.0f, 3.0f));
        Matrix4P b(MTranslation, Vector3P(4.0f, 5.0f, 6.0f));
        Matrix4P result = a + b;

        REQUIRE(result.Position().X() == Catch::Approx(5.0f));
        REQUIRE(result.Position().Y() == Catch::Approx(7.0f));
        REQUIRE(result.Position().Z() == Catch::Approx(9.0f));
    }

    SECTION("Matrix compound operators")
    {
        Matrix3P mat(MScale, 1.0f, 2.0f, 3.0f);
        Matrix3P add(MScale, 1.0f, 1.0f, 1.0f);
        mat += add;

        REQUIRE(mat(0, 0) == Catch::Approx(2.0f));
        REQUIRE(mat(1, 1) == Catch::Approx(3.0f));
        REQUIRE(mat(2, 2) == Catch::Approx(4.0f));
    }
}

TEST_CASE("Matrix4 multiply by identity is unchanged", "[math][matrix][matrix4][multiply]")
{
    Matrix4P mat(MRotationZ, H_PI / 6.0f);
    mat.SetPosition(Vector3P(3.0f, 7.0f, -2.0f));

    Matrix4P result = mat * M4IdentityP;

    REQUIRE(result(0, 0) == Catch::Approx(mat(0, 0)));
    REQUIRE(result(0, 1) == Catch::Approx(mat(0, 1)));
    REQUIRE(result(1, 0) == Catch::Approx(mat(1, 0)));
    REQUIRE(result(1, 1) == Catch::Approx(mat(1, 1)));
    REQUIRE(result(2, 2) == Catch::Approx(mat(2, 2)));
    REQUIRE(result.Position().X() == Catch::Approx(mat.Position().X()));
    REQUIRE(result.Position().Y() == Catch::Approx(mat.Position().Y()));
    REQUIRE(result.Position().Z() == Catch::Approx(mat.Position().Z()));
}

TEST_CASE("Matrix common usage patterns", "[math][matrix][usage]")
{
    SECTION("Transform point from local to world space")
    {
        // Object at (5,0,0), facing along +Z
        Matrix4P worldTransform(MTranslation, Vector3P(5.0f, 0.0f, 0.0f));

        // Point at (1,0,0) in object's local space
        Vector3P localPoint(1.0f, 0.0f, 0.0f);
        Vector3P worldPoint = worldTransform * localPoint;

        REQUIRE(worldPoint.X() == Catch::Approx(6.0f));
        REQUIRE(worldPoint.Y() == Catch::Approx(0.0f));
        REQUIRE(worldPoint.Z() == Catch::Approx(0.0f));
    }

    SECTION("Chain multiple transformations")
    {
        // Parent at (10,0,0)
        Matrix4P parent(MTranslation, Vector3P(10.0f, 0.0f, 0.0f));

        // Child offset by (0,5,0) relative to parent
        Matrix4P childLocal(MTranslation, Vector3P(0.0f, 5.0f, 0.0f));

        // Child in world space
        Matrix4P childWorld = parent * childLocal;

        REQUIRE(childWorld.Position().X() == Catch::Approx(10.0f));
        REQUIRE(childWorld.Position().Y() == Catch::Approx(5.0f));
        REQUIRE(childWorld.Position().Z() == Catch::Approx(0.0f));
    }

    SECTION("Compute inverse transformation for raycasting")
    {
        // Object rotated 45� and translated
        Matrix4P objTransform(MRotationZ, H_PI / 4.0f);
        objTransform.SetPosition(Vector3P(5.0f, 5.0f, 0.0f));

        Matrix4P invTransform = objTransform.InverseRotation();

        // World point transformed to local space
        Vector3P worldPoint(6.0f, 5.0f, 0.0f);
        Vector3P localPoint = invTransform * worldPoint;

        // Verify round-trip
        Vector3P backToWorld = objTransform * localPoint;
        REQUIRE(backToWorld.X() == Catch::Approx(worldPoint.X()).epsilon(0.001f));
        REQUIRE(backToWorld.Y() == Catch::Approx(worldPoint.Y()).epsilon(0.001f));
    }
}
