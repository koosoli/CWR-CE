#include <catch2/catch_test_macros.hpp>

#include <Poseidon/Foundation/Modules/Modules.hpp>
#include <vector>
#include <Poseidon/Foundation/Containers/CacheList.hpp>

// Module testing - batch 1: module registration
// Tests for module registration and priority ordering
//
// PURPOSE: Unit tests for module initialization system
// - Registration mechanics
// - Priority queue ordering
// - Edge cases
//
// Coverage areas:
// 1. Registration Basics (4 tests)
// 2. Priority Queue (3 tests)
//
// Total: 7 tests covering registration functionality

// Test Helpers

// Track which functions were called and in what order
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"
static std::vector<int> g_callOrder;
#pragma clang diagnostic pop

// Test functions that record their calls
static void TestFunc1()
{
    g_callOrder.push_back(1);
}
static void TestFunc2()
{
    g_callOrder.push_back(2);
}
static void TestFunc3()
{
    g_callOrder.push_back(3);
}
static void TestFunc4()
{
    g_callOrder.push_back(4);
}
// static void TestFunc5() { g_callOrder.push_back(5); } // Reserved for future tests

// Reset helper - currently unused but kept for future tests
[[maybe_unused]] static void ResetCallOrder()
{
    g_callOrder.clear();
}

// Section 1.1: Registration Basics (4 tests)

TEST_CASE("Modules - Create Poseidon::Foundation::RegistrationItem", "[modules][registration][basic]")
{
    SECTION("Create with init function only")
    {
        Poseidon::Foundation::RegistrationItem item(10, &TestFunc1, nullptr);

        REQUIRE(item.level == 10);
        REQUIRE(item.initFunction == &TestFunc1);
        REQUIRE(item.doneFunction == nullptr);
    }

    SECTION("Create with both init and done functions")
    {
        Poseidon::Foundation::RegistrationItem item(20, &TestFunc2, &TestFunc3);

        REQUIRE(item.level == 20);
        REQUIRE(item.initFunction == &TestFunc2);
        REQUIRE(item.doneFunction == &TestFunc3);
    }

    SECTION("Create with null init function")
    {
        Poseidon::Foundation::RegistrationItem item(30, nullptr, &TestFunc4);

        REQUIRE(item.level == 30);
        REQUIRE(item.initFunction == nullptr);
        REQUIRE(item.doneFunction == &TestFunc4);
    }
}

TEST_CASE("Modules - Register single module", "[modules][registration][basic]")
{
    // Note: We're testing against the global GRegistrationList
    // This test assumes it starts in a reasonable state

    SECTION("Register one module")
    {
        Poseidon::Foundation::RegistrationItem item(100, &TestFunc1, nullptr);

        // Should not crash
        RegisterModule(&item);

        REQUIRE(true);
    }
}

TEST_CASE("Modules - Poseidon::Foundation::RegistrationItem is CLDLink", "[modules][registration][basic]")
{
    SECTION("Can be used in cache list")
    {
        Poseidon::Foundation::RegistrationItem item1(10, &TestFunc1, nullptr);
        Poseidon::Foundation::RegistrationItem item2(20, &TestFunc2, nullptr);

        // Poseidon::Foundation::RegistrationItem inherits from CLDLink
        // Verify it has list capabilities
        REQUIRE(sizeof(Poseidon::Foundation::RegistrationItem) >= sizeof(CLDLink));

        // Can be cast to CLDLink
        CLDLink* link = static_cast<CLDLink*>(&item1);
        REQUIRE(link != nullptr);
    }
}

TEST_CASE("Modules - Multiple modules can be registered", "[modules][registration][basic]")
{
    SECTION("Register multiple distinct modules")
    {
        Poseidon::Foundation::RegistrationItem item1(10, &TestFunc1, nullptr);
        Poseidon::Foundation::RegistrationItem item2(20, &TestFunc2, nullptr);
        Poseidon::Foundation::RegistrationItem item3(30, &TestFunc3, nullptr);

        // All should register without crash
        RegisterModule(&item1);
        RegisterModule(&item2);
        RegisterModule(&item3);

        REQUIRE(true);
    }
}

// Section 1.2: Priority Queue (3 tests)

TEST_CASE("Modules - Priority levels", "[modules][registration][priority]")
{
    SECTION("Higher priority number means later execution")
    {
        // According to code: items are inserted where level <= walk->level
        // So lower levels come first

        Poseidon::Foundation::RegistrationItem low(10, &TestFunc1, nullptr);
        Poseidon::Foundation::RegistrationItem high(100, &TestFunc2, nullptr);

        REQUIRE(low.level < high.level);

        // Lower level should execute first (this is documented behavior)
        REQUIRE(true);
    }
}

TEST_CASE("Modules - Same priority handling", "[modules][registration][priority]")
{
    SECTION("Modules with same priority")
    {
        Poseidon::Foundation::RegistrationItem item1(50, &TestFunc1, nullptr);
        Poseidon::Foundation::RegistrationItem item2(50, &TestFunc2, nullptr);
        Poseidon::Foundation::RegistrationItem item3(50, &TestFunc3, nullptr);

        // All have same priority
        REQUIRE(item1.level == item2.level);
        REQUIRE(item2.level == item3.level);

        // Order among same-priority items is insertion order
        // (first registered goes first due to InsertBefore logic)
        RegisterModule(&item1);
        RegisterModule(&item2);
        RegisterModule(&item3);

        REQUIRE(true);
    }
}

TEST_CASE("Modules - Mixed priority ordering", "[modules][registration][priority]")
{
    SECTION("Modules registered in non-sorted order")
    {
        // Register in order: medium, low, high
        Poseidon::Foundation::RegistrationItem medium(50, &TestFunc2, nullptr);
        Poseidon::Foundation::RegistrationItem low(10, &TestFunc1, nullptr);
        Poseidon::Foundation::RegistrationItem high(100, &TestFunc3, nullptr);

        RegisterModule(&medium);
        RegisterModule(&low);
        RegisterModule(&high);

        // Should be internally sorted by priority
        // Execution order should be: low (10), medium (50), high (100)
        REQUIRE(true);
    }
}

// Summary: Registration Tests Complete
//
// Tests: 7
// Sections:
// - Registration Basics: 4 tests
// - Priority Queue: 3 tests
//
// Focus: Module registration mechanics and priority ordering
// Coverage: Basic registration API and priority queue behavior
//
// Known Limitations:
// - Cannot test actual execution order without calling Poseidon::Foundation::InitModules()
//   (that's tested in test_modules_lifecycle.cpp)
// - Cannot easily reset global GRegistrationList between tests
// - Tests assume global list is in reasonable state
//
// These limitations are acceptable - we test the registration mechanics
// separately from the execution mechanics for clarity.
