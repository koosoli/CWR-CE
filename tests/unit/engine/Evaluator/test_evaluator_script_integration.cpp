#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Evaluator/express.hpp>
#include "evaluator_fixture.hpp"

// ============================================================================
// Evaluator testing - script-based integration tests
// ============================================================================
// Real-world integration tests that simulate mission script patterns.
// These tests:
// 1. Set up game state using VarSet() (like script initialization)
// 2. Evaluate expressions using Evaluate() (like script conditionals/calculations)
// 3. Verify results (like checking game state after script execution)
//
// This tests the evaluator as it's actually used in the game:
// - Variables are created/modified via VarSet (or assignment operator)
// - Expressions are evaluated to make decisions or calculate values
// - Results drive game logic
// ============================================================================

// ============================================================================
// Test 1: Health System Calculation
// ============================================================================

TEST_CASE("Script - Health damage calculation", "[evaluator][script][health]")
{
    EvaluatorFixture fixture;

    SECTION("Calculate remaining health after damage")
    {
        // Setup: unit with 100 health takes 30 damage
        GGameState.VarSet("health", GameValue(100.0f), false, false);
        GGameState.VarSet("damage", GameValue(30.0f), false, false);

        // Expression: calculate new health
        GameValue result = GGameState.Evaluate("health - damage");

        // Verify calculation
        REQUIRE((float)result == Catch::Approx(70.0f));

        // Update health (simulates assignment)
        GGameState.VarSet("health", result, false, false);
        REQUIRE((float)GGameState.VarGet("health") == Catch::Approx(70.0f));

        // Cleanup
        GGameState.VarDelete("health");
        GGameState.VarDelete("damage");
    }

    SECTION("Multiple damage events")
    {
        GGameState.VarSet("health", GameValue(100.0f), false, false);

        // First hit
        GGameState.VarSet("damage1", GameValue(20.0f), false, false);
        GameValue after1 = GGameState.Evaluate("health - damage1");
        GGameState.VarSet("health", after1, false, false);

        // Second hit
        GGameState.VarSet("damage2", GameValue(15.0f), false, false);
        GameValue after2 = GGameState.Evaluate("health - damage2");
        GGameState.VarSet("health", after2, false, false);

        // Third hit
        GGameState.VarSet("damage3", GameValue(10.0f), false, false);
        GameValue final = GGameState.Evaluate("health - damage3");

        REQUIRE((float) final == Catch::Approx(55.0f));

        // Cleanup
        GGameState.VarDelete("health");
        GGameState.VarDelete("damage1");
        GGameState.VarDelete("damage2");
        GGameState.VarDelete("damage3");
    }
}

// ============================================================================
// Test 2: Distance Calculation
// ============================================================================

TEST_CASE("Script - Distance calculation", "[evaluator][script][distance]")
{
    EvaluatorFixture fixture;

    SECTION("Calculate distance squared between two points")
    {
        // Setup: player at (100, 100), enemy at (110, 105)
        GGameState.VarSet("playerX", GameValue(100.0f), false, false);
        GGameState.VarSet("playerY", GameValue(100.0f), false, false);
        GGameState.VarSet("enemyX", GameValue(110.0f), false, false);
        GGameState.VarSet("enemyY", GameValue(105.0f), false, false);

        // Calculate delta
        GameValue dx = GGameState.Evaluate("enemyX - playerX");
        GameValue dy = GGameState.Evaluate("enemyY - playerY");

        GGameState.VarSet("dx", dx, false, false);
        GGameState.VarSet("dy", dy, false, false);

        // Calculate distance squared
        GameValue distSq = GGameState.Evaluate("(dx * dx) + (dy * dy)");

        // 10^2 + 5^2 = 100 + 25 = 125
        REQUIRE((float)distSq == Catch::Approx(125.0f));

        // Cleanup
        GGameState.VarDelete("playerX");
        GGameState.VarDelete("playerY");
        GGameState.VarDelete("enemyX");
        GGameState.VarDelete("enemyY");
        GGameState.VarDelete("dx");
        GGameState.VarDelete("dy");
    }
}

// ============================================================================
// Test 3: Combat System
// ============================================================================

TEST_CASE("Script - Combat calculations", "[evaluator][script][combat]")
{
    EvaluatorFixture fixture;

    SECTION("Unit engages enemy")
    {
        // Setup
        GGameState.VarSet("unitHealth", GameValue(100.0f), false, false);
        GGameState.VarSet("unitAmmo", GameValue(30.0f), false, false);
        GGameState.VarSet("enemyHealth", GameValue(80.0f), false, false);

        // Unit fires (3 rounds)
        GameValue ammoAfterFiring = GGameState.Evaluate("unitAmmo - 3");
        GGameState.VarSet("unitAmmo", ammoAfterFiring, false, false);

        // Enemy takes damage
        GameValue enemyAfterHit = GGameState.Evaluate("enemyHealth - 20");
        GGameState.VarSet("enemyHealth", enemyAfterHit, false, false);

        // Unit takes return fire
        GameValue unitAfterDamage = GGameState.Evaluate("unitHealth - 25");
        GGameState.VarSet("unitHealth", unitAfterDamage, false, false);

        // Verify final state
        REQUIRE((float)GGameState.VarGet("unitHealth") == Catch::Approx(75.0f));
        REQUIRE((float)GGameState.VarGet("unitAmmo") == Catch::Approx(27.0f));
        REQUIRE((float)GGameState.VarGet("enemyHealth") == Catch::Approx(60.0f));

        // Cleanup
        GGameState.VarDelete("unitHealth");
        GGameState.VarDelete("unitAmmo");
        GGameState.VarDelete("enemyHealth");
    }
}

// ============================================================================
// Test 4: Physics Calculations
// ============================================================================

TEST_CASE("Script - Physics simulation", "[evaluator][script][physics]")
{
    EvaluatorFixture fixture;

    SECTION("Calculate acceleration and velocity")
    {
        // F = ma, so a = F/m
        GGameState.VarSet("force", GameValue(5000.0f), false, false);
        GGameState.VarSet("mass", GameValue(1000.0f), false, false);

        GameValue accel = GGameState.Evaluate("force / mass");
        REQUIRE((float)accel == Catch::Approx(5.0f));

        // dv = a * dt
        GGameState.VarSet("accel", accel, false, false);
        GGameState.VarSet("deltaTime", GameValue(0.1f), false, false);

        GameValue velocityChange = GGameState.Evaluate("accel * deltaTime");
        REQUIRE((float)velocityChange == Catch::Approx(0.5f));

        // Cleanup
        GGameState.VarDelete("force");
        GGameState.VarDelete("mass");
        GGameState.VarDelete("accel");
        GGameState.VarDelete("deltaTime");
    }
}

// ============================================================================
// Test 5: Mission Objectives
// ============================================================================

TEST_CASE("Script - Mission objective progress", "[evaluator][script][mission]")
{
    EvaluatorFixture fixture;

    SECTION("Track objective completion")
    {
        GGameState.VarSet("objComplete", GameValue(0.0f), false, false);
        GGameState.VarSet("objTotal", GameValue(3.0f), false, false);

        // Complete objective 1
        GameValue after1 = GGameState.Evaluate("objComplete + 1");
        GGameState.VarSet("objComplete", after1, false, false);

        // Complete objective 2
        GameValue after2 = GGameState.Evaluate("objComplete + 1");
        GGameState.VarSet("objComplete", after2, false, false);

        // Complete objective 3
        GameValue after3 = GGameState.Evaluate("objComplete + 1");
        GGameState.VarSet("objComplete", after3, false, false);

        // Check mission success (all objectives complete)
        GameValue progress = GGameState.Evaluate("objComplete / objTotal");
        REQUIRE((float)progress == Catch::Approx(1.0f));

        // Cleanup
        GGameState.VarDelete("objComplete");
        GGameState.VarDelete("objTotal");
    }
}

// ============================================================================
// Test 6: Resource Management
// ============================================================================

TEST_CASE("Script - Resource tracking", "[evaluator][script][resources]")
{
    EvaluatorFixture fixture;

    SECTION("Ammunition management")
    {
        GGameState.VarSet("ammo", GameValue(30.0f), false, false);
        GGameState.VarSet("magazines", GameValue(5.0f), false, false);

        // Fire 10 rounds
        GameValue afterFiring = GGameState.Evaluate("ammo - 10");
        GGameState.VarSet("ammo", afterFiring, false, false);
        REQUIRE((float)GGameState.VarGet("ammo") == Catch::Approx(20.0f));

        // Fire 20 more (need reload)
        afterFiring = GGameState.Evaluate("ammo - 20");
        GGameState.VarSet("ammo", afterFiring, false, false);
        REQUIRE((float)GGameState.VarGet("ammo") == Catch::Approx(0.0f));

        // Reload
        GameValue afterReload = GGameState.Evaluate("magazines - 1");
        GGameState.VarSet("magazines", afterReload, false, false);
        GGameState.VarSet("ammo", GameValue(30.0f), false, false);

        REQUIRE((float)GGameState.VarGet("magazines") == Catch::Approx(4.0f));
        REQUIRE((float)GGameState.VarGet("ammo") == Catch::Approx(30.0f));

        // Cleanup
        GGameState.VarDelete("ammo");
        GGameState.VarDelete("magazines");
    }
}

// ============================================================================
// Test 7: Conditional Logic Patterns
// ============================================================================

TEST_CASE("Script - Conditional evaluation", "[evaluator][script][conditions]")
{
    EvaluatorFixture fixture;

    SECTION("Health threshold check")
    {
        GGameState.VarSet("health", GameValue(45.0f), false, false);
        GGameState.VarSet("threshold", GameValue(50.0f), false, false);

        // Check if below threshold
        GameValue needsHealing = GGameState.Evaluate("health < threshold");
        REQUIRE((bool)needsHealing == true);

        // Apply healing
        GGameState.VarSet("healAmount", GameValue(20.0f), false, false);
        GameValue healed = GGameState.Evaluate("health + healAmount");
        GGameState.VarSet("health", healed, false, false);

        // Check again
        GameValue stillNeedsHealing = GGameState.Evaluate("health < threshold");
        REQUIRE((bool)stillNeedsHealing == false);

        // Cleanup
        GGameState.VarDelete("health");
        GGameState.VarDelete("threshold");
        GGameState.VarDelete("healAmount");
    }
}

// ============================================================================
// Test 8: Complex Calculations
// ============================================================================

TEST_CASE("Script - Multi-step calculations", "[evaluator][script][complex]")
{
    EvaluatorFixture fixture;

    SECTION("Accuracy calculation")
    {
        GGameState.VarSet("shotsHit", GameValue(15.0f), false, false);
        GGameState.VarSet("shotsFired", GameValue(20.0f), false, false);

        // Calculate accuracy percentage
        GameValue accuracy = GGameState.Evaluate("(shotsHit / shotsFired) * 100");

        REQUIRE((float)accuracy == Catch::Approx(75.0f));

        // Cleanup
        GGameState.VarDelete("shotsHit");
        GGameState.VarDelete("shotsFired");
    }
}

// ============================================================================
// Test 9: State Machine Values
// ============================================================================

TEST_CASE("Script - State tracking", "[evaluator][script][state]")
{
    EvaluatorFixture fixture;

    SECTION("Unit state values")
    {
        // States: 0=idle, 1=moving, 2=combat, 3=dead
        GGameState.VarSet("state", GameValue(0.0f), false, false);
        GGameState.VarSet("health", GameValue(100.0f), false, false);

        // Transition to moving
        GGameState.VarSet("state", GameValue(1.0f), false, false);
        REQUIRE((float)GGameState.VarGet("state") == Catch::Approx(1.0f));

        // Transition to combat
        GGameState.VarSet("state", GameValue(2.0f), false, false);
        REQUIRE((float)GGameState.VarGet("state") == Catch::Approx(2.0f));

        // Take fatal damage
        GGameState.VarSet("health", GameValue(0.0f), false, false);
        GameValue isDead = GGameState.Evaluate("health <= 0");
        REQUIRE((bool)isDead == true);

        // Cleanup
        GGameState.VarDelete("state");
        GGameState.VarDelete("health");
    }
}

// ============================================================================
// Test 10: Real Mission Pattern
// ============================================================================

TEST_CASE("Script - Complete mission scenario", "[evaluator][script][realistic]")
{
    EvaluatorFixture fixture;

    SECTION("Tactical situation simulation")
    {
        // Initialize scenario
        GGameState.VarSet("playerHealth", GameValue(100.0f), false, false);
        GGameState.VarSet("playerAmmo", GameValue(90.0f), false, false);
        GGameState.VarSet("enemiesKilled", GameValue(0.0f), false, false);
        GGameState.VarSet("shotsFired", GameValue(0.0f), false, false);

        // Combat sequence
        for (int i = 0; i < 2; i++)
        {
            // Fire burst
            GameValue ammo = GGameState.Evaluate("playerAmmo - 10");
            GameValue shots = GGameState.Evaluate("shotsFired + 10");
            GGameState.VarSet("playerAmmo", ammo, false, false);
            GGameState.VarSet("shotsFired", shots, false, false);

            // Enemy eliminated
            GameValue kills = GGameState.Evaluate("enemiesKilled + 1");
            GGameState.VarSet("enemiesKilled", kills, false, false);

            // Take damage
            GameValue health = GGameState.Evaluate("playerHealth - 15");
            GGameState.VarSet("playerHealth", health, false, false);
        }

        // Calculate accuracy
        GameValue accuracy = GGameState.Evaluate("(enemiesKilled * 100) / (shotsFired / 10)");

        // Verify final state
        REQUIRE((float)GGameState.VarGet("playerHealth") == Catch::Approx(70.0f));
        REQUIRE((float)GGameState.VarGet("playerAmmo") == Catch::Approx(70.0f));
        REQUIRE((float)GGameState.VarGet("enemiesKilled") == Catch::Approx(2.0f));
        REQUIRE((float)accuracy == Catch::Approx(100.0f)); // 2 enemies / 2 bursts

        // Cleanup
        GGameState.VarDelete("playerHealth");
        GGameState.VarDelete("playerAmmo");
        GGameState.VarDelete("enemiesKilled");
        GGameState.VarDelete("shotsFired");
    }
}
