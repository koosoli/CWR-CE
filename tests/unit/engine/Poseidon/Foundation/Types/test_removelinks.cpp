// Unit tests for Poseidon/Foundation/Types/RemoveLinks.hpp
// Testing auto-null link system (Link, RemoveLinks)

#define _CRT_SECURE_NO_WARNINGS
#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Poseidon/Foundation/Containers/Array.hpp> // For FindArray
#include <Poseidon/Foundation/Types/Pointers.hpp>   // For RefCount
#include <Poseidon/Foundation/Types/RemoveLinks.hpp>

// Test Helper Classes

// Test object for Link testing (auto-null links)
class TestLinkObject : public RemoveLinks
{
  public:
    int value;
    static int constructCount;
    static int destructCount;

    TestLinkObject() : value(0) { constructCount++; }
    explicit TestLinkObject(int v) : value(v) { constructCount++; }
    ~TestLinkObject() override { destructCount++; }

    static void ResetCounters()
    {
        constructCount = 0;
        destructCount = 0;
    }
};

int TestLinkObject::constructCount = 0;
int TestLinkObject::destructCount = 0;

// Batch 1: Link - Auto-Null Link System

TEST_CASE("Link - Basic construction", "[types][removelinks]")
{
    TestLinkObject::ResetCounters();

    SECTION("Default construction")
    {
        Link<TestLinkObject> link;

        REQUIRE(link.IsNull());
        REQUIRE_FALSE(link.NotNull());
    }

    SECTION("Construction from object pointer")
    {
        TestLinkObject* obj = new TestLinkObject(100);
        Link<TestLinkObject> link(obj);

        REQUIRE(link.NotNull());
        REQUIRE(link.GetTypeRef()->value == 100);

        delete obj;
    }
}

TEST_CASE("Link - Auto-null behavior", "[types][removelinks]")
{
    TestLinkObject::ResetCounters();

    SECTION("Link becomes null when object deleted")
    {
        TestLinkObject* obj = new TestLinkObject(200);
        Link<TestLinkObject> link(obj);

        REQUIRE(link.NotNull());
        REQUIRE(link->value == 200);

        delete obj;

        // Link should auto-null
        REQUIRE(link.IsNull());
    }

    SECTION("Multiple links auto-null together")
    {
        TestLinkObject* obj = new TestLinkObject(300);
        Link<TestLinkObject> link1(obj);
        Link<TestLinkObject> link2(obj);
        Link<TestLinkObject> link3(obj);

        REQUIRE(link1.NotNull());
        REQUIRE(link2.NotNull());
        REQUIRE(link3.NotNull());

        delete obj;

        REQUIRE(link1.IsNull());
        REQUIRE(link2.IsNull());
        REQUIRE(link3.IsNull());
    }
}

TEST_CASE("Link - Copy construction and assignment", "[types][removelinks]")
{
    TestLinkObject::ResetCounters();

    SECTION("Copy construction")
    {
        TestLinkObject* obj = new TestLinkObject(400);
        Link<TestLinkObject> link1(obj);
        const Link<TestLinkObject>& link2(link1);

        REQUIRE(link1.NotNull());
        REQUIRE(link2.NotNull());
        REQUIRE(link1.GetTypeRef() == link2.GetTypeRef());

        delete obj;

        REQUIRE(link1.IsNull());
        REQUIRE(link2.IsNull());
    }
}

TEST_CASE("Link - Operators", "[types][removelinks]")
{
    SECTION("Implicit conversion")
    {
        TestLinkObject* obj = new TestLinkObject(500);
        Link<TestLinkObject> link(obj);

        TestLinkObject* ptr = link;
        REQUIRE(ptr == obj);

        delete obj;
    }

    SECTION("Arrow operator")
    {
        TestLinkObject* obj = new TestLinkObject(600);
        Link<TestLinkObject> link(obj);

        link->value = 666;
        REQUIRE(obj->value == 666);

        delete obj;
    }

    SECTION("Dereference operator")
    {
        TestLinkObject* obj = new TestLinkObject(700);
        Link<TestLinkObject> link(obj);

        TestLinkObject& ref = *link;
        ref.value = 777;
        REQUIRE(obj->value == 777);

        delete obj;
    }
}

TEST_CASE("Link - Remove operation", "[types][removelinks]")
{
    TestLinkObject::ResetCounters();

    SECTION("Manual remove")
    {
        TestLinkObject* obj = new TestLinkObject(800);
        Link<TestLinkObject> link(obj);

        REQUIRE(link.NotNull());

        link.Remove();

        REQUIRE(link.IsNull());

        // Object still alive
        REQUIRE(obj->value == 800);

        delete obj;
    }
}

// Batch 2: LinkArray - Array of Auto-Null Links

TEST_CASE("LinkArray - Basic operations", "[types][removelinks]")
{
    TestLinkObject::ResetCounters();

    SECTION("Empty array")
    {
        LinkArray<TestLinkObject> arr;

        REQUIRE(arr.Size() == 0);
        REQUIRE(arr.Count() == 0);
    }

    SECTION("Add links")
    {
        TestLinkObject* obj1 = new TestLinkObject(100);
        TestLinkObject* obj2 = new TestLinkObject(200);

        LinkArray<TestLinkObject> arr;
        arr.Add(Link<TestLinkObject>(obj1));
        arr.Add(Link<TestLinkObject>(obj2));

        REQUIRE(arr.Size() == 2);
        REQUIRE(arr.Count() == 2);
        REQUIRE(arr[0]->value == 100);
        REQUIRE(arr[1]->value == 200);

        delete obj1;
        delete obj2;
    }
}

TEST_CASE("LinkArray - Count with auto-nulled links", "[types][removelinks]")
{
    TestLinkObject::ResetCounters();

    SECTION("Count after deletion")
    {
        TestLinkObject* obj1 = new TestLinkObject(100);
        TestLinkObject* obj2 = new TestLinkObject(200);
        TestLinkObject* obj3 = new TestLinkObject(300);

        LinkArray<TestLinkObject> arr;
        arr.Add(Link<TestLinkObject>(obj1));
        arr.Add(Link<TestLinkObject>(obj2));
        arr.Add(Link<TestLinkObject>(obj3));

        REQUIRE(arr.Count() == 3);

        delete obj2;

        REQUIRE(arr.Size() == 3);
        REQUIRE(arr.Count() == 2); // One link is null

        delete obj1;
        delete obj3;
    }
}

TEST_CASE("LinkArray - Compact operation", "[types][removelinks]")
{
    TestLinkObject::ResetCounters();

    SECTION("Compact removes null links")
    {
        TestLinkObject* obj1 = new TestLinkObject(100);
        TestLinkObject* obj2 = new TestLinkObject(200);
        TestLinkObject* obj3 = new TestLinkObject(300);

        LinkArray<TestLinkObject> arr;
        arr.Add(Link<TestLinkObject>(obj1));
        arr.Add(Link<TestLinkObject>(obj2));
        arr.Add(Link<TestLinkObject>(obj3));

        delete obj2; // Middle one becomes null

        REQUIRE(arr.Size() == 3);
        arr.Compact();
        REQUIRE(arr.Size() == 2);

        REQUIRE(arr[0]->value == 100);
        REQUIRE(arr[1]->value == 300);

        delete obj1;
        delete obj3;
    }

    SECTION("Compact with multiple nulls")
    {
        TestLinkObject* obj1 = new TestLinkObject(100);
        TestLinkObject* obj2 = new TestLinkObject(200);
        TestLinkObject* obj3 = new TestLinkObject(300);
        TestLinkObject* obj4 = new TestLinkObject(400);

        LinkArray<TestLinkObject> arr;
        arr.Add(Link<TestLinkObject>(obj1));
        arr.Add(Link<TestLinkObject>(obj2));
        arr.Add(Link<TestLinkObject>(obj3));
        arr.Add(Link<TestLinkObject>(obj4));

        delete obj2;
        delete obj4;

        REQUIRE(arr.Count() == 2);
        arr.Compact();
        REQUIRE(arr.Size() == 2);

        REQUIRE(arr[0]->value == 100);
        REQUIRE(arr[1]->value == 300);

        delete obj1;
        delete obj3;
    }
}

// Batch 3: Mixed Scenarios

TEST_CASE("Advanced link patterns", "[types][removelinks]")
{
    TestLinkObject::ResetCounters();

    SECTION("Links survive object reuse")
    {
        TestLinkObject* obj = new TestLinkObject(100);
        Link<TestLinkObject> link(obj);

        REQUIRE(link->value == 100);

        obj->value = 200;
        REQUIRE(link->value == 200);

        delete obj;
        REQUIRE(link.IsNull());
    }

    SECTION("Array of links with partial deletion")
    {
        TestLinkObject* objs[5];
        LinkArray<TestLinkObject> arr;

        for (int i = 0; i < 5; i++)
        {
            objs[i] = new TestLinkObject(i * 100);
            arr.Add(Link<TestLinkObject>(objs[i]));
        }

        REQUIRE(arr.Count() == 5);

        // Delete every other one
        delete objs[1];
        delete objs[3];

        REQUIRE(arr.Count() == 3);

        arr.Compact();

        REQUIRE(arr.Size() == 3);
        REQUIRE(arr[0]->value == 0);
        REQUIRE(arr[1]->value == 200);
        REQUIRE(arr[2]->value == 400);

        delete objs[0];
        delete objs[2];
        delete objs[4];
    }
}

TEST_CASE("RemoveLinks - Copy semantics", "[types][removelinks]")
{
    TestLinkObject::ResetCounters();

    SECTION("Copy construction doesn't copy links")
    {
        TestLinkObject* obj1 = new TestLinkObject(100);
        Link<TestLinkObject> link1(obj1);

        // Copy the object
        TestLinkObject* obj2 = new TestLinkObject(*obj1);

        // Link1 still points to obj1, not obj2
        REQUIRE(link1.GetTypeRef() == obj1);

        delete obj1;
        REQUIRE(link1.IsNull());

        delete obj2;
    }
}
