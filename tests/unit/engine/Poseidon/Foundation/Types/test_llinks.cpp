// Unit tests for Poseidon/Foundation/Types/LLinks.hpp
// Testing weak link system (LLink, TrackLLinks, RemoveLLinks)

#define _CRT_SECURE_NO_WARNINGS
#include <cstring>
#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Poseidon/Foundation/Types/LLinks.hpp>

// Test Helper Classes

// Test object for LLink testing
class TestLLinkObject : public RemoveLLinks
{
  public:
    int value;
    static int constructCount;
    static int destructCount;

    TestLLinkObject() : value(0) { constructCount++; }
    explicit TestLLinkObject(int v) : value(v) { constructCount++; }
    ~TestLLinkObject() override { destructCount++; }

    static void ResetCounters()
    {
        constructCount = 0;
        destructCount = 0;
    }
};

int TestLLinkObject::constructCount = 0;
int TestLLinkObject::destructCount = 0;

// Batch 1: LLink - Weak Link System

TEST_CASE("LLink - Basic construction", "[types][llinks]")
{
    TestLLinkObject::ResetCounters();

    SECTION("Default construction")
    {
        LLink<TestLLinkObject> link;

        REQUIRE(link.GetLink() == nullptr);
    }

    SECTION("Construction from object pointer")
    {
        TestLLinkObject* obj = new TestLLinkObject(100);
        LLink<TestLLinkObject> link(obj);

        REQUIRE(link.GetLink() != nullptr);
        REQUIRE(link.GetLink()->value == 100);

        delete obj;
    }

    SECTION("Construction from nullptr")
    {
        LLink<TestLLinkObject> link(nullptr);

        REQUIRE(link.GetLink() == nullptr);
    }
}

TEST_CASE("LLink - Weak reference behavior", "[types][llinks]")
{
    TestLLinkObject::ResetCounters();

    SECTION("Link becomes null when object deleted")
    {
        TestLLinkObject* obj = new TestLLinkObject(200);
        LLink<TestLLinkObject> link(obj);

        REQUIRE(link.GetLink() != nullptr);
        REQUIRE(link.GetLink()->value == 200);

        delete obj;

        // Link should now be null
        REQUIRE(link.GetLink() == nullptr);
    }

    SECTION("Multiple links to same object")
    {
        TestLLinkObject* obj = new TestLLinkObject(300);
        LLink<TestLLinkObject> link1(obj);
        LLink<TestLLinkObject> link2(obj);
        LLink<TestLLinkObject> link3(obj);

        REQUIRE(link1.GetLink() == obj);
        REQUIRE(link2.GetLink() == obj);
        REQUIRE(link3.GetLink() == obj);

        delete obj;

        // All links should be null
        REQUIRE(link1.GetLink() == nullptr);
        REQUIRE(link2.GetLink() == nullptr);
        REQUIRE(link3.GetLink() == nullptr);
    }
}

TEST_CASE("LLink - Operators", "[types][llinks]")
{
    SECTION("Implicit conversion to pointer")
    {
        TestLLinkObject* obj = new TestLLinkObject(400);
        LLink<TestLLinkObject> link(obj);

        TestLLinkObject* ptr = link;
        REQUIRE(ptr == obj);
        REQUIRE(ptr->value == 400);

        delete obj;
    }

    SECTION("Arrow operator")
    {
        TestLLinkObject* obj = new TestLLinkObject(500);
        LLink<TestLLinkObject> link(obj);

        link->value = 555;
        REQUIRE(obj->value == 555);

        delete obj;
    }
}

TEST_CASE("LLink - Copy and assignment", "[types][llinks]")
{
    TestLLinkObject::ResetCounters();

    SECTION("Copy construction")
    {
        TestLLinkObject* obj = new TestLLinkObject(600);
        LLink<TestLLinkObject> link1(obj);
        const LLink<TestLLinkObject>& link2(link1);

        REQUIRE(link1.GetLink() == obj);
        REQUIRE(link2.GetLink() == obj);

        delete obj;

        REQUIRE(link1.GetLink() == nullptr);
        REQUIRE(link2.GetLink() == nullptr);
    }
}

TEST_CASE("LLink - Edge cases", "[types][llinks]")
{
    TestLLinkObject::ResetCounters();

    SECTION("Link survives object replacement")
    {
        TestLLinkObject* obj1 = new TestLLinkObject(700);
        LLink<TestLLinkObject> link(obj1);

        REQUIRE(link.GetLink() == obj1);

        TestLLinkObject* obj2 = new TestLLinkObject(800);
        link = LLink<TestLLinkObject>(obj2);

        REQUIRE(link.GetLink() == obj2);
        REQUIRE(link->value == 800);

        delete obj1;
        delete obj2;
    }
}

// Batch 2: LLinkArray - Array of Weak Links

TEST_CASE("LLinkArray - Basic operations", "[types][llinks]")
{
    TestLLinkObject::ResetCounters();

    SECTION("Empty array")
    {
        LLinkArray<TestLLinkObject> arr;

        REQUIRE(arr.Size() == 0);
        REQUIRE(arr.Count() == 0);
    }

    SECTION("Add links")
    {
        TestLLinkObject* obj1 = new TestLLinkObject(100);
        TestLLinkObject* obj2 = new TestLLinkObject(200);
        TestLLinkObject* obj3 = new TestLLinkObject(300);

        LLinkArray<TestLLinkObject> arr;
        arr.Add(LLink<TestLLinkObject>(obj1));
        arr.Add(LLink<TestLLinkObject>(obj2));
        arr.Add(LLink<TestLLinkObject>(obj3));

        REQUIRE(arr.Size() == 3);
        REQUIRE(arr.Count() == 3);
        REQUIRE(arr[0]->value == 100);
        REQUIRE(arr[1]->value == 200);
        REQUIRE(arr[2]->value == 300);

        delete obj1;
        delete obj2;
        delete obj3;
    }
}

TEST_CASE("LLinkArray - Count with null links", "[types][llinks]")
{
    TestLLinkObject::ResetCounters();

    SECTION("Count after object deletion")
    {
        TestLLinkObject* obj1 = new TestLLinkObject(100);
        TestLLinkObject* obj2 = new TestLLinkObject(200);
        TestLLinkObject* obj3 = new TestLLinkObject(300);

        LLinkArray<TestLLinkObject> arr;
        arr.Add(LLink<TestLLinkObject>(obj1));
        arr.Add(LLink<TestLLinkObject>(obj2));
        arr.Add(LLink<TestLLinkObject>(obj3));

        REQUIRE(arr.Count() == 3);

        delete obj2; // Delete middle object

        // Count should reflect null link
        REQUIRE(arr.Size() == 3);  // Size unchanged
        REQUIRE(arr.Count() == 2); // Count excludes nulls
        REQUIRE(arr[0] != nullptr);
        REQUIRE(arr[1] == nullptr); // This one is null
        REQUIRE(arr[2] != nullptr);

        delete obj1;
        delete obj3;
    }
}

TEST_CASE("LLinkArray - Compact", "[types][llinks]")
{
    TestLLinkObject::ResetCounters();

    SECTION("Compact removes null links")
    {
        TestLLinkObject* obj1 = new TestLLinkObject(100);
        TestLLinkObject* obj2 = new TestLLinkObject(200);
        TestLLinkObject* obj3 = new TestLLinkObject(300);

        LLinkArray<TestLLinkObject> arr;
        arr.Add(LLink<TestLLinkObject>(obj1));
        arr.Add(LLink<TestLLinkObject>(obj2));
        arr.Add(LLink<TestLLinkObject>(obj3));

        delete obj2; // Create a null in the middle

        REQUIRE(arr.Size() == 3);
        REQUIRE(arr.Count() == 2);

        arr.Compact();

        REQUIRE(arr.Size() == 2); // Size reduced
        REQUIRE(arr.Count() == 2);
        REQUIRE(arr[0]->value == 100);
        REQUIRE(arr[1]->value == 300);

        delete obj1;
        delete obj3;
    }
}

// Regression: GetLink() on a ZEROED slot must be null-safe
//
// A constructed LLink's _tracker is the LLinkNil sentinel (non-null, its
// GetObject() is null). But a slot left ZEROED by container growth/relocation
// has _tracker == null, and the unguarded GetLink() (`_tracker->GetObject()`)
// then read [null + offset] — the crash in AIUnit::CheckGetOut in campaign
// mission 33Escape, where a null OLink in Transport::WhoIsGettingOut() survived
// Compact() and `[i].GetLink() == this` dereferenced it (NULL_CLASS_PTR_READ,
// read from address 0x10). GetLink() must treat a null tracker as an empty link.
//
// Without the fix this TEST_CASE crashes the test binary (null deref); with it,
// GetLink() returns null and Compact() drops the slot.
// Contract guard for the AIUnit::CheckGetOut crash in campaign mission 33Escape
// (NULL_CLASS_PTR_READ at CheckGetOut+0x83, read from 0x10): a null OLink in
// Transport::WhoIsGettingOut() survived Compact(), and `[i].GetLink() == this`
// dereferenced it. A constructed LLink's _tracker is the LLinkNil sentinel
// (non-null), but a slot left zeroed by container growth has _tracker == null,
// and the old unguarded GetLink() (`_tracker->GetObject()`) read TrackLLinks::
// _remove at [null + 0x10]. The fix null-checks _tracker.
//
// NOTE: this can't be a reliable RED at unit level — the broken path is a null
// deref (UB), which the optimizer folds away on a known-zeroed buffer (verified:
// the unguarded version still "passes" here). The authoritative broken-state
// evidence is the game minidump. This guards the fixed contract: GetLink() on a
// null-tracker slot yields null (and Compact() then drops it).
TEST_CASE("LLink - zeroed slot GetLink is null-safe", "[types][llinks]")
{
    alignas(LLink<TestLLinkObject>) unsigned char buf[sizeof(LLink<TestLLinkObject>)] = {};
    const LLink<TestLLinkObject>* link = reinterpret_cast<const LLink<TestLLinkObject>*>(buf);

    REQUIRE(link->GetLink() == nullptr);
    REQUIRE(static_cast<const TestLLinkObject*>(*link) == nullptr); // operator Type*
}
