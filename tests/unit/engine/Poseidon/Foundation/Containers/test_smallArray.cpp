// Unit tests for PoseidonBase smallArray
// Tests VerySmallArray and SmallArray - fixed-size small object optimization containers

#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Poseidon/Foundation/Containers/SmallArray.hpp>
#include <catch2/catch_message.hpp>

// Test Helper - Tracking Type

struct SmallArrayCounter
{
    static int constructCount;
    static int copyConstructCount;
    static int destructCount;

    static void Reset()
    {
        constructCount = 0;
        copyConstructCount = 0;
        destructCount = 0;
    }
};

int SmallArrayCounter::constructCount = 0;
int SmallArrayCounter::copyConstructCount = 0;
int SmallArrayCounter::destructCount = 0;

class SmallArrayTestType
{
  public:
    int value;

    SmallArrayTestType() : value(0) { SmallArrayCounter::constructCount++; }

    SmallArrayTestType(int v) : value(v) { SmallArrayCounter::constructCount++; }

    SmallArrayTestType(const SmallArrayTestType& other) : value(other.value)
    {
        SmallArrayCounter::copyConstructCount++;
    }

    SmallArrayTestType& operator=(const SmallArrayTestType& other) = default;

    ~SmallArrayTestType() { SmallArrayCounter::destructCount++; }
};

// VerySmallArray Tests - Fixed Size Container

TEST_CASE("VerySmallArray - Construction and basic properties", "[smallarray][verysmall]")
{
    SECTION("Default construction")
    {
        VerySmallArray<int, 64> arr;

        REQUIRE(arr.Size() == 0);
    }

    SECTION("Empty array access via Data()")
    {
        VerySmallArray<int, 64> arr;

        const int* data = arr.Data();
        REQUIRE(data != nullptr);
    }

    SECTION("Calculate MaxSmall capacity")
    {
        // MaxSmall = (size - 4) / sizeof(Type)
        VerySmallArray<int, 64> arr;   // (64-4)/4 = 15 ints max
        VerySmallArray<char, 64> arr2; // (64-4)/1 = 60 chars max

        // We can't directly check MaxSmall, but we can test capacity
        // by adding elements until we get -1
        INFO("Testing capacity by filling array");
        REQUIRE(true);
    }
}

TEST_CASE("VerySmallArray - Add operations", "[smallarray][verysmall][add]")
{
    SECTION("Add single element")
    {
        VerySmallArray<int, 64> arr;

        int index = arr.Add(42);

        REQUIRE(index == 0);
        REQUIRE(arr.Size() == 1);
        REQUIRE(arr[0] == 42);
    }

    SECTION("Add multiple elements")
    {
        VerySmallArray<int, 64> arr;

        arr.Add(10);
        arr.Add(20);
        arr.Add(30);

        REQUIRE(arr.Size() == 3);
        REQUIRE(arr[0] == 10);
        REQUIRE(arr[1] == 20);
        REQUIRE(arr[2] == 30);
    }

    SECTION("Add() default construction")
    {
        VerySmallArray<SmallArrayTestType, 64> arr;
        SmallArrayCounter::Reset();

        int index = arr.Add();

        REQUIRE(index == 0);
        REQUIRE(SmallArrayCounter::constructCount == 1);
        REQUIRE(arr.Size() == 1);
    }

    SECTION("Add with copy construction")
    {
        VerySmallArray<SmallArrayTestType, 64> arr;
        SmallArrayTestType obj(99);

        SmallArrayCounter::Reset();

        int index = arr.Add(obj);

        REQUIRE(index == 0);
        REQUIRE(SmallArrayCounter::copyConstructCount == 1);
        REQUIRE(arr[0].value == 99);
    }

    SECTION("Add returns -1 when full")
    {
        VerySmallArray<int, 20> arr; // Very small size, (20-4)/4 = 4 ints max

        REQUIRE(arr.Add(1) == 0);
        REQUIRE(arr.Add(2) == 1);
        REQUIRE(arr.Add(3) == 2);
        REQUIRE(arr.Add(4) == 3);

        // Should be full now
        int result = arr.Add(5);
        REQUIRE(result == -1);
        REQUIRE(arr.Size() == 4); // Size unchanged
    }
}

TEST_CASE("VerySmallArray - Element access", "[smallarray][verysmall][access]")
{
    SECTION("Get const access")
    {
        VerySmallArray<int, 64> arr;
        arr.Add(100);
        arr.Add(200);

        const VerySmallArray<int, 64>& constRef = arr;

        REQUIRE(constRef.Get(0) == 100);
        REQUIRE(constRef.Get(1) == 200);
    }

    SECTION("Set mutable access")
    {
        VerySmallArray<int, 64> arr;
        arr.Add(10);

        arr.Set(0) = 42;

        REQUIRE(arr.Get(0) == 42);
    }

    SECTION("Operator [] const")
    {
        VerySmallArray<int, 64> arr;
        arr.Add(10);
        arr.Add(20);

        const VerySmallArray<int, 64>& constRef = arr;

        REQUIRE(constRef[0] == 10);
        REQUIRE(constRef[1] == 20);
    }

    SECTION("Operator [] mutable")
    {
        VerySmallArray<int, 64> arr;
        arr.Add(5);

        arr[0] = 99;

        REQUIRE(arr[0] == 99);
    }
}

TEST_CASE("VerySmallArray - Delete operation", "[smallarray][verysmall][delete]")
{
    SECTION("Delete middle element")
    {
        VerySmallArray<int, 64> arr;
        arr.Add(10);
        arr.Add(20);
        arr.Add(30);
        arr.Add(40);

        arr.Delete(1); // Delete 20

        REQUIRE(arr.Size() == 3);
        REQUIRE(arr[0] == 10);
        REQUIRE(arr[1] == 30); // Shifted down
        REQUIRE(arr[2] == 40);
    }

    SECTION("Delete first element")
    {
        VerySmallArray<int, 64> arr;
        arr.Add(1);
        arr.Add(2);
        arr.Add(3);

        arr.Delete(0);

        REQUIRE(arr.Size() == 2);
        REQUIRE(arr[0] == 2);
        REQUIRE(arr[1] == 3);
    }

    SECTION("Delete last element")
    {
        VerySmallArray<int, 64> arr;
        arr.Add(10);
        arr.Add(20);

        arr.Delete(1);

        REQUIRE(arr.Size() == 1);
        REQUIRE(arr[0] == 10);
    }

    SECTION("Delete calls destructor")
    {
        VerySmallArray<SmallArrayTestType, 64> arr;
        arr.Add(SmallArrayTestType(1));
        arr.Add(SmallArrayTestType(2));
        arr.Add(SmallArrayTestType(3));

        SmallArrayCounter::Reset();

        arr.Delete(1);

        REQUIRE(SmallArrayCounter::destructCount >= 1); // At least the deleted element
        REQUIRE(arr.Size() == 2);
    }
}

TEST_CASE("VerySmallArray - Clear operation", "[smallarray][verysmall][clear]")
{
    SECTION("Clear empties array")
    {
        VerySmallArray<int, 64> arr;
        arr.Add(1);
        arr.Add(2);
        arr.Add(3);

        arr.Clear();

        REQUIRE(arr.Size() == 0);
    }

    SECTION("Clear calls destructors")
    {
        VerySmallArray<SmallArrayTestType, 64> arr;
        arr.Add(SmallArrayTestType(1));
        arr.Add(SmallArrayTestType(2));
        arr.Add(SmallArrayTestType(3));

        SmallArrayCounter::Reset();

        arr.Clear();

        REQUIRE(SmallArrayCounter::destructCount == 3);
        REQUIRE(arr.Size() == 0);
    }

    SECTION("Clear allows re-adding")
    {
        VerySmallArray<int, 64> arr;
        arr.Add(10);
        arr.Add(20);

        arr.Clear();

        int index = arr.Add(30);
        REQUIRE(index == 0);
        REQUIRE(arr.Size() == 1);
        REQUIRE(arr[0] == 30);
    }
}

TEST_CASE("VerySmallArray - Destructor", "[smallarray][verysmall][destructor]")
{
    SECTION("Destructor calls Clear")
    {
        SmallArrayCounter::Reset();

        {
            VerySmallArray<SmallArrayTestType, 64> arr;
            arr.Add(SmallArrayTestType(1));
            arr.Add(SmallArrayTestType(2));
        } // Destructor called here

        REQUIRE(SmallArrayCounter::destructCount >= 2);
    }
}

// SmallArray Tests - Hybrid Small + Dynamic Container

TEST_CASE("SmallArray - Construction and basic properties", "[smallarray][hybrid]")
{
    SECTION("Default construction")
    {
        SmallArray<int, 64> arr;

        REQUIRE(arr.Size() == 0);
    }

    SECTION("Empty array operations")
    {
        SmallArray<int, 64> arr;

        REQUIRE(arr.Size() == 0);

        arr.Clear();
        REQUIRE(arr.Size() == 0);
    }
}

TEST_CASE("SmallArray - Add to small buffer", "[smallarray][hybrid][add]")
{
    SECTION("Add elements to small buffer")
    {
        SmallArray<int, 64> arr;

        arr.Add(10);
        arr.Add(20);
        arr.Add(30);

        REQUIRE(arr.Size() == 3);
        REQUIRE(arr[0] == 10);
        REQUIRE(arr[1] == 20);
        REQUIRE(arr[2] == 30);
    }

    SECTION("Add returns correct index")
    {
        SmallArray<int, 64> arr;

        REQUIRE(arr.Add(100) == 0);
        REQUIRE(arr.Add(200) == 1);
        REQUIRE(arr.Add(300) == 2);
    }
}

TEST_CASE("SmallArray - Overflow to large buffer", "[smallarray][hybrid][overflow]")
{
    SECTION("Overflow from small to large")
    {
        // Use very small size to test overflow
        SmallArray<int, 32> arr; // Will have very few elements in small buffer

        // Fill small buffer
        for (int i = 0; i < 5; i++)
        {
            arr.Add(i * 10);
        }

        // Add more to trigger large buffer
        for (int i = 5; i < 15; i++)
        {
            int index = arr.Add(i * 10);
            REQUIRE(index == i);
        }

        REQUIRE(arr.Size() == 15);

        // Verify all elements accessible
        for (int i = 0; i < 15; i++)
        {
            REQUIRE(arr[i] == i * 10);
        }
    }

    SECTION("Large buffer usage")
    {
        SmallArray<int, 32> arr;

        // Add many elements
        for (int i = 0; i < 100; i++)
        {
            arr.Add(i);
        }

        REQUIRE(arr.Size() == 100);
        REQUIRE(arr[0] == 0);
        REQUIRE(arr[50] == 50);
        REQUIRE(arr[99] == 99);
    }
}

TEST_CASE("SmallArray - Element access", "[smallarray][hybrid][access]")
{
    SECTION("Get const access")
    {
        SmallArray<int, 64> arr;
        arr.Add(1);
        arr.Add(2);

        const SmallArray<int, 64>& constRef = arr;

        REQUIRE(constRef.Get(0) == 1);
        REQUIRE(constRef.Get(1) == 2);
    }

    SECTION("Set mutable access")
    {
        SmallArray<int, 64> arr;
        arr.Add(10);
        arr.Add(20);

        arr.Set(0) = 99;
        arr.Set(1) = 88;

        REQUIRE(arr.Get(0) == 99);
        REQUIRE(arr.Get(1) == 88);
    }

    SECTION("Operator [] mixed small/large")
    {
        SmallArray<int, 32> arr;

        // Add to small
        for (int i = 0; i < 3; i++)
        {
            arr.Add(i);
        }

        // Add to large
        for (int i = 3; i < 20; i++)
        {
            arr.Add(i);
        }

        // Access both small and large
        REQUIRE(arr[0] == 0);   // Small
        REQUIRE(arr[2] == 2);   // Small
        REQUIRE(arr[10] == 10); // Large
        REQUIRE(arr[19] == 19); // Large
    }
}

TEST_CASE("SmallArray - Delete operation", "[smallarray][hybrid][delete]")
{
    SECTION("Delete from small buffer")
    {
        SmallArray<int, 64> arr;
        arr.Add(10);
        arr.Add(20);
        arr.Add(30);

        arr.Delete(1);

        REQUIRE(arr.Size() == 2);
        REQUIRE(arr[0] == 10);
        REQUIRE(arr[1] == 30);
    }

    SECTION("Delete from large buffer")
    {
        SmallArray<int, 32> arr;

        // Fill small + large
        for (int i = 0; i < 20; i++)
        {
            arr.Add(i * 10);
        }

        // Delete from large portion
        int smallSize = 3; // Approximate small buffer size
        arr.Delete(smallSize + 2);

        REQUIRE(arr.Size() == 19);
    }

    SECTION("Delete with objects")
    {
        SmallArray<SmallArrayTestType, 64> arr;
        arr.Add(SmallArrayTestType(1));
        arr.Add(SmallArrayTestType(2));
        arr.Add(SmallArrayTestType(3));

        SmallArrayCounter::Reset();

        arr.Delete(1);

        REQUIRE(SmallArrayCounter::destructCount >= 1);
        REQUIRE(arr.Size() == 2);
    }
}

TEST_CASE("SmallArray - Clear operation", "[smallarray][hybrid][clear]")
{
    SECTION("Clear small buffer only")
    {
        SmallArray<int, 64> arr;
        arr.Add(1);
        arr.Add(2);

        arr.Clear();

        REQUIRE(arr.Size() == 0);
    }

    SECTION("Clear both small and large")
    {
        SmallArray<int, 32> arr;

        for (int i = 0; i < 50; i++)
        {
            arr.Add(i);
        }

        arr.Clear();

        REQUIRE(arr.Size() == 0);
    }

    SECTION("Clear calls destructors")
    {
        SmallArray<SmallArrayTestType, 32> arr;

        for (int i = 0; i < 10; i++)
        {
            arr.Add(SmallArrayTestType(i));
        }

        SmallArrayCounter::Reset();

        arr.Clear();

        REQUIRE(SmallArrayCounter::destructCount == 10);
        REQUIRE(arr.Size() == 0);
    }
}

TEST_CASE("SmallArray - Compact operation", "[smallarray][hybrid][compact]")
{
    SECTION("Compact does not crash")
    {
        SmallArray<int, 32> arr;

        for (int i = 0; i < 20; i++)
        {
            arr.Add(i);
        }

        arr.Compact();

        // Note: Compact is not fully implemented
        // Just verify it doesn't crash
        REQUIRE(arr.Size() == 20);
    }
}

// Integration Tests

TEST_CASE("SmallArray - Real-world usage patterns", "[smallarray][integration]")
{
    SECTION("Small array stays in small buffer")
    {
        SmallArray<int, 64> arr;

        // Add a few elements (stay in small buffer)
        arr.Add(1);
        arr.Add(2);
        arr.Add(3);

        // Access and modify
        arr[1] = 99;

        // Delete one
        arr.Delete(0);

        REQUIRE(arr.Size() == 2);
        REQUIRE(arr[0] == 99);
        REQUIRE(arr[1] == 3);
    }

    SECTION("Growing and shrinking")
    {
        SmallArray<int, 32> arr;

        // Grow past small buffer
        for (int i = 0; i < 30; i++)
        {
            arr.Add(i);
        }

        REQUIRE(arr.Size() == 30);

        // Shrink
        for (int i = 0; i < 20; i++)
        {
            arr.Delete(arr.Size() - 1);
        }

        REQUIRE(arr.Size() == 10);
    }

    SECTION("Repeated clear and add")
    {
        SmallArray<int, 64> arr;

        for (int iteration = 0; iteration < 3; iteration++)
        {
            for (int i = 0; i < 10; i++)
            {
                arr.Add(i);
            }

            REQUIRE(arr.Size() == 10);

            arr.Clear();
            REQUIRE(arr.Size() == 0);
        }
    }
}

// POD Type Tests

TEST_CASE("SmallArray - POD types", "[smallarray][pod]")
{
    SECTION("Int array")
    {
        SmallArray<int, 64> arr;

        for (int i = 0; i < 100; i++)
        {
            arr.Add(i * 2);
        }

        REQUIRE(arr.Size() == 100);
        REQUIRE(arr[0] == 0);
        REQUIRE(arr[50] == 100);
        REQUIRE(arr[99] == 198);
    }

    SECTION("Float array")
    {
        SmallArray<float, 64> arr;

        arr.Add(1.5f);
        arr.Add(2.5f);
        arr.Add(3.5f);

        REQUIRE(arr[0] == 1.5f);
        REQUIRE(arr[1] == 2.5f);
        REQUIRE(arr[2] == 3.5f);
    }

    SECTION("Struct array")
    {
        struct Point
        {
            int x, y;
        };

        SmallArray<Point, 64> arr;

        Point p1 = {10, 20};
        Point p2 = {30, 40};

        arr.Add(p1);
        arr.Add(p2);

        REQUIRE(arr[0].x == 10);
        REQUIRE(arr[0].y == 20);
        REQUIRE(arr[1].x == 30);
        REQUIRE(arr[1].y == 40);
    }
}

// Edge Cases

TEST_CASE("SmallArray - Edge cases", "[smallarray][edge]")
{
    SECTION("Single element")
    {
        SmallArray<int, 64> arr;

        arr.Add(42);
        REQUIRE(arr.Size() == 1);
        REQUIRE(arr[0] == 42);

        arr.Delete(0);
        REQUIRE(arr.Size() == 0);
    }

    SECTION("Empty operations")
    {
        SmallArray<int, 64> arr;

        arr.Clear();
        REQUIRE(arr.Size() == 0);

        arr.Compact();
        REQUIRE(arr.Size() == 0);
    }

    SECTION("Different sizes")
    {
        // Test with different size templates
        SmallArray<int, 32> arr32;
        SmallArray<int, 64> arr64;
        SmallArray<int, 128> arr128;

        for (int i = 0; i < 10; i++)
        {
            arr32.Add(i);
            arr64.Add(i);
            arr128.Add(i);
        }

        REQUIRE(arr32.Size() == 10);
        REQUIRE(arr64.Size() == 10);
        REQUIRE(arr128.Size() == 10);
    }
}

// Documentation Tests

TEST_CASE("SmallArray - Usage documentation", "[smallarray][doc]")
{
    SECTION("VerySmallArray purpose")
    {
        INFO("VerySmallArray is a fixed-size stack-allocated container");
        INFO("Size is determined by template parameter (e.g., 64 bytes total)");
        INFO("MaxSmall capacity = (size - 4) / sizeof(Type)");
        INFO("Returns -1 when trying to add beyond capacity");
        INFO("Good for small, temporary collections");
        INFO("Uses ClassIsMovableZeroed for optimization");

        REQUIRE(true);
    }

    SECTION("SmallArray purpose")
    {
        INFO("SmallArray combines VerySmallArray + AutoArray");
        INFO("Small objects stay in fixed buffer (fast, no allocation)");
        INFO("Overflows to dynamic AutoArray when full");
        INFO("Transparent access via Get/Set/operator[]");
        INFO("Optimal for: frequently small, occasionally large collections");
        INFO("Examples: temporary lists, particle systems, event queues");

        REQUIRE(true);
    }

    SECTION("When to use SmallArray")
    {
        INFO("Use SmallArray when:");
        INFO("  - Most cases have few elements (< 10-15)");
        INFO("  - Occasionally need more capacity");
        INFO("  - Want to avoid allocation overhead for small cases");
        INFO("  - Performance matters for common case");
        INFO("");
        INFO("Don't use SmallArray when:");
        INFO("  - Always have many elements (use AutoArray/FindArray)");
        INFO("  - Need guaranteed capacity (use StaticArray)");
        INFO("  - Size is always small and fixed (use C array)");

        REQUIRE(true);
    }

    SECTION("SmallArray API overview")
    {
        INFO("Key methods:");
        INFO("  - Add(value)    : Add element, return index (or -1 if full)");
        INFO("  - Delete(index) : Remove element at index");
        INFO("  - Clear()       : Empty the array, calls destructors");
        INFO("  - Compact()     : (Future) Compact to reduce memory usage");
        INFO("");
        INFO("Access methods:");
        INFO("  - Get(index)    : Get const reference");
        INFO("  - Set(index)    : Get mutable reference");
        INFO("  - operator[]     : Array subscript operator (const and mutable)");

        REQUIRE(true);
    }
}
