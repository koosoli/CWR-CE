// InGameUIImpl.hpp is not self-contained — it needs World/AI types (TargetId, AIRadio)
// defined first, the same order InGameUIMenuSim.cpp includes them.
#include <Poseidon/World/World.hpp>
#include <Poseidon/AI/AI.hpp>
#include <Poseidon/World/Entities/Infantry/Person.hpp>
#include <Poseidon/Input/InputSubsystem.hpp>
#include <Poseidon/UI/InGame/InGameUI.hpp>
#include <Poseidon/UI/InGame/InGameUIImpl.hpp>
#include <Poseidon/UI/InGame/InGameUIGroupUnitLabel.hpp>
#include <Poseidon/Core/resincl.hpp>
#include <catch2/catch_test_macros.hpp>

#include <string>

using namespace Poseidon;
TEST_CASE("inGameUI compiles", "[inGameUI][tier3]")
{
    REQUIRE(sizeof(AbstractUI) > 0);
}

TEST_CASE("InGameUI group unit labels hide debug subgroup leaders in release HUD", "[inGameUI][groupInfo]")
{
    char label[8];

    FormatGroupUnitLabel(label, sizeof(label), 2, 2, false);
    REQUIRE(std::string(label) == "2");

    FormatGroupUnitLabel(label, sizeof(label), 2, 2, true);
    REQUIRE(std::string(label) == "2(2)");
}

// A mission's description.ext can override/break RscMainMenu so it has no movement
// command sub-menu. InGameUI::InitMenu then read a null FindMenu(CMD_MOVE_FIRST) result
// and dereferenced it (menuDist->_parent at +0x30) — 0xC0000005 in InitMenu+0xbd
// The fix guards the null and
// skips the move-menu wiring; without it, WireMovementCommandMenu(&emptyMenu) AVs.
TEST_CASE("InGameUI move-menu wiring tolerates a RscMainMenu without move commands", "[inGameUI][menu]")
{
    Menu* menu = new Menu(); // empty — no movement command sub-menu

    REQUIRE(menu->FindMenu(CMD_MOVE_FIRST, true) == nullptr); // the crashing precondition

    InGameUI::WireMovementCommandMenu(menu); // must not crash (was a null deref pre-fix)

    SUCCEED("survived a RscMainMenu missing the movement command menu");
    delete menu;
}
