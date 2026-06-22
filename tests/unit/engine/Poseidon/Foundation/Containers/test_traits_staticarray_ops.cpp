/*
 * Tests StaticArray<T> (AutoArray with MemAllocSS, falling back to dynamic
 * allocation when no static storage is set). Resize(n) default-constructs
 * exactly n elements; scope exit destructs exactly n elements.
 *
 * Broken-state deltas:
 * - "Resize constructs exactly n elements": change REQUIRE(c.defaultCtors == 8)
 *   to REQUIRE(c.defaultCtors == 9); test fails (exactly 8 constructed).
 * - "scope-exit destructs match constructs": change REQUIRE(c.dtors == 8) to
 *   REQUIRE(c.dtors == 9); test fails (exactly 8 destructed).
 * - "TrivialPod fill and read-back": change REQUIRE(arr[1].b == 4) to
 *   REQUIRE(arr[1].b == 5); test fails (value is 4).
 */

#include "OpCounter.hpp"
#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Foundation/Containers/StaticArray.hpp>

using namespace Poseidon::Foundation::ContainerTests;

struct TagSAFill
{
};

TEST_CASE("StaticArray<Counted> Resize constructs exactly n elements and dtors on scope exit",
          "[containers][traits][staticarray]")
{
    Counted<TagSAFill>::Counts().Reset();
    {
        StaticArray<Counted<TagSAFill>> arr;
        arr.Resize(8);
        const auto& c = Counted<TagSAFill>::Counts();
        REQUIRE(c.defaultCtors == 8);
    }
    REQUIRE(Counted<TagSAFill>::Counts().dtors == 8);
}

TEST_CASE("StaticArray<TrivialPod> fill and read-back roundtrip", "[containers][traits][staticarray]")
{
    StaticArray<TrivialPod> arr;
    arr.Resize(4);
    arr[0] = {1, 2};
    arr[1] = {3, 4};
    arr[2] = {5, 6};
    arr[3] = {7, 8};
    REQUIRE(arr[0].a == 1);
    REQUIRE(arr[1].b == 4);
    REQUIRE(arr[2].a == 5);
    REQUIRE(arr[3].b == 8);
}
