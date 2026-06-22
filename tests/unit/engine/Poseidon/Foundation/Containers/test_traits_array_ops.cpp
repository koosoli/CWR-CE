/*
 * Tests AutoArray<T> element construction, grow, copy, and destruction through
 * ModernTraits<T>. The load-bearing invariant is ctor/dtor balance: every
 * element constructed must be destructed exactly once.
 *
 * Broken-state deltas:
 * - "ctor/dtor balance across grow": change REQUIRE(constructed == c.dtors) to
 *   REQUIRE(constructed == c.dtors + 1); test fails (e.g. 32 != 33).
 * - "Clear calls dtors": change REQUIRE(c.dtors == beforeClear + 8) to
 *   REQUIRE(c.dtors == beforeClear + 9); test fails (8 dtors, not 9).
 * - "copy uses per-element assign": AutoArray::Copy calls Resize (defaultCtors) then
 *   CopyData (copyAssigns), not CopyConstruct. Change REQUIRE(c.copyAssigns >= 4) to
 *   REQUIRE(c.copyAssigns >= 1000); test fails (copyAssigns == 4).
 */

#include "OpCounter.hpp"
#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Foundation/Containers/Array.hpp>

using namespace Poseidon::Foundation::ContainerTests;

struct TagAutoArrayGrow
{
};
struct TagAutoArrayClear
{
};
struct TagAutoArrayCopy
{
};

TEST_CASE("AutoArray<TrivialPod> grow keeps elements observable", "[containers][traits][autoarray]")
{
    AutoArray<TrivialPod> a;
    for (int i = 0; i < 1000; ++i)
        a.Add({i, i * 2});
    REQUIRE(a.Size() == 1000);
    REQUIRE(a[500].a == 500);
    REQUIRE(a[500].b == 1000);
}

TEST_CASE("AutoArray<Counted> ctor/dtor balance across grow", "[containers][traits][autoarray]")
{
    Counted<TagAutoArrayGrow>::Counts().Reset();
    {
        AutoArray<Counted<TagAutoArrayGrow>> a;
        for (int i = 0; i < 16; ++i)
            a.Add(Counted<TagAutoArrayGrow>{i});
    }
    const auto& c = Counted<TagAutoArrayGrow>::Counts();
    const int constructed = c.defaultCtors + c.copyCtors + c.moveCtors;
    REQUIRE(constructed == c.dtors);
    REQUIRE(c.dtors > 0);
}

TEST_CASE("AutoArray<Counted> Clear calls dtors", "[containers][traits][autoarray]")
{
    Counted<TagAutoArrayClear>::Counts().Reset();
    AutoArray<Counted<TagAutoArrayClear>> a;
    for (int i = 0; i < 8; ++i)
        a.Add(Counted<TagAutoArrayClear>{i});
    const int beforeClear = Counted<TagAutoArrayClear>::Counts().dtors;
    a.Clear();
    REQUIRE(Counted<TagAutoArrayClear>::Counts().dtors == beforeClear + 8);
}

TEST_CASE("AutoArray<Counted> copy uses per-element assign", "[containers][traits][autoarray]")
{
    // AutoArray::AutoArray(const AutoArray&) calls Copy() which calls Resize (defaultCtors)
    // then CopyData (copyAssigns). It does NOT call CopyConstruct; copyCtors stays 0.
    // This is the correct behaviour for the legacy generic path.
    AutoArray<Counted<TagAutoArrayCopy>> a;
    for (int i = 0; i < 4; ++i)
        a.Add(Counted<TagAutoArrayCopy>{i});
    Counted<TagAutoArrayCopy>::Counts().Reset();
    AutoArray<Counted<TagAutoArrayCopy>> b(a);
    const auto& c = Counted<TagAutoArrayCopy>::Counts();
    REQUIRE(c.copyAssigns >= 4);
    REQUIRE(b.Size() == 4);
}

// Copying/assigning an EMPTY trivially-copyable AutoArray runs ModernTraits::CopyData /
// MoveData with n==0 and a null data pointer. memcpy/memmove require non-null pointers even
// for 0 bytes (C11 7.24.1p2), so the unguarded call is UB: under the sanitizer build it
// reports "null pointer passed as argument 1/2, which is declared to never be null" — the
// single most frequent boot-time UBSan finding. The guard (n > 0) makes the no-op truly a
// no-op. Teeth: build PoseidonFoundationTests under linux-x64-clang-san and run with
// UBSAN_OPTIONS=halt_on_error=1 — without the guard this case aborts; with it, clean.
TEST_CASE("AutoArray empty copy/move does not pass null to memcpy", "[containers][traits][autoarray][ubsan]")
{
    AutoArray<int> empty; // trivially copyable, no allocation -> null data
    REQUIRE(empty.Size() == 0);

    AutoArray<int> copyCtor(empty); // CopyData(null, null, 0)
    REQUIRE(copyCtor.Size() == 0);

    AutoArray<int> copyAssign;
    copyAssign.Add(7);
    copyAssign = empty; // shrinks to 0, then CopyData(null, null, 0)
    REQUIRE(copyAssign.Size() == 0);

    AutoArray<int> moveTgt(std::move(copyCtor)); // MoveData path with empty source
    REQUIRE(moveTgt.Size() == 0);
}
