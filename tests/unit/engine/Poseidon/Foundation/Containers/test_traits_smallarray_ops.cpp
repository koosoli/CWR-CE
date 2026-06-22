/*
 * Tests SmallArray<T> inline-buffer and heap-overflow paths.
 * SmallArray<T, 64> uses VerySmallArray<T, 48> internally (MaxSmall = 44/sizeof(T)).
 * For Counted<Tag> (4-byte payload), MaxSmall = 11; elements 12+ go to the
 * embedded AutoArray. The ctor/dtor balance must hold across both paths.
 *
 * Broken-state deltas:
 * - "inline ctor/dtor balance": change REQUIRE(constructed == c.dtors) to
 *   REQUIRE(constructed == c.dtors + 1); test fails (balance holds for <=11 elements).
 * - "promotion ctor/dtor balance": same change; test fails for >11 elements.
 * - "TrivialPod promotion preserves values": change REQUIRE(arr[0].a == 0) to
 *   REQUIRE(arr[0].a == 99); test fails (value is 0).
 */

#include "OpCounter.hpp"
#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Foundation/Containers/SmallArray.hpp>

using namespace Poseidon::Foundation::ContainerTests;

struct TagSmallArrayInline
{
};
struct TagSmallArrayPromote
{
};

TEST_CASE("SmallArray<Counted> inline ctor/dtor balance", "[containers][traits][smallarray]")
{
    Counted<TagSmallArrayInline>::Counts().Reset();
    {
        SmallArray<Counted<TagSmallArrayInline>> arr;
        for (int i = 0; i < 8; ++i)
            arr.Add(Counted<TagSmallArrayInline>{i});
        REQUIRE(arr.Size() == 8);
    }
    const auto& c = Counted<TagSmallArrayInline>::Counts();
    const int constructed = c.defaultCtors + c.copyCtors + c.moveCtors;
    REQUIRE(constructed == c.dtors);
    REQUIRE(c.dtors > 0);
}

TEST_CASE("SmallArray<Counted> promotion ctor/dtor balance", "[containers][traits][smallarray]")
{
    Counted<TagSmallArrayPromote>::Counts().Reset();
    {
        SmallArray<Counted<TagSmallArrayPromote>> arr;
        for (int i = 0; i < 20; ++i)
            arr.Add(Counted<TagSmallArrayPromote>{i});
        REQUIRE(arr.Size() == 20);
    }
    const auto& c = Counted<TagSmallArrayPromote>::Counts();
    const int constructed = c.defaultCtors + c.copyCtors + c.moveCtors;
    REQUIRE(constructed == c.dtors);
    REQUIRE(c.dtors > 0);
}

TEST_CASE("SmallArray<TrivialPod> promotion preserves element values", "[containers][traits][smallarray]")
{
    SmallArray<TrivialPod> arr;
    for (int i = 0; i < 20; ++i)
        arr.Add({i, i * 3});
    REQUIRE(arr.Size() == 20);
    REQUIRE(arr[0].a == 0);
    REQUIRE(arr[5].a == 5);
    REQUIRE(arr[5].b == 15);
    REQUIRE(arr[19].a == 19);
    REQUIRE(arr[19].b == 57);
}
