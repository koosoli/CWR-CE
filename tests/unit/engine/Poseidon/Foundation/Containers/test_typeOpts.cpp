// Unit tests for PoseidonBase typeOpts - placement new and ConstructAt helpers
// Tests placement new operators and ConstructAt template functions

#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Poseidon/Foundation/Containers/TypeOpts.hpp>
#include <stdint.h>
#include <stdlib.h>
#include <catch2/catch_message.hpp>
#include <new>

// Test Helper Classes

struct ConstructCounter
{
    static int defaultCount;
    static int copyCount;
    static int destructCount;

    static void Reset()
    {
        defaultCount = 0;
        copyCount = 0;
        destructCount = 0;
    }
};

int ConstructCounter::defaultCount = 0;
int ConstructCounter::copyCount = 0;
int ConstructCounter::destructCount = 0;

class TestClass
{
  public:
    int value;

    TestClass() : value(0) { ConstructCounter::defaultCount++; }

    TestClass(int v) : value(v) { ConstructCounter::defaultCount++; }

    TestClass(const TestClass& other) : value(other.value) { ConstructCounter::copyCount++; }

    ~TestClass() { ConstructCounter::destructCount++; }
};

// Placement New Tests

TEST_CASE("Placement new - Basic usage", "[typeopts][placement]")
{
    SECTION("Placement new on raw memory")
    {
        char buffer[sizeof(TestClass)];

        ConstructCounter::Reset();

        // Placement new - construct in pre-allocated memory
        TestClass* obj = new (buffer) TestClass(42);

        REQUIRE(ConstructCounter::defaultCount == 1);
        REQUIRE(obj->value == 42);

        // Must manually call destructor with placement new
        obj->~TestClass();
        REQUIRE(ConstructCounter::destructCount == 1);
    }

    SECTION("Placement new with default constructor")
    {
        char buffer[sizeof(TestClass)];

        ConstructCounter::Reset();

        TestClass* obj = new (buffer) TestClass();

        REQUIRE(ConstructCounter::defaultCount == 1);
        REQUIRE(obj->value == 0);

        obj->~TestClass();
    }

    SECTION("Placement new with copy constructor")
    {
        TestClass source(99);
        char buffer[sizeof(TestClass)];

        ConstructCounter::Reset();

        TestClass* obj = new (buffer) TestClass(source);

        REQUIRE(ConstructCounter::copyCount == 1);
        REQUIRE(obj->value == 99);

        obj->~TestClass();
    }
}

TEST_CASE("Placement new - Array construction", "[typeopts][placement][array]")
{
    SECTION("Placement new for array")
    {
        char buffer[sizeof(TestClass) * 3];

        ConstructCounter::Reset();

        // Note: Placement new for arrays is more complex
        // We construct each element individually
        TestClass* arr = (TestClass*)buffer;
        for (int i = 0; i < 3; i++)
        {
            new (&arr[i]) TestClass(i * 10);
        }

        REQUIRE(ConstructCounter::defaultCount == 3);
        REQUIRE(arr[0].value == 0);
        REQUIRE(arr[1].value == 10);
        REQUIRE(arr[2].value == 20);

        // Destruct in reverse order
        for (int i = 2; i >= 0; i--)
        {
            arr[i].~TestClass();
        }

        REQUIRE(ConstructCounter::destructCount == 3);
    }
}

// ConstructAt Tests

TEST_CASE("ConstructAt - Default construction", "[typeopts][constructat]")
{
    SECTION("ConstructAt with default constructor")
    {
        char buffer[sizeof(TestClass)];
        TestClass& obj = *(TestClass*)buffer;

        ConstructCounter::Reset();

        ConstructAt(obj);

        REQUIRE(ConstructCounter::defaultCount == 1);
        REQUIRE(obj.value == 0);

        obj.~TestClass();
    }

    SECTION("ConstructAt is equivalent to placement new")
    {
        char buffer1[sizeof(TestClass)];
        char buffer2[sizeof(TestClass)];

        ConstructCounter::Reset();

        // Using placement new
        TestClass* obj1 = new (buffer1) TestClass();

        // Using ConstructAt
        TestClass& obj2 = *(TestClass*)buffer2;
        ConstructAt(obj2);

        REQUIRE(ConstructCounter::defaultCount == 2);
        REQUIRE(obj1->value == obj2.value);

        obj1->~TestClass();
        obj2.~TestClass();
    }
}

TEST_CASE("ConstructAt - Copy construction", "[typeopts][constructat][copy]")
{
    SECTION("ConstructAt with copy constructor")
    {
        TestClass source(42);
        char buffer[sizeof(TestClass)];
        TestClass& dest = *(TestClass*)buffer;

        ConstructCounter::Reset();

        ConstructAt(dest, source);

        REQUIRE(ConstructCounter::copyCount == 1);
        REQUIRE(dest.value == 42);

        dest.~TestClass();
    }

    SECTION("ConstructAt copy is equivalent to placement new copy")
    {
        TestClass source(99);
        char buffer1[sizeof(TestClass)];
        char buffer2[sizeof(TestClass)];

        ConstructCounter::Reset();

        // Using placement new
        TestClass* obj1 = new (buffer1) TestClass(source);

        // Using ConstructAt
        TestClass& obj2 = *(TestClass*)buffer2;
        ConstructAt(obj2, source);

        REQUIRE(ConstructCounter::copyCount == 2);
        REQUIRE(obj1->value == obj2.value);
        REQUIRE(obj1->value == 99);

        obj1->~TestClass();
        obj2.~TestClass();
    }
}

// Integration with Container Operations

TEST_CASE("typeOpts - Integration patterns", "[typeopts][integration]")
{
    SECTION("Simulating container element construction")
    {
        // This is how containers like AutoArray use ConstructAt
        const int capacity = 5;
        char* buffer = (char*)malloc(sizeof(TestClass) * capacity);
        TestClass* array = (TestClass*)buffer;

        ConstructCounter::Reset();

        // Construct elements as they're added
        for (int i = 0; i < 3; i++)
        {
            ConstructAt(array[i], TestClass(i * 100));
        }

        REQUIRE(ConstructCounter::defaultCount == 3); // Temporaries
        REQUIRE(ConstructCounter::copyCount == 3);    // Copy construct into array
        REQUIRE(array[0].value == 0);
        REQUIRE(array[1].value == 100);
        REQUIRE(array[2].value == 200);

        // Destruct used elements
        for (int i = 0; i < 3; i++)
        {
            array[i].~TestClass();
        }

        free(buffer);
    }

    SECTION("Reusing memory slots")
    {
        char buffer[sizeof(TestClass)];
        TestClass& obj = *(TestClass*)buffer;

        // Construct
        ConstructAt(obj, TestClass(10));
        REQUIRE(obj.value == 10);

        // Destruct
        obj.~TestClass();

        ConstructCounter::Reset();

        // Reuse same memory
        ConstructAt(obj, TestClass(20));
        REQUIRE(obj.value == 20);
        REQUIRE(ConstructCounter::defaultCount == 1);
        REQUIRE(ConstructCounter::copyCount == 1);

        obj.~TestClass();
    }
}

// POD Types

TEST_CASE("typeOpts - POD types", "[typeopts][pod]")
{
    SECTION("Placement new with POD")
    {
        char buffer[sizeof(int)];

        int* value = new (buffer) int(42);

        REQUIRE(*value == 42);

        // POD types don't need destructor call
    }

    SECTION("ConstructAt with POD")
    {
        char buffer[sizeof(int)];
        int& value = *(int*)buffer;

        ConstructAt(value);
        // POD default construction leaves value uninitialized
        // (or zero-initialized depending on context)

        value = 100;
        REQUIRE(value == 100);
    }

    SECTION("Struct POD")
    {
        struct Point
        {
            int x;
            int y;
        };

        char buffer[sizeof(Point)];
        Point& p = *(Point*)buffer;

        ConstructAt(p);
        p.x = 10;
        p.y = 20;

        REQUIRE(p.x == 10);
        REQUIRE(p.y == 20);
    }
}

// Memory Alignment Tests

TEST_CASE("typeOpts - Memory alignment", "[typeopts][alignment]")
{
    SECTION("Placement new respects alignment")
    {
        // Allocate with proper alignment
        alignas(TestClass) char buffer[sizeof(TestClass)];

        TestClass* obj = new (buffer) TestClass(123);

        REQUIRE(obj->value == 123);

        // Check alignment (should be properly aligned)
        uintptr_t addr = reinterpret_cast<uintptr_t>(obj);
        REQUIRE(addr % alignof(TestClass) == 0);

        obj->~TestClass();
    }

    SECTION("ConstructAt with aligned memory")
    {
        alignas(TestClass) char buffer[sizeof(TestClass)];
        TestClass& obj = *(TestClass*)buffer;

        ConstructAt(obj, TestClass(456));

        REQUIRE(obj.value == 456);

        uintptr_t addr = reinterpret_cast<uintptr_t>(&obj);
        REQUIRE(addr % alignof(TestClass) == 0);

        obj.~TestClass();
    }
}

// Documentation

TEST_CASE("typeOpts - Usage documentation", "[typeopts][doc]")
{
    SECTION("When to use placement new")
    {
        INFO("Placement new constructs object in pre-allocated memory");
        INFO("Use when:");
        INFO("  - Managing memory manually (custom allocators)");
        INFO("  - Constructing in fixed buffer (stack, pool)");
        INFO("  - Implementing containers (AutoArray, etc.)");
        INFO("");
        INFO("Syntax: new(address) Type(args)");
        INFO("Must manually call destructor: obj->~Type()");

        REQUIRE(true);
    }

    SECTION("When to use ConstructAt")
    {
        INFO("ConstructAt is a wrapper around placement new");
        INFO("Takes reference instead of pointer");
        INFO("Used by Es type system macros");
        INFO("");
        INFO("ConstructAt(obj) = new(&obj) Type()");
        INFO("ConstructAt(obj, src) = new(&obj) Type(src)");

        REQUIRE(true);
    }

    SECTION("Integration with Es containers")
    {
        INFO("Es containers use ConstructAt internally");
        INFO("TypeDefines.hpp macros expand to ConstructAt calls");
        INFO("ModernTraits<T> uses these primitives");
        INFO("");
        INFO("Flow: Container -> ModernTraits -> ConstructAt -> placement new");

        REQUIRE(true);
    }
}

// Edge Cases

TEST_CASE("typeOpts - Edge cases", "[typeopts][edge]")
{
    SECTION("Multiple constructions at same address")
    {
        char buffer[sizeof(TestClass)];

        // First construction
        {
            TestClass* obj1 = new (buffer) TestClass(10);
            REQUIRE(obj1->value == 10);
            obj1->~TestClass();
        }

        // Second construction (reusing memory)
        {
            TestClass* obj2 = new (buffer) TestClass(20);
            REQUIRE(obj2->value == 20);
            obj2->~TestClass();
        }

        REQUIRE(true);
    }

    SECTION("Overlapping memory")
    {
        char buffer[sizeof(TestClass) * 2];

        TestClass* obj1 = new (buffer) TestClass(1);
        TestClass* obj2 = new (buffer + sizeof(TestClass)) TestClass(2);

        REQUIRE(obj1->value == 1);
        REQUIRE(obj2->value == 2);

        // Check they don't overlap
        REQUIRE(reinterpret_cast<char*>(obj2) >= reinterpret_cast<char*>(obj1) + sizeof(TestClass));

        obj1->~TestClass();
        obj2->~TestClass();
    }
}
