/*
 * Tests FindArray<T> append, find, and remove paths through ModernTraits<T>.
 * FindArray inherits from AutoArray so the grow path is the same; the main
 * addition is Find/Delete which exercises equality and element shifting.
 *
 * Broken-state deltas:
 * - "ctor/dtor balance across add/remove": change REQUIRE(constructed == c.dtors)
 *   to REQUIRE(constructed == c.dtors + 1); test fails (balance holds).
 * - "append and find": change REQUIRE(a.Find({42, 99}) == 0) to
 *   REQUIRE(a.Find({42, 99}) == 1); test fails (element is at index 0).
 */

#include "OpCounter.hpp"
#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Foundation/Containers/Array.hpp>

using namespace Poseidon::Foundation::ContainerTests;

struct TagFindArrayBalance
{
};

TEST_CASE("FindArray<TrivialPod> append and find keeps invariants", "[containers][traits][findarray]")
{
    FindArray<TrivialPod> a;
    a.Add({42, 99});
    a.Add({1, 2});
    REQUIRE(a.Size() == 2);
    REQUIRE(a.Find({42, 99}) == 0);
    REQUIRE(a.Find({1, 2}) == 1);
    REQUIRE(a.Find({0, 0}) == -1);
    a.Delete(0);
    REQUIRE(a.Size() == 1);
    REQUIRE(a[0].a == 1);
}

TEST_CASE("FindArray<Counted> ctor/dtor balance across add/remove", "[containers][traits][findarray]")
{
    Counted<TagFindArrayBalance>::Counts().Reset();
    {
        FindArray<Counted<TagFindArrayBalance>> a;
        for (int i = 0; i < 8; ++i)
            a.Add(Counted<TagFindArrayBalance>{i});
        REQUIRE(a.Size() == 8);
        a.Delete(Counted<TagFindArrayBalance>{3});
        REQUIRE(a.Size() == 7);
    }
    const auto& c = Counted<TagFindArrayBalance>::Counts();
    const int constructed = c.defaultCtors + c.copyCtors + c.moveCtors;
    REQUIRE(constructed == c.dtors);
    REQUIRE(c.dtors > 0);
}
