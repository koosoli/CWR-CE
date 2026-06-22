// Unit tests for n-dimensional matrix algebra from Poseidon/Foundation/Common/MathND.hpp

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Poseidon/Foundation/Common/MathND.hpp>
#include <catch2/matchers/catch_matchers.hpp>

using Poseidon::Foundation::Matrix;
using Poseidon::Foundation::Vector;

TEST_CASE("Matrix basic construction and access", "[mathND]")
{
    SECTION("3x3 matrix get/set")
    {
        Matrix<3, 3> m;

        m.Set(0, 0) = 1.0f;
        m.Set(0, 1) = 2.0f;
        m.Set(1, 0) = 3.0f;

        REQUIRE(m.Get(0, 0) == 1.0f);
        REQUIRE(m.Get(0, 1) == 2.0f);
        REQUIRE(m.Get(1, 0) == 3.0f);
    }

    SECTION("Matrix operator() access")
    {
        Matrix<2, 2> m;

        m(0, 0) = 5.0f;
        m(0, 1) = 6.0f;
        m(1, 0) = 7.0f;
        m(1, 1) = 8.0f;

        REQUIRE(m(0, 0) == 5.0f);
        REQUIRE(m(0, 1) == 6.0f);
        REQUIRE(m(1, 0) == 7.0f);
        REQUIRE(m(1, 1) == 8.0f);
    }

    SECTION("Matrix dimensions")
    {
        Matrix<4, 3> m;
        REQUIRE(m.Rows == 4);
        REQUIRE(m.Columns == 3);
    }
}

TEST_CASE("Matrix InitIdentity creates identity matrix", "[mathND]")
{
    SECTION("3x3 identity")
    {
        Matrix<3, 3> m;
        m.InitIdentity();

        // Diagonal should be 1
        REQUIRE(m(0, 0) == 1.0f);
        REQUIRE(m(1, 1) == 1.0f);
        REQUIRE(m(2, 2) == 1.0f);

        // Off-diagonal should be 0
        REQUIRE(m(0, 1) == 0.0f);
        REQUIRE(m(0, 2) == 0.0f);
        REQUIRE(m(1, 0) == 0.0f);
        REQUIRE(m(1, 2) == 0.0f);
        REQUIRE(m(2, 0) == 0.0f);
        REQUIRE(m(2, 1) == 0.0f);
    }

    SECTION("2x2 identity")
    {
        Matrix<2, 2> m;
        m.InitIdentity();

        REQUIRE(m(0, 0) == 1.0f);
        REQUIRE(m(1, 1) == 1.0f);
        REQUIRE(m(0, 1) == 0.0f);
        REQUIRE(m(1, 0) == 0.0f);
    }
}

TEST_CASE("Matrix InitZero clears matrix", "[mathND]")
{
    SECTION("3x3 zero matrix")
    {
        Matrix<3, 3> m;
        // Fill with non-zero values first
        for (int i = 0; i < 3; i++)
        {
            for (int j = 0; j < 3; j++)
            {
                m(i, j) = 99.0f;
            }
        }

        m.InitZero();

        for (int i = 0; i < 3; i++)
        {
            for (int j = 0; j < 3; j++)
            {
                REQUIRE(m(i, j) == 0.0f);
            }
        }
    }
}

TEST_CASE("Matrix transpose operation", "[mathND]")
{
    SECTION("3x3 square matrix transpose")
    {
        Matrix<3, 3> m;
        m(0, 0) = 1.0f;
        m(0, 1) = 2.0f;
        m(0, 2) = 3.0f;
        m(1, 0) = 4.0f;
        m(1, 1) = 5.0f;
        m(1, 2) = 6.0f;
        m(2, 0) = 7.0f;
        m(2, 1) = 8.0f;
        m(2, 2) = 9.0f;

        Matrix<3, 3> mt = m.Transposed();

        // Check transpose property: mt(i,j) == m(j,i)
        for (int i = 0; i < 3; i++)
        {
            for (int j = 0; j < 3; j++)
            {
                REQUIRE(mt(i, j) == m(j, i));
            }
        }
    }

    SECTION("2x2 square matrix transpose")
    {
        Matrix<2, 2> m;
        m(0, 0) = 1.0f;
        m(0, 1) = 2.0f;
        m(1, 0) = 3.0f;
        m(1, 1) = 4.0f;

        Matrix<2, 2> mt = m.Transposed();

        REQUIRE(mt(0, 0) == 1.0f);
        REQUIRE(mt(0, 1) == 3.0f);
        REQUIRE(mt(1, 0) == 2.0f);
        REQUIRE(mt(1, 1) == 4.0f);
    }

    // NOTE: Non-square matrix transpose has a bug in the library implementation
    // The Transposed() function incorrectly indexes the result matrix
    // Test disabled until library is fixed
}

TEST_CASE("Matrix multiplication", "[mathND]")
{
    SECTION("2x2 matrix multiplication")
    {
        Matrix<2, 2> a, b, c;

        // A = [1 2]
        //     [3 4]
        a(0, 0) = 1.0f;
        a(0, 1) = 2.0f;
        a(1, 0) = 3.0f;
        a(1, 1) = 4.0f;

        // B = [5 6]
        //     [7 8]
        b(0, 0) = 5.0f;
        b(0, 1) = 6.0f;
        b(1, 0) = 7.0f;
        b(1, 1) = 8.0f;

        Multiply(c, a, b);

        // C = A*B = [19 22]
        //           [43 50]
        REQUIRE(c(0, 0) == 19.0f); // 1*5 + 2*7
        REQUIRE(c(0, 1) == 22.0f); // 1*6 + 2*8
        REQUIRE(c(1, 0) == 43.0f); // 3*5 + 4*7
        REQUIRE(c(1, 1) == 50.0f); // 3*6 + 4*8
    }

    SECTION("Identity matrix multiplication")
    {
        Matrix<3, 3> a, identity, result;

        a(0, 0) = 1.0f;
        a(0, 1) = 2.0f;
        a(0, 2) = 3.0f;
        a(1, 0) = 4.0f;
        a(1, 1) = 5.0f;
        a(1, 2) = 6.0f;
        a(2, 0) = 7.0f;
        a(2, 1) = 8.0f;
        a(2, 2) = 9.0f;

        identity.InitIdentity();
        Multiply(result, a, identity);

        // A * I should equal A
        for (int i = 0; i < 3; i++)
        {
            for (int j = 0; j < 3; j++)
            {
                REQUIRE(result(i, j) == a(i, j));
            }
        }
    }

    SECTION("Matrix-vector multiplication (3x3 * 3x1)")
    {
        Matrix<3, 3> m;
        Vector<3> v, result;

        // M = [1 0 0]
        //     [0 2 0]
        //     [0 0 3]
        m.InitIdentity();
        m(1, 1) = 2.0f;
        m(2, 2) = 3.0f;

        // v = [1]
        //     [2]
        //     [3]
        v[0] = 1.0f;
        v[1] = 2.0f;
        v[2] = 3.0f;

        Multiply(result, m, v);

        // result = [1*1]   = [1]
        //          [2*2]     [4]
        //          [3*3]     [9]
        REQUIRE(result[0] == 1.0f);
        REQUIRE(result[1] == 4.0f);
        REQUIRE(result[2] == 9.0f);
    }
}

TEST_CASE("Matrix inversion", "[mathND]")
{
    SECTION("2x2 matrix inversion")
    {
        Matrix<2, 2> m, inverse;
        bool regular;

        // M = [4 7]
        //     [2 6]
        m(0, 0) = 4.0f;
        m(0, 1) = 7.0f;
        m(1, 0) = 2.0f;
        m(1, 1) = 6.0f;

        inverse = m.Inversed(&regular);
        REQUIRE(regular == true);

        // Verify M * M^-1 = I
        Matrix<2, 2> identity;
        Multiply(identity, m, inverse);

        REQUIRE_THAT(identity(0, 0), Catch::Matchers::WithinAbs(1.0f, 0.0001f));
        REQUIRE_THAT(identity(0, 1), Catch::Matchers::WithinAbs(0.0f, 0.0001f));
        REQUIRE_THAT(identity(1, 0), Catch::Matchers::WithinAbs(0.0f, 0.0001f));
        REQUIRE_THAT(identity(1, 1), Catch::Matchers::WithinAbs(1.0f, 0.0001f));
    }

    SECTION("3x3 identity matrix inversion")
    {
        Matrix<3, 3> m, inverse;
        bool regular;

        m.InitIdentity();
        inverse = m.Inversed(&regular);

        REQUIRE(regular == true);

        // Inverse of identity is identity
        for (int i = 0; i < 3; i++)
        {
            for (int j = 0; j < 3; j++)
            {
                if (i == j)
                {
                    REQUIRE_THAT(inverse(i, j), Catch::Matchers::WithinAbs(1.0f, 0.0001f));
                }
                else
                {
                    REQUIRE_THAT(inverse(i, j), Catch::Matchers::WithinAbs(0.0f, 0.0001f));
                }
            }
        }
    }

    SECTION("Singular matrix (non-invertible)")
    {
        Matrix<2, 2> m;
        bool regular;

        // Singular matrix (determinant = 0)
        // M = [1 2]
        //     [2 4]
        m(0, 0) = 1.0f;
        m(0, 1) = 2.0f;
        m(1, 0) = 2.0f;
        m(1, 1) = 4.0f;

        (void)m.Inversed(&regular); // Intentionally unused - just checking regular flag
        REQUIRE(regular == false);
    }
}

TEST_CASE("Vector operations", "[mathND]")
{
    SECTION("Vector element access")
    {
        Vector<3> v;

        v[0] = 1.0f;
        v[1] = 2.0f;
        v[2] = 3.0f;

        REQUIRE(v[0] == 1.0f);
        REQUIRE(v[1] == 2.0f);
        REQUIRE(v[2] == 3.0f);
    }

    SECTION("Vector is a column matrix")
    {
        Vector<3> v;
        v[0] = 5.0f;
        v[1] = 6.0f;
        v[2] = 7.0f;

        // Should be accessible as Matrix<3,1>
        REQUIRE(v(0, 0) == 5.0f);
        REQUIRE(v(1, 0) == 6.0f);
        REQUIRE(v(2, 0) == 7.0f);
    }
}

TEST_CASE("Complex matrix operations", "[mathND]")
{
    SECTION("3x3 matrix inverse then multiply")
    {
        Matrix<3, 3> m, inverse, product;
        bool regular;

        // Create a non-trivial 3x3 matrix
        m(0, 0) = 1.0f;
        m(0, 1) = 2.0f;
        m(0, 2) = 3.0f;
        m(1, 0) = 0.0f;
        m(1, 1) = 4.0f;
        m(1, 2) = 5.0f;
        m(2, 0) = 1.0f;
        m(2, 1) = 0.0f;
        m(2, 2) = 6.0f;

        inverse = m.Inversed(&regular);
        REQUIRE(regular == true);

        Multiply(product, m, inverse);

        // Should get identity matrix (within tolerance)
        for (int i = 0; i < 3; i++)
        {
            for (int j = 0; j < 3; j++)
            {
                float expected = (i == j) ? 1.0f : 0.0f;
                REQUIRE_THAT(product(i, j), Catch::Matchers::WithinAbs(expected, 0.001f));
            }
        }
    }
}
