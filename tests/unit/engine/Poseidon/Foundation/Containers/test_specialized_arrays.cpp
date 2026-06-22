// Unit tests for SmallArray and StaticArray from PoseidonBase containers
// Note: SortedArray tests skipped due to bug in sortedArray.hpp include paths

#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Poseidon/Foundation/Containers/SmallArray.hpp>
#include <Poseidon/Foundation/Containers/StaticArray.hpp>

// SmallArray Tests

TEST_CASE("SmallArray - Basic operations with inline storage", "[smallarray]")
{
    SECTION("Default constructor creates empty array")
    {
        SmallArray<int> arr;
        REQUIRE(arr.Size() == 0);
    }

    SECTION("Add elements to small storage")
    {
        SmallArray<int, 64> arr; // Can hold several ints inline

        arr.Add(10);
        arr.Add(20);
        arr.Add(30);

        REQUIRE(arr.Size() == 3);
        REQUIRE(arr[0] == 10);
        REQUIRE(arr[1] == 20);
        REQUIRE(arr[2] == 30);
    }

    SECTION("Overflow to large storage")
    {
        SmallArray<int, 32> arr; // Small inline size

        // Add enough elements to exceed small storage
        for (int i = 0; i < 20; i++)
        {
            arr.Add(i * 10);
        }

        REQUIRE(arr.Size() == 20);
        // Verify elements are accessible regardless of storage
        for (int i = 0; i < 20; i++)
        {
            REQUIRE(arr[i] == i * 10);
        }
    }

    SECTION("Delete from small storage")
    {
        SmallArray<int> arr;
        arr.Add(10);
        arr.Add(20);
        arr.Add(30);

        arr.Delete(1); // Remove 20

        REQUIRE(arr.Size() == 2);
        REQUIRE(arr[0] == 10);
        REQUIRE(arr[1] == 30);
    }

    SECTION("Clear empties array")
    {
        SmallArray<int> arr;
        arr.Add(1);
        arr.Add(2);
        arr.Add(3);

        arr.Clear();

        REQUIRE(arr.Size() == 0);
    }

    SECTION("Get and Set methods")
    {
        SmallArray<int> arr;
        arr.Add(100);
        arr.Add(200);

        REQUIRE(arr.Get(0) == 100);
        REQUIRE(arr.Get(1) == 200);

        arr.Set(0) = 150;
        REQUIRE(arr.Get(0) == 150);
    }
}

TEST_CASE("VerySmallArray - Fixed size inline storage", "[smallarray]")
{
    SECTION("Add up to capacity")
    {
        VerySmallArray<int, 64> arr;

        // Add several elements
        int idx1 = arr.Add(10);
        int idx2 = arr.Add(20);
        int idx3 = arr.Add(30);

        REQUIRE(idx1 >= 0);
        REQUIRE(idx2 >= 0);
        REQUIRE(idx3 >= 0);
        REQUIRE(arr.Size() == 3);
    }

    SECTION("Returns -1 when full")
    {
        VerySmallArray<int, 16> arr; // Very small capacity

        // Fill to capacity
        int result = 0;
        int count = 0;
        while (result >= 0 && count < 100)
        { // Safety limit
            result = arr.Add(count);
            if (result >= 0)
            {
                count++;
            }
        }

        // Last add should have failed
        REQUIRE(result == -1);
        REQUIRE(arr.Size() > 0);
    }

    SECTION("Delete and add again")
    {
        VerySmallArray<int> arr;
        arr.Add(10);
        arr.Add(20);
        arr.Add(30);

        arr.Delete(1); // Remove 20
        REQUIRE(arr.Size() == 2);

        arr.Add(40);
        REQUIRE(arr.Size() == 3);
        REQUIRE(arr[0] == 10);
        REQUIRE(arr[1] == 30);
        REQUIRE(arr[2] == 40);
    }
}

// StaticArray Tests

TEST_CASE("StaticArray - Uses static storage allocator", "[staticarray]")
{
    SECTION("Basic construction")
    {
        StaticArray<int> arr;
        REQUIRE(arr.Size() == 0);
    }

    SECTION("Add and access elements")
    {
        StaticArray<int> arr;
        arr.Add(100);
        arr.Add(200);
        arr.Add(300);

        REQUIRE(arr.Size() == 3);
        REQUIRE(arr[0] == 100);
        REQUIRE(arr[1] == 200);
        REQUIRE(arr[2] == 300);
    }

    SECTION("Uses AutoArray interface")
    {
        StaticArray<int> arr;
        arr.Resize(5);

        for (int i = 0; i < 5; i++)
        {
            arr[i] = i * 10;
        }

        REQUIRE(arr.Size() == 5);
        REQUIRE(arr[3] == 30);
    }
}

TEST_CASE("StaticArrayAuto - Stack allocated storage", "[staticarray]")
{
    SECTION("Uses provided memory buffer")
    {
        AUTO_STATIC_ARRAY(int, arr, 10);

        // Array should be pre-allocated
        REQUIRE(arr.MaxSize() >= 10);

        arr.Resize(10);
        for (int i = 0; i < 10; i++)
        {
            arr[i] = i * 5;
        }

        REQUIRE(arr.Size() == 10);
        REQUIRE(arr[5] == 25);
    }
}
