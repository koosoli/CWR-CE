// FastCAlloc — fixed-size pool allocator used by 100+ engine classes via
// USE_FAST_ALLOCATOR.  It was disabled on x64 (fell back to malloc) because it
// "crashed": CAlloc hands out (element + sizeof(void*)), and the first element was
// offset by a hardcoded 12 (the 32-bit value), leaving x64 user pointers at +4 mod
// 16 — misaligned — so any 16B-aligned object (SIMD/Vector members) faulted.  The
// fix offsets by (16 - sizeof(void*)) = 8 on x64, restoring 16B alignment and the
// x86 allocator behaviour 1:1.  Without the fix the alignment REQUIREs below report
// `4 == 0` on x64.

#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Foundation/Memory/FastAlloc.hpp>

#include <cstdint>
#include <cstring>
#include <set>
#include <vector>

using Poseidon::Foundation::FastCAlloc;

static bool aligned16(void* p)
{
    return (reinterpret_cast<uintptr_t>(p) & 15u) == 0;
}

TEST_CASE("FastCAlloc returns 16B-aligned, distinct, non-overlapping blocks", "[fastalloc][memory]")
{
    constexpr size_t OBJ = 64; // > 32 -> esize is a multiple of 16, 16B alignment contract
    FastCAlloc alloc(OBJ, "test64");

    std::vector<void*> ptrs;
    for (int i = 0; i < 500; i++) // spans several 16KB chunks
    {
        void* p = alloc.CAlloc(OBJ);
        REQUIRE(p != nullptr);
        REQUIRE(aligned16(p)); // teeth: == 4 (mod 16) on the broken x64 alignOffset
        ptrs.push_back(p);
    }

    // distinct
    std::set<void*> uniq(ptrs.begin(), ptrs.end());
    REQUIRE(uniq.size() == ptrs.size());

    // non-overlapping: stamp each block, then verify nothing was trampled (would catch
    // a wrong element stride or a clobbered chunk-pointer header)
    for (size_t i = 0; i < ptrs.size(); i++)
    {
        std::memset(ptrs[i], static_cast<int>(i & 0xff), OBJ);
    }
    for (size_t i = 0; i < ptrs.size(); i++)
    {
        const unsigned char* b = static_cast<const unsigned char*>(ptrs[i]);
        for (size_t k = 0; k < OBJ; k++)
        {
            REQUIRE(b[k] == static_cast<unsigned char>(i & 0xff));
        }
    }

    for (void* p : ptrs)
    {
        alloc.CFree(p);
    }
}

TEST_CASE("FastCAlloc reuses freed blocks", "[fastalloc][memory]")
{
    FastCAlloc alloc(48, "test48");
    void* a = alloc.CAlloc(48);
    REQUIRE(aligned16(a));
    alloc.CFree(a);
    void* b = alloc.CAlloc(48); // LIFO freelist -> same slot comes back
    REQUIRE(b == a);
    alloc.CFree(b);
}

TEST_CASE("FastCAlloc survives heavy alloc/free churn without corruption", "[fastalloc][memory][stress]")
{
    FastCAlloc alloc(80, "test80");
    std::vector<void*> live;
    for (int round = 0; round < 5000; round++)
    {
        if (live.size() < 64 && (round & 1) == 0)
        {
            void* p = alloc.CAlloc(80);
            REQUIRE(p != nullptr);
            REQUIRE(aligned16(p));
            std::memset(p, 0x5A, 80); // touch the whole block
            live.push_back(p);
        }
        else if (!live.empty())
        {
            alloc.CFree(live.back());
            live.pop_back();
        }
    }
    for (void* p : live)
    {
        alloc.CFree(p);
    }
}
