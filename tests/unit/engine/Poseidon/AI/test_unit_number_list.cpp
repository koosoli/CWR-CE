#include <Poseidon/AI/UnitNumberList.hpp>
#include <catch2/catch_test_macros.hpp>

#include <initializer_list>
#include <string>
#include <Poseidon/Foundation/Containers/BoolArray.hpp>
#include <Poseidon/Foundation/Strings/RString.hpp>

namespace
{
PackedBoolArray UnitsFromIds(std::initializer_list<int> ids)
{
    PackedBoolArray list;
    for (int id : ids)
    {
        list.Set(id - 1, true); // ids are 1-based; bit positions are 0-based
    }
    return list;
}

std::string Joined(std::initializer_list<int> ids)
{
    return Poseidon::JoinUnitNumbers(UnitsFromIds(ids), 12).Data();
}
} // namespace

// Commanded group members are listed by their 1-based slot number; every
// selected slot stays comma-separated. The multi-unit and two-digit cases
// guard the separator the join must insert between adjacent ids.
TEST_CASE("JoinUnitNumbers comma-separates every selected unit", "[ai][radio]")
{
    // Five units, the case from the field report: each id stays apart, none
    // run together ("6, 7", never "67").
    REQUIRE(Joined({2, 3, 6, 7, 8}) == "2, 3, 6, 7, 8");

    // Full group, including two-digit ids past the single-digit run.
    REQUIRE(Joined({1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}) == "1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12");

    // The adjacent pair on its own.
    REQUIRE(Joined({6, 7}) == "6, 7");

    // Boundaries.
    REQUIRE(Joined({}) == "");
    REQUIRE(Joined({1}) == "1");
}
