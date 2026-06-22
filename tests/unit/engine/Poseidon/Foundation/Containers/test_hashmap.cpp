// Unit tests for MapStringToClass (HashMap) from PoseidonBase containers
// Testing in batches: Basic operations, then advanced features

#define _CRT_SECURE_NO_WARNINGS
#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Poseidon/Foundation/Containers/HashMap.hpp>
#include <Poseidon/Foundation/Containers/Array.hpp>
#include <cstring>
#include <stdio.h>

// Test Types for HashMap

// Simple value type for hash map testing
class TestValue
{
    char _key[64];
    int _data;
    mutable int _refCount;

  public:
    TestValue() : _data(0), _refCount(0) { _key[0] = '\0'; }

    TestValue(const char* key, int data = 0) : _data(data), _refCount(0)
    {
        strncpy(_key, key, sizeof(_key) - 1);
        _key[sizeof(_key) - 1] = '\0';
    }

    const char* GetKey() const { return _key; }
    int GetData() const { return _data; }
    void SetData(int d) { _data = d; }

    // Reference counting for RefArray if needed
    int AddRef() const { return ++_refCount; }
    int Release() const
    {
        int ret = --_refCount;
        if (ret == 0)
        {
            delete this;
        }
        return ret;
    }
    int RefCounter() const { return _refCount; }
};

// Batch 1: Basic HashMap Operations

TEST_CASE("HashMap - Basic construction and initialization", "[hashmap]")
{
    SECTION("Default construction")
    {
        MapStringToClass<TestValue, AutoArray<TestValue>> map;

        REQUIRE(map.NItems() == 0);
        REQUIRE(map.NTables() == 0); // No tables until first add
    }

    SECTION("Construction with size")
    {
        MapStringToClass<TestValue, AutoArray<TestValue>> map(31);

        REQUIRE(map.NItems() == 0);
        REQUIRE(map.NTables() == 0); // Tables created on first add
    }

    SECTION("Init method")
    {
        MapStringToClass<TestValue, AutoArray<TestValue>> map;
        map.Init(31);

        REQUIRE(map.NItems() == 0);
    }
}

TEST_CASE("HashMap - Add and Get operations", "[hashmap]")
{
    SECTION("Add single item")
    {
        MapStringToClass<TestValue, AutoArray<TestValue>> map;

        TestValue val("test", 42);
        map.Add(val);

        REQUIRE(map.NItems() == 1);
        REQUIRE(map.NTables() > 0);

        const TestValue& retrieved = map.Get("test");
        REQUIRE(MapStringToClass<TestValue, AutoArray<TestValue>>::NotNull(retrieved));
        REQUIRE(retrieved.GetData() == 42);
    }

    SECTION("Add multiple items")
    {
        MapStringToClass<TestValue, AutoArray<TestValue>> map;

        map.Add(TestValue("first", 1));
        map.Add(TestValue("second", 2));
        map.Add(TestValue("third", 3));

        REQUIRE(map.NItems() == 3);

        REQUIRE(map.Get("first").GetData() == 1);
        REQUIRE(map.Get("second").GetData() == 2);
        REQUIRE(map.Get("third").GetData() == 3);
    }

    SECTION("Get non-existent item returns null")
    {
        MapStringToClass<TestValue, AutoArray<TestValue>> map;

        map.Add(TestValue("exists", 100));

        const TestValue& notFound = map.Get("doesnotexist");
        REQUIRE(MapStringToClass<TestValue, AutoArray<TestValue>>::IsNull(notFound));
    }

    SECTION("Replace existing item")
    {
        MapStringToClass<TestValue, AutoArray<TestValue>> map;

        map.Add(TestValue("key", 100));
        map.Add(TestValue("key", 200)); // Replace

        REQUIRE(map.NItems() == 1);               // Still only one item
        REQUIRE(map.Get("key").GetData() == 200); // Updated value
    }
}

TEST_CASE("HashMap - Operator[] access", "[hashmap]")
{
    SECTION("Set and get using operator[]")
    {
        MapStringToClass<TestValue, AutoArray<TestValue>> map;

        map.Add(TestValue("test", 42));

        // Const access
        const auto& constMap = map;
        REQUIRE(constMap["test"].GetData() == 42);

        // Mutable access and modify
        map["test"].SetData(99);
        REQUIRE(map["test"].GetData() == 99);
    }
}

TEST_CASE("HashMap - Remove operations", "[hashmap]")
{
    SECTION("Remove existing item")
    {
        MapStringToClass<TestValue, AutoArray<TestValue>> map;

        map.Add(TestValue("test", 42));
        REQUIRE(map.NItems() == 1);

        bool removed = map.Remove("test");

        REQUIRE(removed == true);
        REQUIRE(map.NItems() == 0);
        REQUIRE(MapStringToClass<TestValue, AutoArray<TestValue>>::IsNull(map.Get("test")));
    }

    SECTION("Remove non-existent item")
    {
        MapStringToClass<TestValue, AutoArray<TestValue>> map;

        map.Add(TestValue("test", 42));

        bool removed = map.Remove("doesnotexist");

        REQUIRE(removed == false);
        REQUIRE(map.NItems() == 1); // Original item still there
    }

    SECTION("Remove from multiple items")
    {
        MapStringToClass<TestValue, AutoArray<TestValue>> map;

        map.Add(TestValue("first", 1));
        map.Add(TestValue("second", 2));
        map.Add(TestValue("third", 3));

        map.Remove("second");

        REQUIRE(map.NItems() == 2);
        REQUIRE(MapStringToClass<TestValue, AutoArray<TestValue>>::NotNull(map.Get("first")));
        REQUIRE(MapStringToClass<TestValue, AutoArray<TestValue>>::IsNull(map.Get("second")));
        REQUIRE(MapStringToClass<TestValue, AutoArray<TestValue>>::NotNull(map.Get("third")));
    }

    SECTION("Remove last item clears table")
    {
        MapStringToClass<TestValue, AutoArray<TestValue>> map;

        map.Add(TestValue("only", 42));
        map.Remove("only");

        REQUIRE(map.NItems() == 0);
        REQUIRE(map.NTables() == 0); // Table cleared when empty
    }
}

TEST_CASE("HashMap - Clear operations", "[hashmap]")
{
    SECTION("Clear empty map")
    {
        MapStringToClass<TestValue, AutoArray<TestValue>> map;
        map.Clear();

        REQUIRE(map.NItems() == 0);
        REQUIRE(map.NTables() == 0);
    }

    SECTION("Clear populated map")
    {
        MapStringToClass<TestValue, AutoArray<TestValue>> map;

        for (int i = 0; i < 10; i++)
        {
            char key[32];
            sprintf(key, "item%d", i);
            map.Add(TestValue(key, i));
        }

        REQUIRE(map.NItems() == 10);

        map.Clear();

        REQUIRE(map.NItems() == 0);
        REQUIRE(map.NTables() == 0);
    }
}

// Batch 2: Dynamic Resizing and Capacity

TEST_CASE("HashMap - Dynamic resizing", "[hashmap]")
{
    SECTION("Automatic resize on many adds")
    {
        MapStringToClass<TestValue, AutoArray<TestValue>> map;

        // Add many items to trigger resizing
        for (int i = 0; i < 100; i++)
        {
            char key[32];
            sprintf(key, "key%d", i);
            map.Add(TestValue(key, i));
        }

        REQUIRE(map.NItems() == 100);

        // Verify all items are accessible
        for (int i = 0; i < 100; i++)
        {
            char key[32];
            sprintf(key, "key%d", i);
            REQUIRE(MapStringToClass<TestValue, AutoArray<TestValue>>::NotNull(map.Get(key)));
            REQUIRE(map.Get(key).GetData() == i);
        }
    }

    SECTION("Reserve capacity")
    {
        MapStringToClass<TestValue, AutoArray<TestValue>> map;

        map.Reserve(100);

        // Add items - should not resize
        for (int i = 0; i < 50; i++)
        {
            char key[32];
            sprintf(key, "key%d", i);
            map.Add(TestValue(key, i));
        }

        REQUIRE(map.NItems() == 50);
    }

    SECTION("Rebuild with new size")
    {
        MapStringToClass<TestValue, AutoArray<TestValue>> map;

        for (int i = 0; i < 10; i++)
        {
            char key[32];
            sprintf(key, "key%d", i);
            map.Add(TestValue(key, i));
        }

        int itemsBefore = map.NItems();
        map.Rebuild(31); // Rebuild with specific size

        REQUIRE(map.NItems() == itemsBefore);

        // Verify all items still accessible after rebuild
        for (int i = 0; i < 10; i++)
        {
            char key[32];
            sprintf(key, "key%d", i);
            REQUIRE(map.Get(key).GetData() == i);
        }
    }
}

TEST_CASE("HashMap - Compact operation", "[hashmap]")
{
    SECTION("Compact populated map")
    {
        MapStringToClass<TestValue, AutoArray<TestValue>> map;

        for (int i = 0; i < 20; i++)
        {
            char key[32];
            sprintf(key, "item%d", i);
            map.Add(TestValue(key, i));
        }

        int itemsBefore = map.NItems();
        map.Compact();

        REQUIRE(map.NItems() == itemsBefore);

        // Verify all items still work
        REQUIRE(map.Get("item0").GetData() == 0);
        REQUIRE(map.Get("item19").GetData() == 19);
    }
}

// Batch 3: Copy and Assignment

TEST_CASE("HashMap - Copy constructor", "[hashmap]")
{
    SECTION("Copy empty map")
    {
        MapStringToClass<TestValue, AutoArray<TestValue>> map1;
        const MapStringToClass<TestValue, AutoArray<TestValue>>& map2(map1);

        REQUIRE(map2.NItems() == 0);
    }

    SECTION("Copy populated map")
    {
        MapStringToClass<TestValue, AutoArray<TestValue>> map1;

        map1.Add(TestValue("first", 1));
        map1.Add(TestValue("second", 2));
        map1.Add(TestValue("third", 3));

        MapStringToClass<TestValue, AutoArray<TestValue>> map2(map1);

        REQUIRE(map2.NItems() == 3);
        REQUIRE(map2.Get("first").GetData() == 1);
        REQUIRE(map2.Get("second").GetData() == 2);
        REQUIRE(map2.Get("third").GetData() == 3);

        // Modify copy doesn't affect original
        map2["first"].SetData(100);
        REQUIRE(map1.Get("first").GetData() == 1);
        REQUIRE(map2.Get("first").GetData() == 100);
    }
}

TEST_CASE("HashMap - Assignment operator", "[hashmap]")
{
    SECTION("Assign populated map")
    {
        MapStringToClass<TestValue, AutoArray<TestValue>> map1;
        MapStringToClass<TestValue, AutoArray<TestValue>> map2;

        map1.Add(TestValue("first", 1));
        map1.Add(TestValue("second", 2));

        map2 = map1;

        REQUIRE(map2.NItems() == 2);
        REQUIRE(map2.Get("first").GetData() == 1);
        REQUIRE(map2.Get("second").GetData() == 2);
    }

    SECTION("Assign to non-empty map")
    {
        MapStringToClass<TestValue, AutoArray<TestValue>> map1;
        MapStringToClass<TestValue, AutoArray<TestValue>> map2;

        map1.Add(TestValue("new", 100));
        map2.Add(TestValue("old", 200));

        map2 = map1; // Should clear map2 first

        REQUIRE(map2.NItems() == 1);
        REQUIRE(map2.Get("new").GetData() == 100);
        REQUIRE(MapStringToClass<TestValue, AutoArray<TestValue>>::IsNull(map2.Get("old")));
    }
}

// Batch 4: Iteration and ForEach

TEST_CASE("HashMap - Iterator", "[hashmap]")
{
    SECTION("Iterate empty map")
    {
        MapStringToClass<TestValue, AutoArray<TestValue>> map;

        int count = 0;
        for (MapStringToClass<TestValue, AutoArray<TestValue>>::Iterator it(map); it; ++it)
        {
            count++;
        }

        REQUIRE(count == 0);
    }

    SECTION("Iterate populated map")
    {
        MapStringToClass<TestValue, AutoArray<TestValue>> map;

        map.Add(TestValue("first", 1));
        map.Add(TestValue("second", 2));
        map.Add(TestValue("third", 3));

        int count = 0;
        int sum = 0;
        for (MapStringToClass<TestValue, AutoArray<TestValue>>::Iterator it(map); it; ++it)
        {
            count++;
            sum += (*it).GetData();
        }

        REQUIRE(count == 3);
        REQUIRE(sum == 6);
    }
}

// Helper for ForEach callback
static int g_forEachSum = 0;
static void SumCallback(const TestValue& val, const MapStringToClass<TestValue, AutoArray<TestValue>>*, void*)
{
    g_forEachSum += val.GetData();
}

TEST_CASE("HashMap - ForEach callback", "[hashmap]")
{
    SECTION("ForEach const")
    {
        MapStringToClass<TestValue, AutoArray<TestValue>> map;

        map.Add(TestValue("a", 10));
        map.Add(TestValue("b", 20));
        map.Add(TestValue("c", 30));

        g_forEachSum = 0;
        map.ForEach(SumCallback, nullptr);

        REQUIRE(g_forEachSum == 60);
    }
}

// Batch 5: Hash Function Testing

TEST_CASE("HashMap - Hash function behavior", "[hashmap]")
{
    SECTION("CalculateStringHashValue is consistent")
    {
        unsigned int hash1 = CalculateStringHashValue("test");
        unsigned int hash2 = CalculateStringHashValue("test");

        REQUIRE(hash1 == hash2);
        REQUIRE(hash1 != 0);
    }

    SECTION("Different strings have different hashes")
    {
        unsigned int hash1 = CalculateStringHashValue("abc");
        unsigned int hash2 = CalculateStringHashValue("def");
        unsigned int hash3 = CalculateStringHashValue("xyz");

        // Very unlikely all three would be the same
        REQUIRE((hash1 != hash2 || hash2 != hash3));
    }

    SECTION("Case-insensitive hash")
    {
        unsigned int hash1 = CalculateStringHashValueCI("Test");
        unsigned int hash2 = CalculateStringHashValueCI("TEST");
        unsigned int hash3 = CalculateStringHashValueCI("test");

        REQUIRE(hash1 == hash2);
        REQUIRE(hash2 == hash3);
    }
}

// Batch 6: Edge Cases and Stress Tests

TEST_CASE("HashMap - Edge cases", "[hashmap]")
{
    SECTION("Add with empty key")
    {
        MapStringToClass<TestValue, AutoArray<TestValue>> map;

        map.Add(TestValue("", 42));

        REQUIRE(map.NItems() == 1);
        REQUIRE(map.Get("").GetData() == 42);
    }

    SECTION("Long keys")
    {
        MapStringToClass<TestValue, AutoArray<TestValue>> map;

        char longKey[64];
        for (int i = 0; i < 63; i++)
        {
            longKey[i] = 'a' + (i % 26);
        }
        longKey[63] = '\0';

        map.Add(TestValue(longKey, 999));

        REQUIRE(map.Get(longKey).GetData() == 999);
    }

    SECTION("Keys with special characters")
    {
        MapStringToClass<TestValue, AutoArray<TestValue>> map;

        map.Add(TestValue("key_with_underscores", 1));
        map.Add(TestValue("key.with.dots", 2));
        map.Add(TestValue("key-with-dashes", 3));
        map.Add(TestValue("key123numbers", 4));

        REQUIRE(map.NItems() == 4);
        REQUIRE(map.Get("key_with_underscores").GetData() == 1);
        REQUIRE(map.Get("key.with.dots").GetData() == 2);
    }
}

TEST_CASE("HashMap - Stress test", "[hashmap]")
{
    SECTION("Many operations")
    {
        MapStringToClass<TestValue, AutoArray<TestValue>> map;

        // Add 200 items
        for (int i = 0; i < 200; i++)
        {
            char key[32];
            sprintf(key, "stress%d", i);
            map.Add(TestValue(key, i));
        }

        REQUIRE(map.NItems() == 200);

        // Remove every other item
        for (int i = 0; i < 200; i += 2)
        {
            char key[32];
            sprintf(key, "stress%d", i);
            map.Remove(key);
        }

        REQUIRE(map.NItems() == 100);

        // Verify remaining items
        for (int i = 1; i < 200; i += 2)
        {
            char key[32];
            sprintf(key, "stress%d", i);
            REQUIRE(MapStringToClass<TestValue, AutoArray<TestValue>>::NotNull(map.Get(key)));
            REQUIRE(map.Get(key).GetData() == i);
        }
    }
}
