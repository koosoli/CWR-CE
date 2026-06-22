// Unit tests for Quick Sort algorithm from Es library

#include <catch2/catch_test_macros.hpp>

#include <Poseidon/Foundation/Algorithms/Qsort.hpp>
#include <algorithm> // for std::is_sorted
#include <cstring>   // for memcpy

using Poseidon::Foundation::QSort;
using Poseidon::Foundation::QSortWithContext;

// Simple comparator for integers (ascending)
int IntCompare(const int* a, const int* b)
{
    return *a - *b;
}

// Descending comparator
int IntCompareDesc(const int* a, const int* b)
{
    return *b - *a;
}

TEST_CASE("QSort - Basic sorting", "[qsort]")
{
    SECTION("Sort random array")
    {
        int data[] = {5, 2, 8, 1, 9, 3, 7, 4, 6};
        int expected[] = {1, 2, 3, 4, 5, 6, 7, 8, 9};

        QSort(data, 9, IntCompare);

        for (int i = 0; i < 9; i++)
        {
            REQUIRE(data[i] == expected[i]);
        }
    }

    SECTION("Already sorted array")
    {
        int data[] = {1, 2, 3, 4, 5};
        int expected[] = {1, 2, 3, 4, 5};

        QSort(data, 5, IntCompare);

        for (int i = 0; i < 5; i++)
        {
            REQUIRE(data[i] == expected[i]);
        }
    }

    SECTION("Reverse sorted array")
    {
        int data[] = {9, 8, 7, 6, 5, 4, 3, 2, 1};

        QSort(data, 9, IntCompare);

        for (int i = 0; i < 8; i++)
        {
            REQUIRE(data[i] < data[i + 1]);
        }
    }

    SECTION("Array with duplicates")
    {
        int data[] = {3, 1, 4, 1, 5, 9, 2, 6, 5, 3};

        QSort(data, 10, IntCompare);

        // Check sorted order
        for (int i = 0; i < 9; i++)
        {
            REQUIRE(data[i] <= data[i + 1]);
        }

        // Verify all original values present
        REQUIRE(data[0] == 1);
        REQUIRE(data[9] == 9);
    }
}

TEST_CASE("QSort - Edge cases", "[qsort]")
{
    SECTION("Single element")
    {
        int data[] = {42};
        QSort(data, 1, IntCompare);
        REQUIRE(data[0] == 42);
    }

    SECTION("Two elements - already sorted")
    {
        int data[] = {1, 2};
        QSort(data, 2, IntCompare);
        REQUIRE(data[0] == 1);
        REQUIRE(data[1] == 2);
    }

    SECTION("Two elements - needs swap")
    {
        int data[] = {2, 1};
        QSort(data, 2, IntCompare);
        REQUIRE(data[0] == 1);
        REQUIRE(data[1] == 2);
    }

    SECTION("Empty array (num=0)")
    {
        int data[] = {1, 2, 3};
        QSort(data, 0, IntCompare); // Should not crash
        // No assertion needed, just verify no crash
    }

    SECTION("All elements identical")
    {
        int data[] = {5, 5, 5, 5, 5};
        QSort(data, 5, IntCompare);
        for (int i = 0; i < 5; i++)
        {
            REQUIRE(data[i] == 5);
        }
    }
}

TEST_CASE("QSort - Small array optimization (insertion sort path)", "[qsort]")
{
    SECTION("Array size exactly at cutoff (8 elements)")
    {
        int data[] = {8, 3, 6, 1, 7, 2, 5, 4};

        QSort(data, 8, IntCompare);

        for (int i = 0; i < 7; i++)
        {
            REQUIRE(data[i] < data[i + 1]);
        }
    }

    SECTION("Array size just below cutoff (7 elements)")
    {
        int data[] = {7, 3, 6, 1, 5, 2, 4};

        QSort(data, 7, IntCompare);

        for (int i = 0; i < 6; i++)
        {
            REQUIRE(data[i] < data[i + 1]);
        }
    }

    SECTION("Very small array (3 elements)")
    {
        int data[] = {3, 1, 2};

        QSort(data, 3, IntCompare);

        REQUIRE(data[0] == 1);
        REQUIRE(data[1] == 2);
        REQUIRE(data[2] == 3);
    }
}

TEST_CASE("QSort - Large array (quicksort path)", "[qsort]")
{
    SECTION("Array size above cutoff (100 elements)")
    {
        int data[100];

        // Fill with reverse sorted data
        for (int i = 0; i < 100; i++)
        {
            data[i] = 100 - i;
        }

        QSort(data, 100, IntCompare);

        // Verify sorted
        for (int i = 0; i < 99; i++)
        {
            REQUIRE(data[i] < data[i + 1]);
        }

        REQUIRE(data[0] == 1);
        REQUIRE(data[99] == 100);
    }
}

TEST_CASE("QSort - Descending sort", "[qsort]")
{
    SECTION("Sort in descending order")
    {
        int data[] = {5, 2, 8, 1, 9, 3, 7, 4, 6};

        QSort(data, 9, IntCompareDesc);

        // Verify descending order
        for (int i = 0; i < 8; i++)
        {
            REQUIRE(data[i] > data[i + 1]);
        }

        REQUIRE(data[0] == 9);
        REQUIRE(data[8] == 1);
    }
}

TEST_CASE("QSort - Stability and correctness", "[qsort]")
{
    SECTION("All permutations of small array")
    {
        // Test all permutations of {1, 2, 3} to ensure correctness
        int original[] = {1, 2, 3};
        int data[3];

        // Generate and test all 6 permutations
        do
        {
            memcpy(data, original, sizeof(data));
            QSort(data, 3, IntCompare);

            REQUIRE(data[0] == 1);
            REQUIRE(data[1] == 2);
            REQUIRE(data[2] == 3);
        } while (std::next_permutation(original, original + 3));
    }
}

TEST_CASE("QSort with custom types", "[qsort]")
{
    struct Person
    {
        int age;
        char name[20];
    };

    // Use static function instead of lambda
    static auto PersonAgeCompare = [](const Person* a, const Person* b) -> int { return a->age - b->age; };

    SECTION("Sort people by age")
    {
        Person people[] = {{30, "Alice"}, {25, "Bob"}, {35, "Charlie"}, {20, "David"}};

        // Convert lambda to function pointer
        int (*cmp)(const Person*, const Person*) = PersonAgeCompare;
        QSort(people, 4, cmp);

        REQUIRE(people[0].age == 20);
        REQUIRE(people[1].age == 25);
        REQUIRE(people[2].age == 30);
        REQUIRE(people[3].age == 35);

        REQUIRE(strcmp(people[0].name, "David") == 0);
        REQUIRE(strcmp(people[3].name, "Charlie") == 0);
    }
}

TEST_CASE("QSort - Context version", "[qsort]")
{
    struct SortContext
    {
        bool ascending;
    };

    // Use function pointer version, not lambda
    auto ContextCompare = [](const int* a, const int* b, SortContext ctx) -> int
    {
        if (ctx.ascending)
        {
            return *a - *b;
        }
        else
        {
            return *b - *a;
        }
    };

    SECTION("Ascending with context")
    {
        int data[] = {5, 2, 8, 1, 9};
        SortContext ctx{true};

        // Use QSortWithContext for functor version
        QSortWithContext(data, 5, ctx, ContextCompare);

        REQUIRE(data[0] == 1);
        REQUIRE(data[4] == 9);
    }

    SECTION("Descending with context")
    {
        int data[] = {5, 2, 8, 1, 9};
        SortContext ctx{false};

        // Use QSortWithContext for functor version
        QSortWithContext(data, 5, ctx, ContextCompare);

        REQUIRE(data[0] == 9);
        REQUIRE(data[4] == 1);
    }
}
