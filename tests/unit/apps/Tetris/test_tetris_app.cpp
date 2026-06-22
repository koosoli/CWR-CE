// Phase 0 smoke tests for TetrisApplication.  Holds the contract that
// the app can be constructed without booting a window — important
// because later subsystem-extraction phases need TetrisApplication to be
// instantiable in test harnesses that don't have a display.

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "TetrisApplication.hpp"
#include "TetrisNotebookUI.hpp"
#include <Poseidon/Foundation/Time/Time.hpp>

TEST_CASE("TetrisApplication is default-constructible", "[tetris][app]")
{
    // Constructor must not pull in any subsystem state — Run() does
    // the heavy lifting.  Two back-to-back instances also must not
    // collide on any global init (singletons should be lazy).
    {
        TetrisApplication app;
        (void)app;
    }
    {
        TetrisApplication second;
        (void)second;
    }
}

TEST_CASE("TetrisApplication registers no game modules", "[tetris][app]")
{
    // RegisterGameModules() is a pure act of registration — it should
    // be a no-op for Tetris and never fail.  Calling it twice (as a
    // defensive double-init) must also be safe.
    TetrisApplication app;
    app.RegisterGameModules();
    app.RegisterGameModules();
}

#include <Poseidon/Audio/IAudioSystem.hpp>
#include <Poseidon/Audio/Dummy/SoundSystemDummy.hpp>

namespace Poseidon
{
extern World* GWorld;
extern Landscape* GLandscape;
} // namespace Poseidon
using Poseidon::GLandscape;
using Poseidon::GWorld;

using namespace Poseidon;
using namespace Poseidon::Foundation;

extern void CleanupSoundSystem();

TEST_CASE("TetrisApplication::InitializeSound installs a dummy backend", "[tetris][app]")
{
    // Tetris must not open an OpenAL device; consumers that touch
    // GSoundsys should see a no-op dummy backend.
    CleanupSoundSystem();
    REQUIRE(GSoundsys == nullptr);

    TetrisApplication app;
    REQUIRE(app.InitializeSound());

    REQUIRE(GSoundsys != nullptr);
    CHECK(dynamic_cast<SoundSystemDummy*>(GSoundsys) != nullptr);

    CleanupSoundSystem();
}

TEST_CASE("TetrisApplication::InitializeWorld is a no-op", "[tetris][app]")
{
    // Tetris skips CreateWorld/CreateLandscape/VehicleTypes_Preload —
    // none of those tolerate a missing CONFIG.BIN, and Tetris ships no
    // CfgWorlds entries.  The override must succeed without touching
    // GWorld or GLandscape.
    World* worldBefore = GWorld;
    Landscape* landscapeBefore = GLandscape;

    TetrisApplication app;
    REQUIRE(app.InitializeWorld());

    CHECK(GWorld == worldBefore);
    CHECK(GLandscape == landscapeBefore);
}

TEST_CASE("Tetris UI time advances animation time", "[tetris][app]")
{
    const UITime start(0);
    const UITime advanced = AdvanceTetrisNotebookUiTime(start, 0.25f);

    CHECK((advanced - start) == Catch::Approx(0.25f).margin(0.001f));
}
