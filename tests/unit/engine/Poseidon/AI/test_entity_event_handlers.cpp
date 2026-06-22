#include <catch2/catch_test_macros.hpp>

#include <Poseidon/AI/EntityAIType.hpp>
#include <Poseidon/Foundation/Enums/EnumNames.hpp>

using namespace Poseidon;

TEST_CASE("EntityEvent enum names stay aligned with legacy event handler contract", "[ai][events]")
{
    struct Expected
    {
        EntityEvent value;
        const char* name;
    };

    const Expected expected[] = {
        {EEKilled, "Killed"},
        {EEHit, "Hit"},
        {EEEngine, "Engine"},
        {EEGetIn, "GetIn"},
        {EEGetOut, "GetOut"},
        {EEFired, "Fired"},
        {EEIncomingMissile, "IncomingMissile"},
        {EEDammaged, "Dammaged"},
        {EEGear, "Gear"},
        {EEFuel, "Fuel"},
        {EEAnimChanged, "AnimChanged"},
        {EEInit, "Init"},
    };

    REQUIRE(Foundation::GetEnumCount(EntityEvent()) == NEntityEvent);
    REQUIRE(static_cast<int>(NEntityEvent) == static_cast<int>(sizeof(expected) / sizeof(expected[0])));

    for (const Expected& entry : expected)
    {
        CAPTURE(entry.name);
        CHECK(Foundation::FindEnumName(entry.value) == RStringB(entry.name));
        CHECK(Foundation::GetEnumValue<EntityEvent>(entry.name) == entry.value);
    }
}
