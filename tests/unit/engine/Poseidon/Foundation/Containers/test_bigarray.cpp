// Unit tests for BigArray from PoseidonBase containers

#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Poseidon/Foundation/Containers/BigArray.hpp>

// BigArrayNormal Tests

TEST_CASE("BigArrayNormal - Fixed size 2D array", "[bigarray]")
{
    SECTION("Access and modify elements")
    {
        BigArrayNormal<int, 10, 10> arr;

        // Set some values
        arr(0, 0) = 100;
        arr(5, 5) = 200;
        arr(9, 9) = 300;

        // Verify values
        REQUIRE(arr(0, 0) == 100);
        REQUIRE(arr(5, 5) == 200);
        REQUIRE(arr(9, 9) == 300);
    }

    SECTION("Const access")
    {
        BigArrayNormal<int, 8, 8> arr;
        arr(3, 4) = 42;

        const BigArrayNormal<int, 8, 8>& constArr = arr;
        REQUIRE(constArr(3, 4) == 42);
    }

    SECTION("GetFour retrieves 2x2 block")
    {
        BigArrayNormal<int, 10, 10> arr;

        // Set up a 2x2 region
        arr(3, 3) = 1;
        arr(4, 3) = 2;
        arr(3, 4) = 3;
        arr(4, 4) = 4;

        int result[2][2];
        arr.GetFour(result, 3, 3);

        REQUIRE(result[0][0] == 1);
        REQUIRE(result[0][1] == 2);
        REQUIRE(result[1][0] == 3);
        REQUIRE(result[1][1] == 4);
    }

    SECTION("Different data types")
    {
        BigArrayNormal<float, 16, 16> arr;

        arr(10, 10) = 3.14f;
        arr(15, 15) = 2.71f;

        REQUIRE(arr(10, 10) == 3.14f);
        REQUIRE(arr(15, 15) == 2.71f);
    }

    SECTION("Fill and verify pattern")
    {
        BigArrayNormal<int, 4, 4> arr;

        // Fill with pattern
        int val = 0;
        for (int y = 0; y < 4; y++)
        {
            for (int x = 0; x < 4; x++)
            {
                arr(x, y) = val++;
            }
        }

        // Verify pattern
        val = 0;
        for (int y = 0; y < 4; y++)
        {
            for (int x = 0; x < 4; x++)
            {
                REQUIRE(arr(x, y) == val++);
            }
        }
    }
}

// BigArray Tests (cache-optimized version)

TEST_CASE("BigArray - Cache-optimized 2D array", "[bigarray]")
{
    SECTION("Access and modify elements")
    {
        BigArray<int, 16, 16> arr;

        // Set some values
        arr(0, 0) = 100;
        arr(8, 8) = 200;
        arr(15, 15) = 300;

        // Verify values
        REQUIRE(arr(0, 0) == 100);
        REQUIRE(arr(8, 8) == 200);
        REQUIRE(arr(15, 15) == 300);
    }

    SECTION("Const access")
    {
        BigArray<int, 32, 32> arr;
        arr(20, 20) = 42;

        const BigArray<int, 32, 32>& constArr = arr;
        REQUIRE(constArr(20, 20) == 42);
    }

    SECTION("GetFour retrieves 2x2 block")
    {
        BigArray<int, 16, 16> arr;

        // Set up a 2x2 region
        arr(7, 7) = 10;
        arr(8, 7) = 20;
        arr(7, 8) = 30;
        arr(8, 8) = 40;

        int result[2][2];
        arr.GetFour(result, 7, 7);

        REQUIRE(result[0][0] == 10);
        REQUIRE(result[0][1] == 20);
        REQUIRE(result[1][0] == 30);
        REQUIRE(result[1][1] == 40);
    }

    SECTION("Works with power-of-2 dimensions")
    {
        // BigArray requires power-of-2 dimensions
        BigArray<int, 32, 32> arr32;
        arr32(31, 31) = 999;
        REQUIRE(arr32(31, 31) == 999);

        BigArray<int, 64, 64> arr64;
        arr64(63, 63) = 888;
        REQUIRE(arr64(63, 63) == 888);

        BigArray<int, 128, 128> arr128;
        arr128(127, 127) = 777;
        REQUIRE(arr128(127, 127) == 777);
    }

    SECTION("Fill and verify pattern")
    {
        BigArray<int, 16, 16> arr;

        // Fill with pattern
        int val = 0;
        for (int y = 0; y < 16; y++)
        {
            for (int x = 0; x < 16; x++)
            {
                arr(x, y) = val++;
            }
        }

        // Verify pattern
        val = 0;
        for (int y = 0; y < 16; y++)
        {
            for (int x = 0; x < 16; x++)
            {
                REQUIRE(arr(x, y) == val++);
            }
        }
    }

    SECTION("GetFour across cache block boundaries")
    {
        BigArray<int, 16, 16> arr;

        // Set values that span cache blocks (elem size is 4x4)
        arr(3, 3) = 1;
        arr(4, 3) = 2;
        arr(3, 4) = 3;
        arr(4, 4) = 4;

        int result[2][2];
        arr.GetFour(result, 3, 3);

        REQUIRE(result[0][0] == 1);
        REQUIRE(result[0][1] == 2);
        REQUIRE(result[1][0] == 3);
        REQUIRE(result[1][1] == 4);
    }
}

TEST_CASE("BigArray - Larger dimensions", "[bigarray]")
{
    SECTION("256x256 array")
    {
        BigArray<short, 256, 256> arr;

        arr(0, 0) = 1;
        arr(128, 128) = 2;
        arr(255, 255) = 3;

        REQUIRE(arr(0, 0) == 1);
        REQUIRE(arr(128, 128) == 2);
        REQUIRE(arr(255, 255) == 3);
    }

    SECTION("Non-square dimensions")
    {
        BigArray<int, 64, 32> arr;

        arr(63, 31) = 100;
        arr(0, 0) = 200;

        REQUIRE(arr(63, 31) == 100);
        REQUIRE(arr(0, 0) == 200);
    }
}

TEST_CASE("BigArray vs BigArrayNormal - Same behavior", "[bigarray]")
{
    SECTION("Equivalent access patterns")
    {
        BigArrayNormal<int, 16, 16> normal;
        BigArray<int, 16, 16> optimized;

        // Fill both with same pattern
        for (int y = 0; y < 16; y++)
        {
            for (int x = 0; x < 16; x++)
            {
                int val = y * 16 + x;
                normal(x, y) = val;
                optimized(x, y) = val;
            }
        }

        // Verify both return same values
        for (int y = 0; y < 16; y++)
        {
            for (int x = 0; x < 16; x++)
            {
                REQUIRE(normal(x, y) == optimized(x, y));
            }
        }
    }

    SECTION("Equivalent GetFour behavior")
    {
        BigArrayNormal<int, 16, 16> normal;
        BigArray<int, 16, 16> optimized;

        // Set same values
        for (int y = 5; y <= 6; y++)
        {
            for (int x = 5; x <= 6; x++)
            {
                int val = y * 10 + x;
                normal(x, y) = val;
                optimized(x, y) = val;
            }
        }

        int resultNormal[2][2];
        int resultOptimized[2][2];

        normal.GetFour(resultNormal, 5, 5);
        optimized.GetFour(resultOptimized, 5, 5);

        for (int y = 0; y < 2; y++)
        {
            for (int x = 0; x < 2; x++)
            {
                REQUIRE(resultNormal[y][x] == resultOptimized[y][x]);
            }
        }
    }
}
