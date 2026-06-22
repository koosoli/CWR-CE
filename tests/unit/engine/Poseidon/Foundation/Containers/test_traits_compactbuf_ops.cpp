/*
 * Tests CompactBuffer<T> construction, copy-construction, and destruction.
 *
 * Implementation note: CompactBuffer has a Type _data[1] data member. C++ behaviour
 * for non-trivial Type:
 * - Construction: _data[0] is default-initialised before the constructor body runs
 *   (member init, +1 defaultCtors), then ConstructArray(n) placement-news over all n
 *   slots — net defaultCtors = n + 1.
 * - Destruction: the destructor body calls DestructArray(_data, n) which destructs
 *   all n slots including _data[0]; then C++ implicitly destructs the _data[0] member
 *   again — net dtors = n + 1 (double-destruction of _data[0]; UB for non-trivial
 *   types, harmless for our counter since Counted dtors are idempotent in effect).
 * For trivial types (CompactBuffer's intended usage) both issues vanish.
 *
 * Broken-state deltas:
 * - "New constructs ConstructArray path": change REQUIRE(c.defaultCtors == n + 1) to
 *   REQUIRE(c.defaultCtors == n + 2); test fails (defaultCtors is n+1).
 * - "Copy copy-constructs n elements": change REQUIRE(c.copyCtors == n) to
 *   REQUIRE(c.copyCtors == n + 1); test fails (exactly n copy-constructed).
 * - "Delete destructs via DestructArray": change REQUIRE(c.dtors == n + 1) to
 *   REQUIRE(c.dtors == n + 2); test fails (dtors is n+1).
 */

#include "OpCounter.hpp"
#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Foundation/Containers/CompactBuf.hpp>

using namespace Poseidon::Foundation::ContainerTests;

struct TagCBConstruct
{
};
struct TagCBCopy
{
};
struct TagCBDestruct
{
};

TEST_CASE("CompactBuffer<Counted> New constructs ConstructArray path", "[containers][traits][compactbuf]")
{
    const int n = 5;
    Counted<TagCBConstruct>::Counts().Reset();
    auto* buf = CompactBuffer<Counted<TagCBConstruct>>::New(n);
    const auto& c = Counted<TagCBConstruct>::Counts();
    REQUIRE(c.defaultCtors == n + 1);
    buf->Delete();
    REQUIRE(c.dtors == n + 1);
}

TEST_CASE("CompactBuffer<Counted> Copy copy-constructs n elements", "[containers][traits][compactbuf]")
{
    const int n = 4;
    auto* src = CompactBuffer<Counted<TagCBCopy>>::New(n);
    Counted<TagCBCopy>::Counts().Reset();
    auto* copy = CompactBuffer<Counted<TagCBCopy>>::Copy(src);
    const auto& c = Counted<TagCBCopy>::Counts();
    REQUIRE(c.copyCtors == n);
    copy->Delete();
    REQUIRE(c.dtors == n + 1);
    src->Delete();
}

TEST_CASE("CompactBuffer<Counted> Delete destructs via DestructArray", "[containers][traits][compactbuf]")
{
    const int n = 6;
    Counted<TagCBDestruct>::Counts().Reset();
    auto* buf = CompactBuffer<Counted<TagCBDestruct>>::New(n);
    REQUIRE(Counted<TagCBDestruct>::Counts().defaultCtors == n + 1);
    buf->Delete();
    REQUIRE(Counted<TagCBDestruct>::Counts().dtors == n + 1);
}
