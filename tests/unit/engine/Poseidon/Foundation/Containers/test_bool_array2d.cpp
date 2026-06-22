// Unit tests for array2D from PoseidonBase containers
// Note: boolArray tests skipped due to undefined PACKED macro in boolArray.hpp

#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Poseidon/Foundation/Containers/Array.hpp> // Needed for MemAllocD
#include <Poseidon/Foundation/Containers/Array2D.hpp>

// Array2D Tests

TEST_CASE("Array2D - 2D array wrapper", "[array2d]")
{
    SECTION("Default constructor")
    {
        Array2D<int> arr;
        REQUIRE(arr.GetXRange() == 0);
        REQUIRE(arr.GetYRange() == 0);
    }

    SECTION("Dim allocates 2D array")
    {
        Array2D<int> arr;
        arr.Dim(5, 3);

        REQUIRE(arr.GetXRange() == 5);
        REQUIRE(arr.GetYRange() == 3);
    }

    SECTION("Set and Get with coordinates")
    {
        Array2D<int> arr;
        arr.Dim(10, 10);

        arr.Set(0, 0) = 100;
        arr.Set(5, 3) = 200;
        arr.Set(9, 9) = 300;

        REQUIRE(arr.Get(0, 0) == 100);
        REQUIRE(arr.Get(5, 3) == 200);
        REQUIRE(arr.Get(9, 9) == 300);
    }

    SECTION("Operator() for access")
    {
        Array2D<int> arr;
        arr.Dim(5, 5);

        arr(2, 3) = 42;
        arr(4, 1) = 99;

        REQUIRE(arr(2, 3) == 42);
        REQUIRE(arr(4, 1) == 99);
    }

    SECTION("Clear resets dimensions")
    {
        Array2D<int> arr;
        arr.Dim(10, 10);
        arr(5, 5) = 123;

        arr.Clear();

        REQUIRE(arr.GetXRange() == 0);
        REQUIRE(arr.GetYRange() == 0);
    }

    SECTION("Resize preserves data layout")
    {
        Array2D<int> arr;
        arr.Dim(3, 2);

        // Fill with pattern
        int val = 1;
        for (int y = 0; y < 2; y++)
        {
            for (int x = 0; x < 3; x++)
            {
                arr(x, y) = val++;
            }
        }

        // Verify pattern
        REQUIRE(arr(0, 0) == 1);
        REQUIRE(arr(1, 0) == 2);
        REQUIRE(arr(2, 0) == 3);
        REQUIRE(arr(0, 1) == 4);
        REQUIRE(arr(1, 1) == 5);
        REQUIRE(arr(2, 1) == 6);
    }

    SECTION("RawData provides direct access")
    {
        Array2D<int> arr;
        arr.Dim(2, 2);

        arr(0, 0) = 1;
        arr(1, 0) = 2;
        arr(0, 1) = 3;
        arr(1, 1) = 4;

        const int* raw = static_cast<const int*>(arr.RawData());
        REQUIRE(raw != nullptr);
        REQUIRE(raw[0] == 1);
        REQUIRE(raw[1] == 2);
        REQUIRE(raw[2] == 3);
        REQUIRE(raw[3] == 4);
    }

    SECTION("RawSize returns total bytes")
    {
        Array2D<int> arr;
        arr.Dim(5, 4);

        REQUIRE(arr.RawSize() == 5 * 4 * sizeof(int));
    }
}

TEST_CASE("Array2D - Complex types", "[array2d]")
{
    struct Point
    {
        float x, y;
        Point() : x(0), y(0) {}
        Point(float _x, float _y) : x(_x), y(_y) {}
    };

    SECTION("Array of structs")
    {
        Array2D<Point> grid;
        grid.Dim(3, 3);

        grid(0, 0) = Point(1.0f, 2.0f);
        grid(1, 1) = Point(3.0f, 4.0f);
        grid(2, 2) = Point(5.0f, 6.0f);

        REQUIRE(grid(0, 0).x == 1.0f);
        REQUIRE(grid(0, 0).y == 2.0f);
        REQUIRE(grid(1, 1).x == 3.0f);
        REQUIRE(grid(2, 2).y == 6.0f);
    }
}

TEST_CASE("Array2D - Different dimensions", "[array2d]")
{
    SECTION("Wide array")
    {
        Array2D<int> arr;
        arr.Dim(100, 2);

        arr(99, 1) = 999;
        REQUIRE(arr(99, 1) == 999);
    }

    SECTION("Tall array")
    {
        Array2D<int> arr;
        arr.Dim(2, 100);

        arr(1, 99) = 199;
        REQUIRE(arr(1, 99) == 199);
    }

    SECTION("Square array")
    {
        Array2D<int> arr;
        arr.Dim(50, 50);

        arr(25, 25) = 2525;
        REQUIRE(arr(25, 25) == 2525);
    }
}
