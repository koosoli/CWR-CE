#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Core/ProgressSystem.hpp>

using Poseidon::ShouldArmStartupSplash;

// The "Bohemia Interactive presents" startup splash must play once, on the genuine
// first boot. A mod re-mount (MODS -> Apply) re-runs the content-init path; before the fix it
// re-armed the splash, which both rendered over the freshly rebuilt main menu AND ran its script
// (cwr_startup.sqs) again — and that script does `disableUserInput true` for the ~9s of titleRsc
// animation, which over the menu froze the mouse + keyboard (InputProcessingSdl gates on
// IsUserInputEnabled). The harness never caught it because tri always launches with --no-splash.
// firstBoot=false must never arm the splash, whatever the other flags.
TEST_CASE("startup splash arms only on first boot, never on re-mount", "[core][splash]")
{
    // Genuine first boot, nothing special -> arm.
    REQUIRE(ShouldArmStartupSplash(/*firstBoot*/ true, /*noSplash*/ false, /*landEditor*/ false,
                                   /*dedicatedServer*/ false, /*clientConnected*/ false));

    // Re-mount: firstBoot=false -> never arm, regardless of the other flags.
    REQUIRE_FALSE(ShouldArmStartupSplash(false, false, false, false, false));
    REQUIRE_FALSE(ShouldArmStartupSplash(false, false, false, false, true));

    // First boot but suppressed by an explicit flag / mode.
    REQUIRE_FALSE(ShouldArmStartupSplash(true, true, false, false, false)); // -nosplash
    REQUIRE_FALSE(ShouldArmStartupSplash(true, false, true, false, false)); // land editor
    REQUIRE_FALSE(ShouldArmStartupSplash(true, false, false, true, false)); // dedicated server
    REQUIRE_FALSE(ShouldArmStartupSplash(true, false, false, false, true)); // joining a server
}
