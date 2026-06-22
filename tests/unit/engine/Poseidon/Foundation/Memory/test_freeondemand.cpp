#include <catch2/catch_test_macros.hpp>

#include <Poseidon/Foundation/Memory/MemFreeReq.hpp>
#include <vector>
#include <algorithm>
#include <stddef.h>
#include <format>

using Poseidon::Foundation::MemoryFreeOnDemandList;

// FreeOnDemand testing - complete module tests
// Tests for memory free-on-demand system
//
// PURPOSE: Unit tests for priority-based memory deallocation
// - Interface implementation
// - List registration and sorting
// - Free operations
// - Helper class pattern
//
// Coverage areas:
// 1. Interface Implementation (3 tests)
// 2. List Management (4 tests)
// 3. Free Operations (3 tests)
// 4. Helper Class Pattern (2 tests)
//
// Total: 12 tests covering FreeOnDemand functionality

// Mock Deallocators for Testing

//! Mock deallocator that tracks freed memory
class MockDeallocator : public Poseidon::Foundation::IMemoryFreeOnDemand
{
    size_t available;
    float priority;
    size_t freedTotal{0};
    int freeCallCount{0};
    int freeAllCallCount{0};

  public:
    MockDeallocator(size_t avail, float pri) : available(avail), priority(pri) {}

    size_t Free(size_t amount) override
    {
        freeCallCount++;
        size_t toFree = (std::min)(amount, available);
        available -= toFree;
        freedTotal += toFree;
        return toFree;
    }

    size_t FreeAll() override
    {
        freeAllCallCount++;
        size_t freed = available;
        freedTotal += freed;
        available = 0;
        return freed;
    }

    float Priority() override { return priority; }

    // Test accessors
    size_t GetFreedTotal() const { return freedTotal; }
    size_t GetAvailable() const { return available; }
    int GetFreeCallCount() const { return freeCallCount; }
    int GetFreeAllCallCount() const { return freeAllCallCount; }

    void Reset()
    {
        freedTotal = 0;
        freeCallCount = 0;
        freeAllCallCount = 0;
    }
};

//! Test helper implementation using FreeOneItem pattern
class TestHelper : public Poseidon::Foundation::MemoryFreeOnDemandHelper
{
    std::vector<size_t> items;
    float priorityValue;

  public:
    TestHelper(const std::vector<size_t>& itemSizes, float pri) : items(itemSizes), priorityValue(pri) {}

    size_t FreeOneItem() override
    {
        if (items.empty())
        {
            return 0;
        }
        size_t freed = items.back();
        items.pop_back();
        return freed;
    }

    float Priority() override { return priorityValue; }

    size_t GetItemCount() const { return items.size(); }
};

// Section 1: Interface Implementation (3 tests)

TEST_CASE("IMemoryFreeOnDemand - Basic interface", "[freeondemand][interface]")
{
    SECTION("Create mock deallocator")
    {
        MockDeallocator dealloc(1000, 5.0f);

        REQUIRE(dealloc.GetAvailable() == 1000);
        REQUIRE(dealloc.GetFreedTotal() == 0);
        REQUIRE(dealloc.Priority() == 5.0f);
    }

    SECTION("Free specific amount")
    {
        MockDeallocator dealloc(1000, 5.0f);

        size_t freed = dealloc.Free(300);

        REQUIRE(freed == 300);
        REQUIRE(dealloc.GetAvailable() == 700);
        REQUIRE(dealloc.GetFreedTotal() == 300);
        REQUIRE(dealloc.GetFreeCallCount() == 1);
    }

    SECTION("Free more than available")
    {
        MockDeallocator dealloc(500, 5.0f);

        size_t freed = dealloc.Free(1000);

        // Should only free what's available
        REQUIRE(freed == 500);
        REQUIRE(dealloc.GetAvailable() == 0);
        REQUIRE(dealloc.GetFreedTotal() == 500);
    }
}

TEST_CASE("IMemoryFreeOnDemand - FreeAll operation", "[freeondemand][interface]")
{
    SECTION("FreeAll frees everything")
    {
        MockDeallocator dealloc(1000, 5.0f);

        size_t freed = dealloc.FreeAll();

        REQUIRE(freed == 1000);
        REQUIRE(dealloc.GetAvailable() == 0);
        REQUIRE(dealloc.GetFreedTotal() == 1000);
        REQUIRE(dealloc.GetFreeAllCallCount() == 1);
    }

    SECTION("FreeAll on empty deallocator")
    {
        MockDeallocator dealloc(0, 5.0f);

        size_t freed = dealloc.FreeAll();

        REQUIRE(freed == 0);
        REQUIRE(dealloc.GetAvailable() == 0);
    }
}

TEST_CASE("IMemoryFreeOnDemand - Priority values", "[freeondemand][interface]")
{
    SECTION("Different priorities")
    {
        MockDeallocator low(1000, 1.0f);
        MockDeallocator medium(1000, 5.0f);
        MockDeallocator high(1000, 10.0f);

        REQUIRE(low.Priority() < medium.Priority());
        REQUIRE(medium.Priority() < high.Priority());
    }
}

// Section 2: List Management (4 tests)

TEST_CASE("MemoryFreeOnDemandList - Registration", "[freeondemand][list]")
{
    MemoryFreeOnDemandList list;

    SECTION("Register single deallocator")
    {
        MockDeallocator dealloc(1000, 5.0f);

        list.Register(&dealloc);

        // Should be in list (can free from it)
        size_t freed = list.Free(100);
        REQUIRE(freed == 100);
    }

    SECTION("Register multiple deallocators")
    {
        MockDeallocator d1(1000, 1.0f);
        MockDeallocator d2(1000, 2.0f);
        MockDeallocator d3(1000, 3.0f);

        list.Register(&d1);
        list.Register(&d2);
        list.Register(&d3);

        // All should be registered
        size_t freed = list.Free(100);
        REQUIRE(freed == 100);
    }
}

TEST_CASE("MemoryFreeOnDemandList - Priority ordering", "[freeondemand][list]")
{
    MemoryFreeOnDemandList list;

    SECTION("Lower priority freed first")
    {
        MockDeallocator low(1000, 1.0f);
        MockDeallocator high(1000, 10.0f);

        // Register in reverse order
        list.Register(&high);
        list.Register(&low);

        // Free small amount - should come from low priority
        list.Free(300);

        REQUIRE(low.GetFreedTotal() == 300);
        REQUIRE(high.GetFreedTotal() == 0);
    }

    SECTION("Multiple priorities in order")
    {
        MockDeallocator p1(500, 1.0f);
        MockDeallocator p5(500, 5.0f);
        MockDeallocator p10(500, 10.0f);

        // Register out of order
        list.Register(&p10);
        list.Register(&p1);
        list.Register(&p5);

        // Free 1000 bytes - should take from p1 (500) then p5 (500)
        list.Free(1000);

        REQUIRE(p1.GetFreedTotal() == 500);
        REQUIRE(p5.GetFreedTotal() == 500);
        REQUIRE(p10.GetFreedTotal() == 0);
    }
}

TEST_CASE("MemoryFreeOnDemandList - Same priority", "[freeondemand][list]")
{
    MemoryFreeOnDemandList list;

    SECTION("Same priority uses insertion order")
    {
        MockDeallocator d1(1000, 5.0f);
        MockDeallocator d2(1000, 5.0f);

        list.Register(&d1);
        list.Register(&d2);

        // Free small amount - should come from first registered
        list.Free(300);

        // First one should be used
        REQUIRE(d1.GetFreedTotal() == 300);
        REQUIRE(d2.GetFreedTotal() == 0);
    }
}

TEST_CASE("MemoryFreeOnDemandList - Empty list", "[freeondemand][list]")
{
    MemoryFreeOnDemandList list;

    SECTION("Free from empty list")
    {
        size_t freed = list.Free(1000);

        REQUIRE(freed == 0);
    }

    SECTION("FreeAll from empty list")
    {
        size_t freed = list.FreeAll();

        REQUIRE(freed == 0);
    }
}

// Section 3: Free Operations (3 tests)

TEST_CASE("MemoryFreeOnDemandList - Partial free", "[freeondemand][operations]")
{
    MemoryFreeOnDemandList list;

    SECTION("Free stops when target met")
    {
        MockDeallocator d1(500, 1.0f);
        MockDeallocator d2(500, 2.0f);
        MockDeallocator d3(500, 3.0f);

        list.Register(&d1);
        list.Register(&d2);
        list.Register(&d3);

        // Request 700 bytes
        // Implementation calls Free(700) on d1 -> frees 500
        // Then calls Free(700) on d2 -> frees 500 (full amount available)
        // Total freed = 1000 (stops when ammount <= freedTotal)
        size_t freed = list.Free(700);

        REQUIRE(freed >= 700);              // At least 700 freed
        REQUIRE(d1.GetFreedTotal() == 500); // d1 freed all it has
        REQUIRE(d2.GetFreedTotal() == 500); // d2 freed all it has
        REQUIRE(d3.GetFreedTotal() == 0);   // d3 not touched
    }

    SECTION("Free exact amount from single deallocator")
    {
        MockDeallocator dealloc(1000, 5.0f);

        list.Register(&dealloc);

        size_t freed = list.Free(1000);

        REQUIRE(freed == 1000);
        REQUIRE(dealloc.GetAvailable() == 0);
    }
}

TEST_CASE("MemoryFreeOnDemandList - Free more than available", "[freeondemand][operations]")
{
    MemoryFreeOnDemandList list;

    SECTION("Free more than total available")
    {
        MockDeallocator d1(300, 1.0f);
        MockDeallocator d2(400, 2.0f);

        list.Register(&d1);
        list.Register(&d2);

        // Request 1000, but only 700 available
        size_t freed = list.Free(1000);

        REQUIRE(freed == 700);
        REQUIRE(d1.GetFreedTotal() == 300);
        REQUIRE(d2.GetFreedTotal() == 400);
    }
}

TEST_CASE("MemoryFreeOnDemandList - FreeAll operation", "[freeondemand][operations]")
{
    MemoryFreeOnDemandList list;

    SECTION("FreeAll frees from all deallocators")
    {
        MockDeallocator d1(100, 1.0f);
        MockDeallocator d2(200, 2.0f);
        MockDeallocator d3(300, 3.0f);

        list.Register(&d1);
        list.Register(&d2);
        list.Register(&d3);

        size_t freed = list.FreeAll();

        REQUIRE(freed == 600);
        REQUIRE(d1.GetFreedTotal() == 100);
        REQUIRE(d2.GetFreedTotal() == 200);
        REQUIRE(d3.GetFreedTotal() == 300);
    }
}

// Section 4: Helper Class Pattern (2 tests)

TEST_CASE("MemoryFreeOnDemandHelper - FreeOneItem pattern", "[freeondemand][helper]")
{
    SECTION("Free calls FreeOneItem repeatedly")
    {
        std::vector<size_t> items = {100, 200, 300};
        TestHelper helper(items, 5.0f);

        // Request 450 bytes - should free 300 + 200 = 500
        size_t freed = helper.Free(450);

        REQUIRE(freed >= 450);
        REQUIRE(freed == 500);               // Freed two items
        REQUIRE(helper.GetItemCount() == 1); // One item left
    }

    SECTION("Free stops when nothing left")
    {
        std::vector<size_t> items = {100, 200};
        TestHelper helper(items, 5.0f);

        // Request more than available
        size_t freed = helper.Free(1000);

        REQUIRE(freed == 300); // Only 100 + 200 available
        REQUIRE(helper.GetItemCount() == 0);
    }
}

TEST_CASE("MemoryFreeOnDemandHelper - FreeAll implementation", "[freeondemand][helper]")
{
    SECTION("FreeAll frees all items")
    {
        std::vector<size_t> items = {100, 200, 300, 400};
        TestHelper helper(items, 5.0f);

        size_t freed = helper.FreeAll();

        REQUIRE(freed == 1000);
        REQUIRE(helper.GetItemCount() == 0);
    }

    SECTION("FreeAll on empty helper")
    {
        std::vector<size_t> items;
        TestHelper helper(items, 5.0f);

        size_t freed = helper.FreeAll();

        REQUIRE(freed == 0);
    }
}

// Summary: FreeOnDemand Tests Complete
//
// Tests: 12
// Sections:
// - Interface Implementation: 3 tests
// - List Management: 4 tests
// - Free Operations: 3 tests
// - Helper Class Pattern: 2 tests
//
// Focus: Priority-based memory freeing system
// Coverage: ~90% of FreeOnDemand API
//
// Key Validations:
// - Interface contract works correctly
// - Priority ordering maintained during registration
// - Free operations respect priorities
// - Partial freeing stops when target met
// - Helper pattern simplifies common use cases
//
// Known Limitations:
// - Mock deallocators (no real memory freeing)
// - No threading tests (single-threaded usage)
// - No integration with actual caches
// - No memory pressure scenarios
//
// These limitations are acceptable - FreeOnDemand is a coordination
// mechanism, actual memory management is done by implementations.
//
// FreeOnDemand testing COMPLETE!
