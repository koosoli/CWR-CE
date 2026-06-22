// JimboAllocator Tests
// Verifies the memory allocator works correctly on both x86 and x64

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <Poseidon/Foundation/Memory/JimboAllocator.hpp>
#include <vector>
#include <cstring>
#include <algorithm>
#include <stdint.h>
#include <Poseidon/Foundation/Memory/CheckMem.hpp>

using Poseidon::Foundation::GMemFunctions;

TEST_CASE("JimboAllocator basic allocation", "[memory][allocator]")
{
    SECTION("Allocate and free single block")
    {
        void* ptr = GMemFunctions()->New(100);
        REQUIRE(ptr != nullptr);

        // Should be able to write to allocated memory
        std::memset(ptr, 0xAB, 100);

        GMemFunctions()->Delete(ptr);
    }

    SECTION("Allocate zero bytes returns nullptr")
    {
        void* ptr = GMemFunctions()->New(0);
        REQUIRE(ptr == nullptr);
    }

    SECTION("Delete nullptr is safe")
    {
        GMemFunctions()->Delete(nullptr);
        // Should not crash
    }

    SECTION("Allocate various sizes")
    {
        std::vector<void*> ptrs;
        size_t sizes[] = {1, 8, 16, 32, 64, 128, 256, 512, 1024, 4096, 65536};

        for (size_t size : sizes)
        {
            void* ptr = GMemFunctions()->New(size);
            REQUIRE(ptr != nullptr);
            ptrs.push_back(ptr);
        }

        for (void* ptr : ptrs)
        {
            GMemFunctions()->Delete(ptr);
        }
    }
}

// Regression: a hard memory ceiling must NEVER make New() return null.
//
// A user set a hard limit in the dev panel ("compact memory")
// while playing; ~10s later the terrain allocated a segment and crashed in
// VertexTable::AddVertex -> _pos.Resize() -> memset. The engine has thousands of
// unchecked `new` sites that assume allocation never fails; a budget that refuses
// over the ceiling hands them null and they write into it. The ceiling must evict
// caches, never refuse.
//
// Broken-state delta: restore the `if (!_budget.Reserve(size)) return nullptr;`
// reject branch in JimboAllocator::New (and the over-ceiling `return false` in
// ProcessMemoryBudget::Reserve) and `over` below is null → the REQUIRE fails,
// exactly as the engine crashed.
TEST_CASE("JimboAllocator hard ceiling evicts but never returns null", "[memory][allocator]")
{
    Poseidon::Foundation::JimboAllocator alloc;
    alloc.SetBudgetLimits(/*soft*/ 0, /*hard*/ 1); // 1-byte ceiling: every alloc is "over"

    // A terrain-segment-sized block, far past the ceiling. Old code returned null.
    void* over = alloc.New(64 * 1024);
    REQUIRE(over != nullptr);

    // Prove it is real, addressable memory — this is the write that crashed
    // (VertexTable's memset into a null buffer).
    std::memset(over, 0xCD, 64 * 1024);
    alloc.Delete(over);

    // Still admits on the next allocation (the user kept playing after compacting).
    void* again = alloc.New(128 * 1024);
    REQUIRE(again != nullptr);
    alloc.Delete(again);
}

namespace
{
// A registrable evictable cache: Free(amount) sheds up to `amount` of held bytes.
// Models a real FreeOnDemand registrant (TextBank, file cache, …) for the
// allocator's FrameMaintenance path.
struct FakeCache : public Poseidon::Foundation::IMemoryFreeOnDemand
{
    size_t held;
    float prio;
    size_t budget;
    int freeCalls = 0;

    FakeCache(size_t h, float p, size_t b = 0) : held(h), prio(p), budget(b) {}

    size_t Free(size_t amount) override
    {
        ++freeCalls;
        const size_t freed = amount < held ? amount : held;
        held -= freed;
        return freed;
    }
    size_t FreeAll() override
    {
        const size_t f = held;
        held = 0;
        return f;
    }
    float Priority() override { return prio; }
    const char* DomainName() const override { return "FakeCache"; }
    size_t HeldBytes() const override { return held; }
    size_t BudgetBytes() const override { return budget; }
};
} // namespace

// FrameMaintenance is where crossing a budget turns into eviction — off the
// allocation path, once per frame. Reserve() itself never evicts (pure accounting),
// so the cache stays full until the frame tick runs.
//
// Broken-state delta: if Reserve still evicted (the old per-alloc model), freeCalls
// would already be >0 before FrameMaintenance; if FrameMaintenance didn't claw back
// over the hard ceiling, freeCalls would stay 0 after it.
TEST_CASE("JimboAllocator FrameMaintenance evicts over the hard ceiling", "[memory][allocator]")
{
    Poseidon::Foundation::JimboAllocator alloc;
    FakeCache cache(/*held*/ 8 * 1024 * 1024, /*priority*/ 0.1f);
    alloc.RegisterFreeOnDemand(&cache);
    alloc.SetBudgetLimits(/*soft*/ 0, /*hard*/ 1 * 1024 * 1024);

    // Push the process budget well over the hard ceiling. New never refuses.
    void* p = alloc.New(4 * 1024 * 1024);
    REQUIRE(p != nullptr);
    REQUIRE(cache.freeCalls == 0); // Reserve did not evict

    const size_t heldBefore = cache.held;
    const size_t freed = alloc.FrameMaintenance();
    REQUIRE(cache.freeCalls > 0); // the frame tick drove eviction
    REQUIRE(freed > 0);
    REQUIRE(cache.held < heldBefore); // cache was actually trimmed

    alloc.Delete(p);
}

// Under both watermarks (the common case — limits default off) the tick must be a
// cheap no-op: it must not walk into the registry or evict anything.
TEST_CASE("JimboAllocator FrameMaintenance is a no-op under budget", "[memory][allocator]")
{
    Poseidon::Foundation::JimboAllocator alloc;
    FakeCache cache(/*held*/ 8 * 1024 * 1024, /*priority*/ 0.1f);
    alloc.RegisterFreeOnDemand(&cache);
    alloc.SetBudgetLimits(/*soft*/ 64 * 1024 * 1024, /*hard*/ 128 * 1024 * 1024);

    void* p = alloc.New(1 * 1024 * 1024); // far under both limits
    REQUIRE(p != nullptr);

    REQUIRE(alloc.FrameMaintenance() == 0);
    REQUIRE(cache.freeCalls == 0);
    REQUIRE(cache.held == 8 * 1024 * 1024);

    alloc.Delete(p);
}

TEST_CASE("JimboAllocator alignment", "[memory][allocator]")
{
    SECTION("Allocations are 16-byte aligned")
    {
        // spot-check representative sizes instead of exhaustive loop
        int sizes[] = {1, 2, 7, 8, 15, 16, 17, 31, 32, 50, 63, 64, 99, 100};
        for (int size : sizes)
        {
            void* ptr = GMemFunctions()->New(size);
            REQUIRE(ptr != nullptr);
            uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
            REQUIRE((addr % 16) == 0);
            GMemFunctions()->Delete(ptr);
        }
    }
}

TEST_CASE("JimboAllocator statistics", "[memory][allocator]")
{
    SECTION("HeapUsed increases after allocation")
    {
        size_t before = GMemFunctions()->HeapUsed();

        void* ptr = GMemFunctions()->New(1024);
        REQUIRE(ptr != nullptr);

        size_t after = GMemFunctions()->HeapUsed();
        REQUIRE(after > before);

        GMemFunctions()->Delete(ptr);
    }

    SECTION("MemoryAllocatedBlocks tracks allocations")
    {
        int before = GMemFunctions()->MemoryAllocatedBlocks();

        void* ptr1 = GMemFunctions()->New(100);
        void* ptr2 = GMemFunctions()->New(200);

        int afterAlloc = GMemFunctions()->MemoryAllocatedBlocks();
        REQUIRE(afterAlloc >= before + 2);

        GMemFunctions()->Delete(ptr1);
        GMemFunctions()->Delete(ptr2);
    }

    SECTION("CheckIntegrity returns true")
    {
        REQUIRE(GMemFunctions()->CheckIntegrity() == true);
    }

    SECTION("IsOutOfMemory is false initially")
    {
        REQUIRE(GMemFunctions()->IsOutOfMemory() == false);
    }
}

TEST_CASE("JimboAllocator stress test", "[memory][allocator][stress]")
{
    SECTION("Many small allocations")
    {
        const int count = 1000;
        std::vector<void*> ptrs;
        ptrs.reserve(count);

        for (int i = 0; i < count; ++i)
        {
            void* ptr = GMemFunctions()->New(16);
            ptrs.push_back(ptr);
        }

        // spot-check first, middle, last
        REQUIRE(ptrs[0] != nullptr);
        REQUIRE(ptrs[count / 2] != nullptr);
        REQUIRE(ptrs[count - 1] != nullptr);
        // verify all non-null
        bool allValid = std::all_of(ptrs.begin(), ptrs.end(), [](void* p) { return p != nullptr; });
        REQUIRE(allValid);

        for (void* ptr : ptrs)
            GMemFunctions()->Delete(ptr);
    }

    SECTION("Mixed size allocations")
    {
        const int count = 500;
        std::vector<void*> ptrs;
        ptrs.reserve(count);

        for (int i = 0; i < count; ++i)
        {
            size_t size = ((i * 17) % 4096) + 1; // Pseudo-random sizes 1-4096
            void* ptr = GMemFunctions()->New(size);
            ptrs.push_back(ptr);
        }

        REQUIRE(ptrs[0] != nullptr);
        REQUIRE(ptrs[count / 2] != nullptr);
        REQUIRE(ptrs[count - 1] != nullptr);
        bool allValid = std::all_of(ptrs.begin(), ptrs.end(), [](void* p) { return p != nullptr; });
        REQUIRE(allValid);

        for (auto it = ptrs.rbegin(); it != ptrs.rend(); ++it)
            GMemFunctions()->Delete(*it);
    }
}

TEST_CASE("JimboAllocator new/delete operators", "[memory][allocator]")
{
    SECTION("Global new/delete work")
    {
        int* p = new int(42);
        REQUIRE(p != nullptr);
        REQUIRE(*p == 42);
        delete p;
    }

    SECTION("Array new/delete work")
    {
        int* arr = new int[100];
        REQUIRE(arr != nullptr);

        for (int i = 0; i < 100; ++i)
            arr[i] = i;

        // spot-check first, middle, last
        REQUIRE(arr[0] == 0);
        REQUIRE(arr[50] == 50);
        REQUIRE(arr[99] == 99);

        delete[] arr;
    }

    SECTION("Class with virtual functions")
    {
        struct Base
        {
            virtual ~Base() = default;
            virtual int value() const = 0;
        };

        struct Derived : Base
        {
            int _val;
            Derived(int v) : _val(v) {}
            int value() const override { return _val; }
        };

        Base* obj = new Derived(123);
        REQUIRE(obj != nullptr);
        REQUIRE(obj->value() == 123);
        delete obj;
    }
}

TEST_CASE("JimboAllocator CleanUp", "[memory][allocator]")
{
    SECTION("CleanUp does not crash")
    {
        GMemFunctions()->CleanUp();
        // Should not crash and allocator should still work

        void* ptr = GMemFunctions()->New(100);
        REQUIRE(ptr != nullptr);
        GMemFunctions()->Delete(ptr);
    }
}
