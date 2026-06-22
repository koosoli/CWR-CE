// Unit tests for Binary Search algorithms from Es library

#include <catch2/catch_test_macros.hpp>

#include <Poseidon/Foundation/Algorithms/BSearch.hpp>

using Poseidon::Foundation::BSearch;
using Poseidon::Foundation::BSearchPos;

// Simple comparator for integers
struct IntComparator
{
    int operator()(int key, int value) const { return key - value; }
};

TEST_CASE("BSearch - Standard binary search", "[bSearch]")
{
    IntComparator cmp;

    SECTION("Find element in sorted array")
    {
        int data[] = {1, 3, 5, 7, 9, 11, 13, 15};

        REQUIRE(BSearch(data, 8, cmp, 1) == 0);  // First element
        REQUIRE(BSearch(data, 8, cmp, 7) == 3);  // Middle element
        REQUIRE(BSearch(data, 8, cmp, 15) == 7); // Last element
        REQUIRE(BSearch(data, 8, cmp, 5) == 2);  // Arbitrary element
    }

    SECTION("Element not found")
    {
        int data[] = {1, 3, 5, 7, 9};

        REQUIRE(BSearch(data, 5, cmp, 0) == -1);  // Before first
        REQUIRE(BSearch(data, 5, cmp, 4) == -1);  // Between elements
        REQUIRE(BSearch(data, 5, cmp, 10) == -1); // After last
    }

    SECTION("Single element array")
    {
        int data[] = {42};

        REQUIRE(BSearch(data, 1, cmp, 42) == 0);
        REQUIRE(BSearch(data, 1, cmp, 41) == -1);
        REQUIRE(BSearch(data, 1, cmp, 43) == -1);
    }

    SECTION("Two element array")
    {
        int data[] = {10, 20};

        REQUIRE(BSearch(data, 2, cmp, 10) == 0);
        REQUIRE(BSearch(data, 2, cmp, 20) == 1);
        REQUIRE(BSearch(data, 2, cmp, 15) == -1);
    }

    SECTION("Empty array")
    {
        int* data = nullptr;
        REQUIRE(BSearch(data, 0, cmp, 5) == -1);
    }

    SECTION("Duplicate elements")
    {
        int data[] = {1, 3, 5, 5, 5, 7, 9};

        // Should return ANY valid index where 5 is found
        int idx = BSearch(data, 7, cmp, 5);
        REQUIRE(idx >= 2);
        REQUIRE(idx <= 4);
        REQUIRE(data[idx] == 5);
    }
}

TEST_CASE("BSearchPos - Find insertion position", "[bSearch]")
{
    IntComparator cmp;

    SECTION("Element exists - returns index")
    {
        int data[] = {1, 3, 5, 7, 9};

        REQUIRE(BSearchPos(data, 5, cmp, 1) == 0);
        REQUIRE(BSearchPos(data, 5, cmp, 5) == 2);
        REQUIRE(BSearchPos(data, 5, cmp, 9) == 4);
    }

    SECTION("Element doesn't exist - returns insertion point")
    {
        int data[] = {1, 3, 5, 7, 9};

        REQUIRE(BSearchPos(data, 5, cmp, 0) == 0);  // Insert at start
        REQUIRE(BSearchPos(data, 5, cmp, 2) == 1);  // Insert between 1 and 3
        REQUIRE(BSearchPos(data, 5, cmp, 4) == 2);  // Insert between 3 and 5
        REQUIRE(BSearchPos(data, 5, cmp, 10) == 5); // Insert at end
    }

    SECTION("Single element")
    {
        int data[] = {5};

        REQUIRE(BSearchPos(data, 1, cmp, 3) == 0); // Insert before
        REQUIRE(BSearchPos(data, 1, cmp, 5) == 0); // Equal
        REQUIRE(BSearchPos(data, 1, cmp, 7) == 1); // Insert after
    }

    SECTION("Two elements")
    {
        int data[] = {10, 20};

        REQUIRE(BSearchPos(data, 2, cmp, 5) == 0);  // Before first
        REQUIRE(BSearchPos(data, 2, cmp, 10) == 0); // Equal to first
        REQUIRE(BSearchPos(data, 2, cmp, 15) == 1); // Between
        REQUIRE(BSearchPos(data, 2, cmp, 20) == 1); // Equal to second
        REQUIRE(BSearchPos(data, 2, cmp, 25) == 2); // After last
    }

    SECTION("Empty array")
    {
        int* data = nullptr;
        REQUIRE(BSearchPos(data, 0, cmp, 5) == 0);
    }

    SECTION("Maintains sorted order property")
    {
        int data[] = {1, 5, 10, 15, 20};

        // For any value, insertion at returned position maintains order
        for (int value = 0; value <= 21; value++)
        {
            int pos = BSearchPos(data, 5, cmp, value);

            // Check position is valid
            REQUIRE(pos >= 0);
            REQUIRE(pos <= 5);

            // Check order property: all before pos are <= value
            for (int i = 0; i < pos; i++)
            {
                REQUIRE(data[i] <= value);
            }

            // Check order property: all after pos are >= value
            for (int i = pos; i < 5; i++)
            {
                REQUIRE(data[i] >= value);
            }
        }
    }
}

TEST_CASE("BSearch with custom types", "[bSearch]")
{
    struct Point
    {
        int x, y;
    };

    struct PointXComparator
    {
        int operator()(int key, const Point& pt) const { return key - pt.x; }
    };

    Point points[] = {{1, 10}, {3, 20}, {5, 30}, {7, 40}, {9, 50}};

    PointXComparator cmp;

    SECTION("Search by x coordinate")
    {
        REQUIRE(BSearch(points, 5, cmp, 1) == 0);
        REQUIRE(BSearch(points, 5, cmp, 5) == 2);
        REQUIRE(BSearch(points, 5, cmp, 9) == 4);
        REQUIRE(BSearch(points, 5, cmp, 4) == -1);
    }

    SECTION("Find insertion position")
    {
        REQUIRE(BSearchPos(points, 5, cmp, 0) == 0);
        REQUIRE(BSearchPos(points, 5, cmp, 6) == 3);
        REQUIRE(BSearchPos(points, 5, cmp, 10) == 5);
    }
}
