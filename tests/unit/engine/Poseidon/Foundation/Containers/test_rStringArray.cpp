// Unit tests for PoseidonBase rStringArray
// Testing case-insensitive string arrays

#define _CRT_SECURE_NO_WARNINGS
#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Poseidon/Foundation/Containers/RStringArray.hpp>
#include <stdio.h>
#include <string.h>
#include <Poseidon/Foundation/Containers/Array.hpp>
#include <Poseidon/Foundation/Strings/RString.hpp>

// FindArrayTraitsCI - Case-Insensitive Traits Testing

TEST_CASE("FindArrayTraitsCI - Case-insensitive equality", "[containers][rStringArray]")
{
    SECTION("Same case strings are equal")
    {
        REQUIRE(FindArrayTraitsCI::IsEqual("test", "test") == true);
        REQUIRE(FindArrayTraitsCI::IsEqual("TEST", "TEST") == true);
    }

    SECTION("Different case strings are equal (case-insensitive)")
    {
        REQUIRE(FindArrayTraitsCI::IsEqual("Test", "test") == true);
        REQUIRE(FindArrayTraitsCI::IsEqual("TEST", "test") == true);
        REQUIRE(FindArrayTraitsCI::IsEqual("TeSt", "tEsT") == true);
    }

    SECTION("Different strings are not equal")
    {
        REQUIRE(FindArrayTraitsCI::IsEqual("abc", "def") == false);
        REQUIRE(FindArrayTraitsCI::IsEqual("test", "testing") == false);
    }

    SECTION("Empty strings")
    {
        REQUIRE(FindArrayTraitsCI::IsEqual("", "") == true);
        REQUIRE(FindArrayTraitsCI::IsEqual("test", "") == false);
    }
}

TEST_CASE("FindArrayTraitsCI - GetKey for RString", "[containers][rStringArray]")
{
    SECTION("GetKey returns const char*")
    {
        RString str("test");
        const char* key = FindArrayTraitsCI::GetKey(str);

        REQUIRE(key != nullptr);
        REQUIRE(strcmp(key, "test") == 0);
    }
}

TEST_CASE("FindArrayTraitsCI - GetKey for RStringB", "[containers][rStringArray]")
{
    SECTION("GetKey returns const char*")
    {
        RStringB str("test");
        const char* key = FindArrayTraitsCI::GetKey(str);

        REQUIRE(key != nullptr);
        REQUIRE(strcmp(key, "test") == 0);
    }
}

// FindArrayRStringCI - Case-Insensitive RString Array

TEST_CASE("FindArrayRStringCI - Basic construction", "[containers][rStringArray]")
{
    SECTION("Default construction")
    {
        FindArrayRStringCI arr;

        REQUIRE(arr.Size() == 0);
    }

    SECTION("Add items to grow array")
    {
        FindArrayRStringCI arr;

        // Add items - array will grow as needed
        for (int i = 0; i < 10; i++)
        {
            char buf[32];
            sprintf(buf, "Item%d", i);
            arr.Add(RString(buf));
        }

        REQUIRE(arr.Size() == 10);
    }
}

TEST_CASE("FindArrayRStringCI - Add and Find (case-insensitive)", "[containers][rStringArray]")
{
    SECTION("Add unique strings")
    {
        FindArrayRStringCI arr;

        arr.Add(RString("Test"));
        arr.Add(RString("Hello"));
        arr.Add(RString("World"));

        REQUIRE(arr.Size() == 3);
    }

    SECTION("Find with exact case")
    {
        FindArrayRStringCI arr;

        arr.Add(RString("Test"));
        arr.Add(RString("Hello"));

        REQUIRE(arr.Find(RString("Test")) == 0);
        REQUIRE(arr.Find(RString("Hello")) == 1);
    }

    SECTION("Find with different case (case-insensitive)")
    {
        FindArrayRStringCI arr;

        arr.Add(RString("Test"));
        arr.Add(RString("Hello"));

        // Should find regardless of case
        REQUIRE(arr.Find(RString("test")) == 0);
        REQUIRE(arr.Find(RString("TEST")) == 0);
        REQUIRE(arr.Find(RString("hello")) == 1);
        REQUIRE(arr.Find(RString("HELLO")) == 1);
    }

    SECTION("FindKey with const char*")
    {
        FindArrayRStringCI arr;

        arr.Add(RString("Test"));
        arr.Add(RString("Hello"));

        // FindKey uses const char* directly
        REQUIRE(arr.FindKey("test") == 0);
        REQUIRE(arr.FindKey("TEST") == 0);
        REQUIRE(arr.FindKey("hello") == 1);
        REQUIRE(arr.FindKey("HELLO") == 1);
    }

    SECTION("Find non-existent string")
    {
        FindArrayRStringCI arr;

        arr.Add(RString("Test"));

        REQUIRE(arr.Find(RString("NotThere")) < 0);
        REQUIRE(arr.FindKey("missing") < 0);
    }
}

TEST_CASE("FindArrayRStringCI - AddUnique (case-insensitive)", "[containers][rStringArray]")
{
    SECTION("AddUnique with truly unique strings")
    {
        FindArrayRStringCI arr;

        int idx1 = arr.AddUnique(RString("First"));
        int idx2 = arr.AddUnique(RString("Second"));

        REQUIRE(idx1 >= 0);
        REQUIRE(idx2 >= 0);
        REQUIRE(arr.Size() == 2);
    }

    SECTION("AddUnique with duplicate (same case)")
    {
        FindArrayRStringCI arr;

        int idx1 = arr.AddUnique(RString("Test"));
        int idx2 = arr.AddUnique(RString("Test")); // Duplicate

        REQUIRE(idx1 >= 0);
        REQUIRE(idx2 < 0);        // Returns -1 for duplicate
        REQUIRE(arr.Size() == 1); // Only one item
    }

    SECTION("AddUnique with duplicate (different case)")
    {
        FindArrayRStringCI arr;

        int idx1 = arr.AddUnique(RString("Test"));
        int idx2 = arr.AddUnique(RString("test")); // Same key, different case

        REQUIRE(idx1 >= 0);
        REQUIRE(idx2 < 0); // Should be treated as duplicate (case-insensitive)
        REQUIRE(arr.Size() == 1);
    }
}

TEST_CASE("FindArrayRStringCI - Delete operations", "[containers][rStringArray]")
{
    SECTION("Delete existing item (exact case)")
    {
        FindArrayRStringCI arr;

        arr.Add(RString("Test"));
        arr.Add(RString("Hello"));

        bool deleted = arr.Delete(RString("Test"));

        REQUIRE(deleted == true);
        REQUIRE(arr.Size() == 1);
        REQUIRE(arr.Find(RString("Test")) < 0);
    }

    SECTION("Delete existing item (different case)")
    {
        FindArrayRStringCI arr;

        arr.Add(RString("Test"));
        arr.Add(RString("Hello"));

        bool deleted = arr.Delete(RString("test")); // Lowercase

        REQUIRE(deleted == true);
        REQUIRE(arr.Size() == 1);
        REQUIRE(arr.FindKey("Test") < 0);
        REQUIRE(arr.FindKey("test") < 0);
    }

    SECTION("DeleteKey with const char*")
    {
        FindArrayRStringCI arr;

        arr.Add(RString("Test"));

        bool deleted = arr.DeleteKey("TEST"); // Uppercase

        REQUIRE(deleted == true);
        REQUIRE(arr.Size() == 0);
    }

    SECTION("Delete non-existent item")
    {
        FindArrayRStringCI arr;

        arr.Add(RString("Test"));

        bool deleted = arr.Delete(RString("NotThere"));

        REQUIRE(deleted == false);
        REQUIRE(arr.Size() == 1); // Original item still there
    }
}

TEST_CASE("FindArrayRStringCI - Access operations", "[containers][rStringArray]")
{
    SECTION("Get by index")
    {
        FindArrayRStringCI arr;

        arr.Add(RString("First"));
        arr.Add(RString("Second"));
        arr.Add(RString("Third"));

        REQUIRE(strcmp(arr.Get(0), "First") == 0);
        REQUIRE(strcmp(arr.Get(1), "Second") == 0);
        REQUIRE(strcmp(arr.Get(2), "Third") == 0);
    }

    SECTION("Operator[] access")
    {
        FindArrayRStringCI arr;

        arr.Add(RString("Test"));

        REQUIRE(strcmp(arr[0], "Test") == 0);
    }
}

TEST_CASE("FindArrayRStringCI - Complex scenarios", "[containers][rStringArray]")
{
    SECTION("Mixed case operations")
    {
        FindArrayRStringCI arr;

        arr.Add(RString("Apple"));
        arr.Add(RString("BANANA"));
        arr.Add(RString("ChErRy"));

        // Find with various cases
        REQUIRE(arr.FindKey("apple") == 0);
        REQUIRE(arr.FindKey("APPLE") == 0);
        REQUIRE(arr.FindKey("banana") == 1);
        REQUIRE(arr.FindKey("cherry") == 2);
        REQUIRE(arr.FindKey("CHERRY") == 2);
    }

    SECTION("Add, find, delete cycle")
    {
        FindArrayRStringCI arr;

        arr.Add(RString("Item1"));
        arr.Add(RString("Item2"));
        arr.Add(RString("Item3"));

        REQUIRE(arr.FindKey("item2") >= 0);

        arr.DeleteKey("ITEM2");

        REQUIRE(arr.FindKey("item2") < 0);
        REQUIRE(arr.Size() == 2);
    }
}

// FindArrayRStringBCI - Case-Insensitive RStringB Array

TEST_CASE("FindArrayRStringBCI - Basic operations", "[containers][rStringArray]")
{
    SECTION("Add and find")
    {
        FindArrayRStringBCI arr;

        arr.Add(RStringB("Test"));
        arr.Add(RStringB("Hello"));

        REQUIRE(arr.Size() == 2);
        REQUIRE(arr.FindKey("test") == 0);
        REQUIRE(arr.FindKey("HELLO") == 1);
    }

    SECTION("Case-insensitive uniqueness")
    {
        FindArrayRStringBCI arr;

        int idx1 = arr.AddUnique(RStringB("Test"));
        int idx2 = arr.AddUnique(RStringB("TEST"));

        REQUIRE(idx1 >= 0);
        REQUIRE(idx2 < 0);
        REQUIRE(arr.Size() == 1);
    }

    SECTION("Delete with different case")
    {
        FindArrayRStringBCI arr;

        arr.Add(RStringB("Test"));

        bool deleted = arr.DeleteKey("TEST");

        REQUIRE(deleted == true);
        REQUIRE(arr.Size() == 0);
    }
}

// Edge Cases and Special Scenarios

TEST_CASE("FindArrayRStringCI - Edge cases", "[containers][rStringArray]")
{
    SECTION("Empty strings")
    {
        FindArrayRStringCI arr;

        arr.Add(RString(""));
        arr.Add(RString("test"));

        REQUIRE(arr.Size() == 2);
        REQUIRE(arr.FindKey("") == 0);
        REQUIRE(arr.FindKey("test") == 1);
    }

    SECTION("Strings with special characters")
    {
        FindArrayRStringCI arr;

        arr.Add(RString("test_123"));
        arr.Add(RString("hello.world"));
        arr.Add(RString("item-name"));

        REQUIRE(arr.FindKey("TEST_123") == 0);
        REQUIRE(arr.FindKey("HELLO.WORLD") == 1);
        REQUIRE(arr.FindKey("ITEM-NAME") == 2);
    }

    SECTION("Long strings")
    {
        FindArrayRStringCI arr;

        RString longStr("This_Is_A_Very_Long_String_With_Many_Characters_To_Test_Case_Insensitivity");
        arr.Add(longStr);

        REQUIRE(arr.FindKey("this_is_a_very_long_string_with_many_characters_to_test_case_insensitivity") == 0);
    }
}

TEST_CASE("FindArrayRStringCI - Stress test", "[containers][rStringArray]")
{
    SECTION("Many items with various cases")
    {
        FindArrayRStringCI arr;

        // Add 50 items with mixed case
        for (int i = 0; i < 50; i++)
        {
            char buf[32];
            sprintf(buf, "Item%d", i);
            arr.Add(RString(buf));
        }

        REQUIRE(arr.Size() == 50);

        // Find all with lowercase
        for (int i = 0; i < 50; i++)
        {
            char buf[32];
            sprintf(buf, "item%d", i); // lowercase
            REQUIRE(arr.FindKey(buf) >= 0);
        }

        // Find all with uppercase
        for (int i = 0; i < 50; i++)
        {
            char buf[32];
            sprintf(buf, "ITEM%d", i); // uppercase
            REQUIRE(arr.FindKey(buf) >= 0);
        }
    }
}

// Comparison with Case-Sensitive Array

TEST_CASE("FindArrayRStringCI - Comparison with case-sensitive", "[containers][rStringArray]")
{
    SECTION("Case-insensitive finds different cases")
    {
        FindArrayRStringCI ciArr;
        FindArrayKey<RString> csArr; // Case-sensitive (default traits)

        ciArr.Add(RString("Test"));
        csArr.Add(RString("Test"));

        // Case-insensitive finds both cases
        REQUIRE(ciArr.FindKey("test") >= 0);
        REQUIRE(ciArr.FindKey("TEST") >= 0);

        // Case-sensitive only finds exact match
        REQUIRE(csArr.FindKey("Test") >= 0);
        REQUIRE(csArr.FindKey("test") < 0);
        REQUIRE(csArr.FindKey("TEST") < 0);
    }
}
