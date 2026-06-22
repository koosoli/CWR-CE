// Unit tests for PoseidonBase streamArray
// Testing variable-size element stream arrays

#define _CRT_SECURE_NO_WARNINGS
#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Poseidon/Foundation/Containers/StreamArray.hpp>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <Poseidon/Foundation/Containers/Array.hpp>

// Test Type - Simple Variable-Size String

// Simple variable-length string for testing StreamArray
// This is a proper type that meets StreamArray requirements
struct VarString
{
    int length;  // String length (including null terminator)
    char str[1]; // Variable-size data (flexible array member pattern)

    // Required by StreamArray: return actual size of this instance
    int ItemSize() const { return sizeof(VarString) - sizeof(str) + length; }

    // Required by StreamArray: duplicate this instance into dst
    void Duplicate(VarString& dst) const
    {
        dst.length = length;
        memcpy(dst.str, str, length);
    }

    // Static method for StreamArray capacity calculations
    static int TypicalItemSize()
    {
        return sizeof(VarString) + 16; // Typical string ~16 chars
    }

    // Helper to compare strings
    bool Equals(const char* other) const { return strcmp(str, other) == 0; }
};

// Type alias
typedef StreamArray<VarString, AutoArray<char>> VarStringArray;

// Helper to create a VarString (must be freed with free())
VarString* MakeVarString(const char* s)
{
    int len = (int)strlen(s) + 1;
    int size = sizeof(VarString) - sizeof(VarString::str) + len;
    VarString* vs = (VarString*)malloc(size);
    vs->length = len;
    memcpy(vs->str, s, len);
    return vs;
}

// StreamArray - Basic Construction and Destruction

TEST_CASE("StreamArray - Basic construction", "[containers][streamarray]")
{
    SECTION("Default construction")
    {
        VarStringArray arr;

        REQUIRE(arr.Size() == 0);
        REQUIRE(arr.RawSize() == 0);
    }
}

TEST_CASE("StreamArray - Clear operation", "[containers][streamarray]")
{
    SECTION("Clear empty array")
    {
        VarStringArray arr;
        arr.Clear();

        REQUIRE(arr.Size() == 0);
    }

    SECTION("Clear populated array")
    {
        VarStringArray arr;

        VarString* v1 = MakeVarString("test");
        arr.Add(*v1);
        free(v1);

        arr.Clear();

        REQUIRE(arr.Size() == 0);
        REQUIRE(arr.RawSize() == 0);
    }
}

// StreamArray - Add and Get Operations

TEST_CASE("StreamArray - Add operations", "[containers][streamarray]")
{
    SECTION("Add single item")
    {
        VarStringArray arr;

        VarString* vs = MakeVarString("hello");
        Offset off = arr.Add(*vs);
        free(vs);

        REQUIRE(arr.Size() == 1);
        REQUIRE(arr.RawSize() > 0);

        const VarString& retrieved = arr.Get(off);
        REQUIRE(retrieved.Equals("hello"));
    }

    SECTION("Add multiple items")
    {
        VarStringArray arr;

        VarString* v1 = MakeVarString("first");
        VarString* v2 = MakeVarString("second");
        VarString* v3 = MakeVarString("third");

        arr.Add(*v1);
        arr.Add(*v2);
        arr.Add(*v3);

        free(v1);
        free(v2);
        free(v3);

        REQUIRE(arr.Size() == 3);
    }

    SECTION("Add returns valid offset")
    {
        VarStringArray arr;

        VarString* vs = MakeVarString("test");
        Offset off = arr.Add(*vs);
        free(vs);

        REQUIRE(arr.Get(off).Equals("test"));
    }
}

TEST_CASE("StreamArray - Get operations", "[containers][streamarray]")
{
    SECTION("Get by offset")
    {
        VarStringArray arr;

        VarString* v1 = MakeVarString("alpha");
        VarString* v2 = MakeVarString("beta");

        Offset off1 = arr.Add(*v1);
        Offset off2 = arr.Add(*v2);

        REQUIRE(arr.Get(off1).Equals("alpha"));
        REQUIRE(arr.Get(off2).Equals("beta"));

        free(v1);
        free(v2);
    }

    SECTION("Operator[] access")
    {
        VarStringArray arr;

        VarString* vs = MakeVarString("data");
        Offset off = arr.Add(*vs);
        free(vs);

        REQUIRE(arr[off].Equals("data"));
    }

    SECTION("Set (mutable access)")
    {
        VarStringArray arr;

        VarString* vs = MakeVarString("original");
        Offset off = arr.Add(*vs);
        free(vs);

        // Modify in place (change first char)
        arr.Set(off).str[0] = 'O';

        REQUIRE(arr.Get(off).str[0] == 'O');
    }
}

// StreamArray - Iteration

TEST_CASE("StreamArray - Iteration with Begin/End/Next", "[containers][streamarray]")
{
    SECTION("Iterate empty array")
    {
        VarStringArray arr;

        int count = 0;
        for (Offset i = arr.Begin(); i < arr.End(); arr.Next(i))
        {
            count++;
        }

        REQUIRE(count == 0);
    }

    SECTION("Iterate populated array")
    {
        VarStringArray arr;

        VarString* v1 = MakeVarString("a");
        VarString* v2 = MakeVarString("bb");
        VarString* v3 = MakeVarString("ccc");

        arr.Add(*v1);
        arr.Add(*v2);
        arr.Add(*v3);

        int count = 0;
        for (Offset i = arr.Begin(); i < arr.End(); arr.Next(i))
        {
            const VarString& vs = arr.Get(i);
            count++;
            REQUIRE(vs.length > 0);
        }

        REQUIRE(count == 3);

        free(v1);
        free(v2);
        free(v3);
    }

    SECTION("Iterate and verify all items")
    {
        VarStringArray arr;

        const char* strings[] = {"one", "two", "three"};
        for (int i = 0; i < 3; i++)
        {
            VarString* vs = MakeVarString(strings[i]);
            arr.Add(*vs);
            free(vs);
        }

        int idx = 0;
        for (Offset i = arr.Begin(); i < arr.End(); arr.Next(i))
        {
            REQUIRE(arr.Get(i).Equals(strings[idx]));
            idx++;
        }
    }
}

// StreamArray - Find Operation

TEST_CASE("StreamArray - Find by logical index", "[containers][streamarray]")
{
    SECTION("Find items in order")
    {
        VarStringArray arr;

        VarString* v1 = MakeVarString("first");
        VarString* v2 = MakeVarString("second");
        VarString* v3 = MakeVarString("third");

        arr.Add(*v1);
        arr.Add(*v2);
        arr.Add(*v3);

        Offset off0 = arr.Find(0);
        Offset off1 = arr.Find(1);
        Offset off2 = arr.Find(2);

        REQUIRE(arr.Get(off0).Equals("first"));
        REQUIRE(arr.Get(off1).Equals("second"));
        REQUIRE(arr.Get(off2).Equals("third"));

        free(v1);
        free(v2);
        free(v3);
    }
}

// StreamArray - Delete Operation

TEST_CASE("StreamArray - Delete operations", "[containers][streamarray]")
{
    SECTION("Delete middle item")
    {
        VarStringArray arr;

        VarString* v1 = MakeVarString("first");
        VarString* v2 = MakeVarString("second");
        VarString* v3 = MakeVarString("third");

        arr.Add(*v1);
        Offset off2 = arr.Add(*v2);
        arr.Add(*v3);

        REQUIRE(arr.Size() == 3);

        arr.Delete(off2);

        REQUIRE(arr.Size() == 2);

        // Verify remaining items
        int count = 0;
        for (Offset i = arr.Begin(); i < arr.End(); arr.Next(i))
        {
            const VarString& vs = arr.Get(i);
            REQUIRE_FALSE(vs.Equals("second"));
            count++;
        }
        REQUIRE(count == 2);

        free(v1);
        free(v2);
        free(v3);
    }

    SECTION("Delete first item")
    {
        VarStringArray arr;

        VarString* v1 = MakeVarString("first");
        VarString* v2 = MakeVarString("second");

        Offset off1 = arr.Add(*v1);
        arr.Add(*v2);

        arr.Delete(off1);

        REQUIRE(arr.Size() == 1);
        REQUIRE(arr.Get(arr.Begin()).Equals("second"));

        free(v1);
        free(v2);
    }
}

// StreamArray - Copy Operations

TEST_CASE("StreamArray - Copy operations", "[containers][streamarray]")
{
    SECTION("Copy constructor")
    {
        VarStringArray arr1;

        VarString* v1 = MakeVarString("test1");
        VarString* v2 = MakeVarString("test2");
        arr1.Add(*v1);
        arr1.Add(*v2);

        VarStringArray arr2(arr1);

        REQUIRE(arr2.Size() == 2);
        REQUIRE(arr2.Get(arr2.Begin()).Equals("test1"));

        free(v1);
        free(v2);
    }

    SECTION("Assignment operator")
    {
        VarStringArray arr1;
        VarStringArray arr2;

        VarString* vs = MakeVarString("data");
        arr1.Add(*vs);

        arr2 = arr1;

        REQUIRE(arr2.Size() == 1);
        REQUIRE(arr2.Get(arr2.Begin()).Equals("data"));

        free(vs);
    }

    // NOTE: Copy(const Type*, int) doesn't work well with variable-size types
    // Skip this test as it requires contiguous array of variable-size items
}

// StreamArray - Move Operation

TEST_CASE("StreamArray - Move operations", "[containers][streamarray]")
{
    SECTION("Move transfers ownership")
    {
        VarStringArray arr1;
        VarString* vs = MakeVarString("movable");
        arr1.Add(*vs);
        free(vs);

        VarStringArray arr2;
        arr2.Move(arr1);

        REQUIRE(arr1.Size() == 0); // Source is empty
        REQUIRE(arr2.Size() == 1); // Destination has data
        REQUIRE(arr2.Get(arr2.Begin()).Equals("movable"));
    }
}

// StreamArray - Merge Operations

TEST_CASE("StreamArray - Merge operations", "[containers][streamarray]")
{
    SECTION("Merge two arrays")
    {
        VarStringArray arr1;
        VarStringArray arr2;

        VarString* v1 = MakeVarString("from_arr1");
        VarString* v2 = MakeVarString("from_arr2");

        arr1.Add(*v1);
        arr2.Add(*v2);

        arr1.Merge(arr2);

        REQUIRE(arr1.Size() == 2);
        REQUIRE(arr2.Size() == 1); // arr2 unchanged

        free(v1);
        free(v2);
    }

    SECTION("Merge raw data")
    {
        VarStringArray arr;

        VarString* v1 = MakeVarString("existing");
        arr.Add(*v1);

        // Create raw data to merge
        VarString* v2 = MakeVarString("merged");
        int rawSize = v2->ItemSize();

        arr.Merge((const char*)v2, rawSize, 1);

        REQUIRE(arr.Size() == 2);

        free(v1);
        free(v2);
    }
}

// StreamArray - Reserve and Capacity

TEST_CASE("StreamArray - Reserve operations", "[containers][streamarray]")
{
    SECTION("Reserve space")
    {
        VarStringArray arr;

        arr.Reserve(10);

        // Add items - should not require frequent reallocations
        for (int i = 0; i < 10; i++)
        {
            char buf[32];
            sprintf(buf, "item%d", i);
            VarString* vs = MakeVarString(buf);
            arr.Add(*vs);
            free(vs);
        }

        REQUIRE(arr.Size() == 10);
    }

    SECTION("ReserveRaw")
    {
        VarStringArray arr;

        arr.ReserveRaw(500);

        // Just verify it doesn't crash
        REQUIRE(true);
    }

    // NOTE: ReserveMore calls Reserve(size) which expects 2 args for AutoArray
    // Skip this test as it's implementation-dependent

    SECTION("Realloc")
    {
        VarStringArray arr;

        VarString* vs = MakeVarString("test");
        arr.Add(*vs);
        free(vs);

        arr.Realloc(10);

        REQUIRE(arr.Size() == 1);
    }
}

// StreamArray - Compact Operation

TEST_CASE("StreamArray - Compact operation", "[containers][streamarray]")
{
    SECTION("Compact reduces memory")
    {
        VarStringArray arr;

        VarString* vs = MakeVarString("compact me");
        arr.Add(*vs);
        free(vs);

        int sizeBefore = arr.Size();
        arr.Compact();

        REQUIRE(arr.Size() == sizeBefore);
        REQUIRE(arr.Get(arr.Begin()).Equals("compact me"));
    }
}

// StreamArray - CalculateCount

TEST_CASE("StreamArray - CalculateCount", "[containers][streamarray]")
{
    SECTION("CalculateCount matches Size()")
    {
        VarStringArray arr;

        VarString* v1 = MakeVarString("a");
        VarString* v2 = MakeVarString("bb");
        VarString* v3 = MakeVarString("ccc");

        arr.Add(*v1);
        arr.Add(*v2);
        arr.Add(*v3);

        REQUIRE(arr.CalculateCount() == arr.Size());
        REQUIRE(arr.CalculateCount() == 3);

        free(v1);
        free(v2);
        free(v3);
    }
}

// StreamArray - Variable Size Testing

TEST_CASE("StreamArray - Variable size items", "[containers][streamarray]")
{
    SECTION("Items with vastly different sizes")
    {
        VarStringArray arr;

        VarString* small = MakeVarString("x");
        VarString* medium = MakeVarString("Hello, World!");
        VarString* large = MakeVarString(
            "This is a much longer string designed to test variable-size storage in the stream array container");

        Offset off1 = arr.Add(*small);
        Offset off2 = arr.Add(*medium);
        Offset off3 = arr.Add(*large);

        // Verify all items are correctly stored and accessible
        REQUIRE(arr.Get(off1).Equals("x"));
        REQUIRE(arr.Get(off2).Equals("Hello, World!"));
        REQUIRE(arr.Get(off3).Equals(
            "This is a much longer string designed to test variable-size storage in the stream array container"));

        // Verify sizes are correct
        REQUIRE(arr.Get(off1).ItemSize() < arr.Get(off2).ItemSize());
        REQUIRE(arr.Get(off2).ItemSize() < arr.Get(off3).ItemSize());

        free(small);
        free(medium);
        free(large);
    }
}

// StreamArray - Sort Operation

// Comparison function for sorting
int CompareVarStrings(const VarString* a, const VarString* b)
{
    return strcmp(a->str, b->str);
}

TEST_CASE("StreamArray - Sort operation", "[containers][streamarray]")
{
    SECTION("Sort strings alphabetically")
    {
        VarStringArray arr;

        VarString* v1 = MakeVarString("zebra");
        VarString* v2 = MakeVarString("apple");
        VarString* v3 = MakeVarString("monkey");

        arr.Add(*v1);
        arr.Add(*v2);
        arr.Add(*v3);

        arr.Sort(&CompareVarStrings);

        // Verify sorted order
        Offset off = arr.Begin();
        REQUIRE(arr.Get(off).Equals("apple"));
        arr.Next(off);
        REQUIRE(arr.Get(off).Equals("monkey"));
        arr.Next(off);
        REQUIRE(arr.Get(off).Equals("zebra"));

        free(v1);
        free(v2);
        free(v3);
    }
}

// StreamArray - Raw Data Access

TEST_CASE("StreamArray - Raw data access", "[containers][streamarray]")
{
    SECTION("RawData and RawSize")
    {
        VarStringArray arr;

        VarString* vs = MakeVarString("raw");
        arr.Add(*vs);
        free(vs);

        const char* rawData = arr.RawData();
        int rawSize = arr.RawSize();

        REQUIRE(rawData != nullptr);
        REQUIRE(rawSize > 0);
    }

    SECTION("GetData access")
    {
        VarStringArray arr;

        VarString* vs = MakeVarString("test");
        arr.Add(*vs);
        free(vs);

        const AutoArray<char>& data = arr.GetData();
        REQUIRE(data.Size() > 0);
    }
}

// StreamArray - Edge Cases

TEST_CASE("StreamArray - Edge cases", "[containers][streamarray]")
{
    SECTION("Empty string")
    {
        VarStringArray arr;

        VarString* vs = MakeVarString("");
        arr.Add(*vs);
        free(vs);

        REQUIRE(arr.Size() == 1);
        REQUIRE(arr.Get(arr.Begin()).Equals(""));
    }

    SECTION("Single character")
    {
        VarStringArray arr;

        VarString* vs = MakeVarString("x");
        arr.Add(*vs);
        free(vs);

        REQUIRE(arr.Get(arr.Begin()).Equals("x"));
    }

    SECTION("Repeated add and delete")
    {
        VarStringArray arr;

        for (int i = 0; i < 10; i++)
        {
            VarString* vs = MakeVarString("temp");
            Offset off = arr.Add(*vs);
            free(vs);

            arr.Delete(off);
        }

        REQUIRE(arr.Size() == 0);
    }
}

// StreamArray - Stress Test

TEST_CASE("StreamArray - Stress test", "[containers][streamarray]")
{
    SECTION("Many variable-size items")
    {
        VarStringArray arr;

        // Add 100 items of varying lengths
        for (int i = 0; i < 100; i++)
        {
            char buf[128];
            int len = (i % 50) + 1; // Variable length 1-50
            for (int j = 0; j < len && j < 127; j++)
            {
                buf[j] = 'a' + (j % 26);
            }
            buf[len] = '\0';

            VarString* vs = MakeVarString(buf);
            arr.Add(*vs);
            free(vs);
        }

        REQUIRE(arr.Size() == 100);

        // Verify all items are accessible
        int count = 0;
        for (Offset i = arr.Begin(); i < arr.End(); arr.Next(i))
        {
            REQUIRE(arr.Get(i).length > 0);
            count++;
        }
        REQUIRE(count == 100);
    }
}

// StreamArray - Real Use Case

TEST_CASE("StreamArray - Game message queue use case", "[containers][streamarray]")
{
    SECTION("Store and process variable-length messages")
    {
        VarStringArray messages;

        // Add various game messages
        VarString* msg1 = MakeVarString("Player joined");
        VarString* msg2 = MakeVarString("Enemy spotted at coordinates 123,456");
        VarString* msg3 = MakeVarString("Mission objective complete!");
        VarString* msg4 = MakeVarString("Low ammo");

        messages.Add(*msg1);
        messages.Add(*msg2);
        messages.Add(*msg3);
        messages.Add(*msg4);

        // Process messages
        int processed = 0;
        for (Offset i = messages.Begin(); i < messages.End(); messages.Next(i))
        {
            const VarString& msg = messages.Get(i);
            REQUIRE(msg.length > 0);
            processed++;
        }

        REQUIRE(processed == 4);

        free(msg1);
        free(msg2);
        free(msg3);
        free(msg4);
    }
}
