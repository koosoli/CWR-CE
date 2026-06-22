#include <Poseidon/UI/GameModule.hpp>
#include <catch2/catch_test_macros.hpp>
#include <functional>
#include <string>
#include <vector>

using Poseidon::GameModuleId;
using Poseidon::GameModuleRegistry;

TEST_CASE("GameModuleRegistry: default empty", "[UI][GameModule]")
{
    GameModuleRegistry::Clear();
    REQUIRE(GameModuleRegistry::All().empty());
    REQUIRE_FALSE(GameModuleRegistry::IsRegistered(GameModuleId::Missions));
    REQUIRE_FALSE(GameModuleRegistry::IsRegistered(GameModuleId::Campaigns));
    REQUIRE_FALSE(GameModuleRegistry::IsRegistered(GameModuleId::Multiplayer));
    REQUIRE_FALSE(GameModuleRegistry::IsRegistered(GameModuleId::Editor));
    REQUIRE(GameModuleRegistry::Find(GameModuleId::Missions) == nullptr);
    REQUIRE(GameModuleRegistry::FindByIDC(117) == nullptr);
}

TEST_CASE("GameModuleRegistry: register and query", "[UI][GameModule]")
{
    GameModuleRegistry::Clear();

    bool called = false;
    GameModuleRegistry::Register(
        {GameModuleId::Missions, "Single Missions", 117, [&called](ControlsContainer*) { called = true; }});

    REQUIRE(GameModuleRegistry::IsRegistered(GameModuleId::Missions));
    REQUIRE_FALSE(GameModuleRegistry::IsRegistered(GameModuleId::Campaigns));
    REQUIRE(GameModuleRegistry::All().size() == 1);

    auto* mod = GameModuleRegistry::Find(GameModuleId::Missions);
    REQUIRE(mod != nullptr);
    REQUIRE(mod->menuButtonIDC == 117);
    REQUIRE(std::string(mod->name) == "Single Missions");

    mod->menuAction(nullptr);
    REQUIRE(called);
}

TEST_CASE("GameModuleRegistry: FindByIDC", "[UI][GameModule]")
{
    GameModuleRegistry::Clear();

    GameModuleRegistry::Register({GameModuleId::Campaigns, "Campaigns", 101, nullptr});
    GameModuleRegistry::Register({GameModuleId::Multiplayer, "Multiplayer", 105, nullptr});

    auto* byIDC = GameModuleRegistry::FindByIDC(101);
    REQUIRE(byIDC != nullptr);
    REQUIRE(byIDC->id == GameModuleId::Campaigns);

    byIDC = GameModuleRegistry::FindByIDC(105);
    REQUIRE(byIDC != nullptr);
    REQUIRE(byIDC->id == GameModuleId::Multiplayer);

    REQUIRE(GameModuleRegistry::FindByIDC(999) == nullptr);
}

TEST_CASE("GameModuleRegistry: multiple modules", "[UI][GameModule]")
{
    GameModuleRegistry::Clear();

    GameModuleRegistry::Register({GameModuleId::Missions, "SP", 117, nullptr});
    GameModuleRegistry::Register({GameModuleId::Campaigns, "Camp", 101, nullptr});
    GameModuleRegistry::Register({GameModuleId::Multiplayer, "MP", 105, nullptr});
    GameModuleRegistry::Register({GameModuleId::Editor, "Ed", 115, nullptr});

    REQUIRE(GameModuleRegistry::All().size() == 4);
    REQUIRE(GameModuleRegistry::IsRegistered(GameModuleId::Missions));
    REQUIRE(GameModuleRegistry::IsRegistered(GameModuleId::Campaigns));
    REQUIRE(GameModuleRegistry::IsRegistered(GameModuleId::Multiplayer));
    REQUIRE(GameModuleRegistry::IsRegistered(GameModuleId::Editor));
}

TEST_CASE("GameModuleRegistry: re-register replaces", "[UI][GameModule]")
{
    GameModuleRegistry::Clear();

    GameModuleRegistry::Register({GameModuleId::Missions, "Old", 117, nullptr});
    GameModuleRegistry::Register({GameModuleId::Missions, "New", 117, nullptr});

    REQUIRE(GameModuleRegistry::All().size() == 1);
    auto* mod = GameModuleRegistry::Find(GameModuleId::Missions);
    REQUIRE(std::string(mod->name) == "New");
}

TEST_CASE("GameModuleRegistry: Clear removes all", "[UI][GameModule]")
{
    GameModuleRegistry::Clear();
    GameModuleRegistry::Register({GameModuleId::Editor, "Ed", 115, nullptr});
    REQUIRE(GameModuleRegistry::All().size() == 1);

    GameModuleRegistry::Clear();
    REQUIRE(GameModuleRegistry::All().empty());
    REQUIRE_FALSE(GameModuleRegistry::IsRegistered(GameModuleId::Editor));
}
