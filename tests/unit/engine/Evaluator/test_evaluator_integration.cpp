#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Evaluator/express.hpp>
#include "evaluator_fixture.hpp"
#include <stdio.h>

// ============================================================================
// Evaluator testing - integration tests
// ============================================================================
// Real-world SQF scripting scenarios that test the complete evaluator flow.
// These tests simulate actual mission scripts with multiple statements,
// variables, conditions, and computations.
//
// Purpose:
// - Verify end-to-end evaluator functionality
// - Test realistic script patterns used in OFP missions
// - Validate complex interactions between components
// - Ensure evaluator behaves correctly in production scenarios
//
// Scenarios Covered:
// 1. Variable initialization and computation
// 2. Conditional logic (if/then patterns)
// 3. Loops and iteration (while patterns)
// 4. State machines and counters
// 5. Array manipulation
// 6. Multi-step calculations
// 7. Real mission script patterns
//
// Note: These are integration tests - they test multiple components together
// ============================================================================

// ============================================================================
// Test 1: Basic Variable Workflow
// ============================================================================

TEST_CASE("Integration - Basic variable workflow", "[evaluator][integration][workflow]")
{
    EvaluatorFixture fixture;

    SECTION("Define, compute, modify, verify")
    {
        // Define initial variables
        GGameState.VarSet("health", GameValue(100.0f), false, false);
        GGameState.VarSet("damage", GameValue(25.0f), false, false);

        // Compute new value based on variables
        GameValue remainingHealth = GGameState.Evaluate("health - damage");
        GGameState.VarSet("currentHealth", remainingHealth, false, false);

        // Verify computation
        REQUIRE((float)GGameState.VarGet("currentHealth") == Catch::Approx(75.0f));

        // Apply additional damage
        GGameState.VarSet("damage", GameValue(15.0f), false, false);
        GameValue newHealth = GGameState.Evaluate("currentHealth - damage");
        GGameState.VarSet("currentHealth", newHealth, false, false);

        // Verify final state
        REQUIRE((float)GGameState.VarGet("currentHealth") == Catch::Approx(60.0f));
        REQUIRE((float)GGameState.VarGet("damage") == Catch::Approx(15.0f));
        REQUIRE((float)GGameState.VarGet("health") == Catch::Approx(100.0f)); // Original unchanged

        // Cleanup
        GGameState.VarDelete("health");
        GGameState.VarDelete("damage");
        GGameState.VarDelete("currentHealth");
    }
}

// ============================================================================
// Test 2: Conditional Logic Simulation
// ============================================================================

TEST_CASE("Integration - Conditional logic", "[evaluator][integration][conditions]")
{
    EvaluatorFixture fixture;

    SECTION("Health threshold check")
    {
        // Setup scenario: unit takes damage
        GGameState.VarSet("unitHealth", GameValue(100.0f), false, false);
        GGameState.VarSet("damageTaken", GameValue(30.0f), false, false);
        GGameState.VarSet("healAmount", GameValue(0.0f), false, false);

        // Apply damage
        GameValue newHealth = GGameState.Evaluate("unitHealth - damageTaken");
        GGameState.VarSet("unitHealth", newHealth, false, false);

        // Check if health is below threshold (simulate: if (unitHealth < 50) then { heal })
        float currentHealth = (float)GGameState.VarGet("unitHealth");
        bool needsHealing = currentHealth < 50.0f;

        if (!needsHealing)
        {
            // Unit is still above threshold, no healing needed
            REQUIRE(currentHealth == Catch::Approx(70.0f));
            REQUIRE((float)GGameState.VarGet("healAmount") == Catch::Approx(0.0f));
        }

        // Apply more damage to trigger healing threshold
        GGameState.VarSet("damageTaken", GameValue(25.0f), false, false);
        newHealth = GGameState.Evaluate("unitHealth - damageTaken");
        GGameState.VarSet("unitHealth", newHealth, false, false);

        currentHealth = (float)GGameState.VarGet("unitHealth");
        needsHealing = currentHealth < 50.0f;

        if (needsHealing)
        {
            // Trigger healing
            GGameState.VarSet("healAmount", GameValue(20.0f), false, false);
            GameValue healedHealth = GGameState.Evaluate("unitHealth + healAmount");
            GGameState.VarSet("unitHealth", healedHealth, false, false);

            REQUIRE((float)GGameState.VarGet("unitHealth") == Catch::Approx(65.0f));
        }

        // Cleanup
        GGameState.VarDelete("unitHealth");
        GGameState.VarDelete("damageTaken");
        GGameState.VarDelete("healAmount");
    }
}

// ============================================================================
// Test 3: Counter and Loop Simulation
// ============================================================================

TEST_CASE("Integration - Counter simulation", "[evaluator][integration][loops]")
{
    EvaluatorFixture fixture;

    SECTION("Counting loop simulation")
    {
        // Simulate: counter = 0; while { counter < 10 } do { counter = counter + 1 }
        GGameState.VarSet("counter", GameValue(0.0f), false, false);
        GGameState.VarSet("maxCount", GameValue(10.0f), false, false);

        // Manually simulate loop (evaluator may not support full while syntax)
        for (int i = 0; i < 10; i++)
        {
            float current = (float)GGameState.VarGet("counter");
            float max = (float)GGameState.VarGet("maxCount");

            if (current < max)
            {
                GameValue incremented = GGameState.Evaluate("counter + 1");
                GGameState.VarSet("counter", incremented, false, false);
            }
        }

        // Verify final count
        REQUIRE((float)GGameState.VarGet("counter") == Catch::Approx(10.0f));

        // Cleanup
        GGameState.VarDelete("counter");
        GGameState.VarDelete("maxCount");
    }
}

// ============================================================================
// Test 4: State Machine Simulation
// ============================================================================

TEST_CASE("Integration - State machine", "[evaluator][integration][state]")
{
    EvaluatorFixture fixture;

    SECTION("Unit state transitions")
    {
        // States: 0=idle, 1=moving, 2=attacking, 3=dead
        GGameState.VarSet("state", GameValue(0.0f), false, false); // Idle
        GGameState.VarSet("health", GameValue(100.0f), false, false);
        GGameState.VarSet("enemyNear", GameValue(1.0f), false, false); // 1=true, 0=false

        // Idle -> Moving (if enemy near)
        if ((float)GGameState.VarGet("enemyNear") > 0.5f)
        {
            GGameState.VarSet("state", GameValue(1.0f), false, false);
        }

        REQUIRE((float)GGameState.VarGet("state") == Catch::Approx(1.0f)); // Moving

        // Moving -> Attacking (when in range)
        GGameState.VarSet("distanceToEnemy", GameValue(10.0f), false, false);
        float distance = (float)GGameState.VarGet("distanceToEnemy");

        if (distance < 50.0f)
        {
            GGameState.VarSet("state", GameValue(2.0f), false, false); // Attacking
        }

        REQUIRE((float)GGameState.VarGet("state") == Catch::Approx(2.0f)); // Attacking

        // Take fatal damage
        GGameState.VarSet("damage", GameValue(120.0f), false, false);
        GameValue newHealth = GGameState.Evaluate("health - damage");
        GGameState.VarSet("health", newHealth, false, false);

        if ((float)GGameState.VarGet("health") <= 0.0f)
        {
            GGameState.VarSet("state", GameValue(3.0f), false, false); // Dead
        }

        REQUIRE((float)GGameState.VarGet("state") == Catch::Approx(3.0f)); // Dead

        // Cleanup
        GGameState.VarDelete("state");
        GGameState.VarDelete("health");
        GGameState.VarDelete("enemyNear");
        GGameState.VarDelete("distanceToEnemy");
        GGameState.VarDelete("damage");
    }
}

// ============================================================================
// Test 5: Array Manipulation
// ============================================================================

TEST_CASE("Integration - Array manipulation", "[evaluator][integration][arrays]")
{
    EvaluatorFixture fixture;

    SECTION("Unit group management")
    {
        // Create array of unit IDs
        GameArrayType unitGroup;
        unitGroup.Add(GameValue(101.0f));
        unitGroup.Add(GameValue(102.0f));
        unitGroup.Add(GameValue(103.0f));

        GGameState.VarSet("group", GameValue(unitGroup), false, false);

        // Get group size
        GameValue groupVar = GGameState.VarGet("group");
        const GameArrayType& group = (const GameArrayType&)groupVar;
        int groupSize = group.Size();

        REQUIRE(groupSize == 3);

        // Store individual unit data
        for (int i = 0; i < groupSize; i++)
        {
            float unitId = (float)group[i];

            // Create variables for each unit: unit101_health, unit102_health, etc.
            char varName[32];
            sprintf(varName, "unit%d_health", (int)unitId);
            GGameState.VarSet(varName, GameValue(100.0f), false, false);
        }

        // Damage first unit
        GGameState.VarSet("unit101_health", GameValue(75.0f), false, false);

        // Verify states
        REQUIRE((float)GGameState.VarGet("unit101_health") == Catch::Approx(75.0f));
        REQUIRE((float)GGameState.VarGet("unit102_health") == Catch::Approx(100.0f));
        REQUIRE((float)GGameState.VarGet("unit103_health") == Catch::Approx(100.0f));

        // Calculate total group health
        float totalHealth = 0.0f;
        for (int i = 0; i < groupSize; i++)
        {
            float unitId = (float)group[i];
            char varName[32];
            sprintf(varName, "unit%d_health", (int)unitId);
            totalHealth += (float)GGameState.VarGet(varName);
        }

        REQUIRE(totalHealth == Catch::Approx(275.0f));

        // Cleanup
        GGameState.VarDelete("group");
        GGameState.VarDelete("unit101_health");
        GGameState.VarDelete("unit102_health");
        GGameState.VarDelete("unit103_health");
    }
}

// ============================================================================
// Test 6: Multi-Step Calculation
// ============================================================================

TEST_CASE("Integration - Multi-step calculation", "[evaluator][integration][calculation]")
{
    EvaluatorFixture fixture;

    SECTION("Ballistic trajectory calculation")
    {
        // Initial conditions
        GGameState.VarSet("velocity", GameValue(100.0f), false, false); // m/s
        GGameState.VarSet("angle", GameValue(45.0f), false, false);     // degrees
        GGameState.VarSet("gravity", GameValue(9.8f), false, false);    // m/s?

        // Calculate horizontal and vertical velocity components
        // Note: Real calculation would use sin/cos, we'll simulate with simplified math
        GameValue horizVel = GGameState.Evaluate("velocity * 0.707"); // cos(45�) ? 0.707
        GameValue vertVel = GGameState.Evaluate("velocity * 0.707");  // sin(45�) ? 0.707

        GGameState.VarSet("vx", horizVel, false, false);
        GGameState.VarSet("vy", vertVel, false, false);

        // Calculate time of flight (simplified)
        GameValue timeOfFlight = GGameState.Evaluate("(vy * 2) / gravity");
        GGameState.VarSet("timeToImpact", timeOfFlight, false, false);

        // Calculate range
        GameValue range = GGameState.Evaluate("vx * timeToImpact");
        GGameState.VarSet("maxRange", range, false, false);

        // Verify results
        REQUIRE((float)GGameState.VarGet("vx") == Catch::Approx(70.7f).margin(0.1f));
        REQUIRE((float)GGameState.VarGet("vy") == Catch::Approx(70.7f).margin(0.1f));
        REQUIRE((float)GGameState.VarGet("timeToImpact") > 14.0f);
        REQUIRE((float)GGameState.VarGet("maxRange") > 1000.0f);

        // Cleanup
        GGameState.VarDelete("velocity");
        GGameState.VarDelete("angle");
        GGameState.VarDelete("gravity");
        GGameState.VarDelete("vx");
        GGameState.VarDelete("vy");
        GGameState.VarDelete("timeToImpact");
        GGameState.VarDelete("maxRange");
    }
}

// ============================================================================
// Test 7: Mission Objective Tracking
// ============================================================================

TEST_CASE("Integration - Mission objective tracking", "[evaluator][integration][mission]")
{
    EvaluatorFixture fixture;

    SECTION("Complete mission scenario")
    {
        // Mission state
        GGameState.VarSet("objectivesCompleted", GameValue(0.0f), false, false);
        GGameState.VarSet("totalObjectives", GameValue(3.0f), false, false);
        GGameState.VarSet("missionSuccess", GameValue(0.0f), false, false);

        // Objective 1: Reach checkpoint
        GGameState.VarSet("playerX", GameValue(100.0f), false, false);
        GGameState.VarSet("playerY", GameValue(150.0f), false, false);
        GGameState.VarSet("checkpointX", GameValue(120.0f), false, false);
        GGameState.VarSet("checkpointY", GameValue(160.0f), false, false);

        // Calculate distance (simplified)
        GameValue dx = GGameState.Evaluate("checkpointX - playerX");
        GameValue dy = GGameState.Evaluate("checkpointY - playerY");
        GameValue distSq = GGameState.Evaluate("(20 * 20) + (10 * 10)"); // Using known values
        GGameState.VarSet("distanceSquared", distSq, false, false);

        float dist = (float)GGameState.VarGet("distanceSquared");
        if (dist < 625.0f) // Within 25m
        {
            GameValue completed = GGameState.Evaluate("objectivesCompleted + 1");
            GGameState.VarSet("objectivesCompleted", completed, false, false);
        }

        REQUIRE((float)GGameState.VarGet("objectivesCompleted") == Catch::Approx(1.0f));

        // Objective 2: Eliminate target
        GGameState.VarSet("targetAlive", GameValue(1.0f), false, false);

        // Player attacks target
        GGameState.VarSet("targetAlive", GameValue(0.0f), false, false);

        if ((float)GGameState.VarGet("targetAlive") < 0.5f)
        {
            GameValue completed = GGameState.Evaluate("objectivesCompleted + 1");
            GGameState.VarSet("objectivesCompleted", completed, false, false);
        }

        REQUIRE((float)GGameState.VarGet("objectivesCompleted") == Catch::Approx(2.0f));

        // Objective 3: Return to base
        GGameState.VarSet("playerX", GameValue(10.0f), false, false);
        GGameState.VarSet("playerY", GameValue(20.0f), false, false);
        GGameState.VarSet("baseX", GameValue(15.0f), false, false);
        GGameState.VarSet("baseY", GameValue(25.0f), false, false);

        // Check if near base
        GameValue baseDistSq = GGameState.Evaluate("(5 * 5) + (5 * 5)");
        float baseDist = (float)baseDistSq;

        if (baseDist < 100.0f) // Within 10m
        {
            GameValue completed = GGameState.Evaluate("objectivesCompleted + 1");
            GGameState.VarSet("objectivesCompleted", completed, false, false);
        }

        REQUIRE((float)GGameState.VarGet("objectivesCompleted") == Catch::Approx(3.0f));

        // Check mission completion
        float objCompleted = (float)GGameState.VarGet("objectivesCompleted");
        float objTotal = (float)GGameState.VarGet("totalObjectives");

        if (objCompleted >= objTotal)
        {
            GGameState.VarSet("missionSuccess", GameValue(1.0f), false, false);
        }

        REQUIRE((float)GGameState.VarGet("missionSuccess") == Catch::Approx(1.0f));

        // Cleanup
        GGameState.VarDelete("objectivesCompleted");
        GGameState.VarDelete("totalObjectives");
        GGameState.VarDelete("missionSuccess");
        GGameState.VarDelete("playerX");
        GGameState.VarDelete("playerY");
        GGameState.VarDelete("checkpointX");
        GGameState.VarDelete("checkpointY");
        GGameState.VarDelete("distanceSquared");
        GGameState.VarDelete("targetAlive");
        GGameState.VarDelete("baseX");
        GGameState.VarDelete("baseY");
    }
}

// ============================================================================
// Test 8: Resource Management
// ============================================================================

TEST_CASE("Integration - Resource management", "[evaluator][integration][resources]")
{
    EvaluatorFixture fixture;

    SECTION("Ammunition and supplies")
    {
        // Initialize resources
        GGameState.VarSet("ammoCount", GameValue(30.0f), false, false);
        GGameState.VarSet("magazineSize", GameValue(30.0f), false, false);
        GGameState.VarSet("totalMagazines", GameValue(5.0f), false, false);
        GGameState.VarSet("currentMag", GameValue(1.0f), false, false);

        // Fire 15 rounds
        GameValue ammoAfterFiring = GGameState.Evaluate("ammoCount - 15");
        GGameState.VarSet("ammoCount", ammoAfterFiring, false, false);

        REQUIRE((float)GGameState.VarGet("ammoCount") == Catch::Approx(15.0f));

        // Fire 20 more rounds (need to reload)
        float currentAmmo = (float)GGameState.VarGet("ammoCount");
        int roundsToFire = 20;
        int roundsFired = 0;

        while (roundsToFire > 0 && (float)GGameState.VarGet("totalMagazines") > 0.0f)
        {
            currentAmmo = (float)GGameState.VarGet("ammoCount");

            if (currentAmmo <= 0.0f)
            {
                // Reload
                float magsLeft = (float)GGameState.VarGet("totalMagazines");
                if (magsLeft > 0.0f)
                {
                    GGameState.VarSet("totalMagazines", GameValue(magsLeft - 1.0f), false, false);
                    GGameState.VarSet("ammoCount", GGameState.VarGet("magazineSize"), false, false);
                    GameValue newMag = GGameState.Evaluate("currentMag + 1");
                    GGameState.VarSet("currentMag", newMag, false, false);
                }
                else
                {
                    break; // Out of ammo
                }
            }

            currentAmmo = (float)GGameState.VarGet("ammoCount");
            int canFire = (int)currentAmmo < roundsToFire ? (int)currentAmmo : roundsToFire;

            GGameState.VarSet("ammoCount", GameValue(currentAmmo - canFire), false, false);
            roundsFired += canFire;
            roundsToFire -= canFire;
        }

        // Verify final state
        REQUIRE(roundsFired == 20);
        REQUIRE((float)GGameState.VarGet("ammoCount") ==
                Catch::Approx(25.0f)); // 15 from first mag + 30 from second - 20 = 25
        REQUIRE((float)GGameState.VarGet("totalMagazines") == Catch::Approx(4.0f)); // Used 1 mag
        REQUIRE((float)GGameState.VarGet("currentMag") == Catch::Approx(2.0f));

        // Cleanup
        GGameState.VarDelete("ammoCount");
        GGameState.VarDelete("magazineSize");
        GGameState.VarDelete("totalMagazines");
        GGameState.VarDelete("currentMag");
    }
}

// ============================================================================
// Test 9: Complex Expression Chain
// ============================================================================

TEST_CASE("Integration - Complex expression chain", "[evaluator][integration][expressions]")
{
    EvaluatorFixture fixture;

    SECTION("Chained calculations with multiple variables")
    {
        // Physics simulation setup
        GGameState.VarSet("mass", GameValue(1000.0f), false, false);   // kg
        GGameState.VarSet("force", GameValue(5000.0f), false, false);  // N
        GGameState.VarSet("deltaTime", GameValue(0.1f), false, false); // seconds

        // Calculate acceleration: F = ma, so a = F/m
        GameValue acceleration = GGameState.Evaluate("force / mass");
        GGameState.VarSet("accel", acceleration, false, false);

        REQUIRE((float)GGameState.VarGet("accel") == Catch::Approx(5.0f));

        // Calculate velocity change: dv = a * dt
        GameValue velocityChange = GGameState.Evaluate("accel * deltaTime");
        GGameState.VarSet("velocityChange", velocityChange, false, false);

        REQUIRE((float)GGameState.VarGet("velocityChange") == Catch::Approx(0.5f));

        // Initial velocity
        GGameState.VarSet("velocity", GameValue(0.0f), false, false);

        // Update velocity over 10 time steps
        for (int i = 0; i < 10; i++)
        {
            GameValue newVelocity = GGameState.Evaluate("velocity + velocityChange");
            GGameState.VarSet("velocity", newVelocity, false, false);
        }

        // Verify final velocity
        REQUIRE((float)GGameState.VarGet("velocity") == Catch::Approx(5.0f));

        // Calculate distance traveled (simplified): d = v * t
        GameValue totalTime = GGameState.Evaluate("deltaTime * 10");
        GGameState.VarSet("totalTime", totalTime, false, false);

        // Average velocity = final velocity / 2 (starting from 0)
        GameValue avgVelocity = GGameState.Evaluate("velocity / 2");
        GameValue distance = GGameState.Evaluate("(velocity / 2) * totalTime");
        GGameState.VarSet("distance", distance, false, false);

        REQUIRE((float)GGameState.VarGet("distance") == Catch::Approx(2.5f));

        // Cleanup
        GGameState.VarDelete("mass");
        GGameState.VarDelete("force");
        GGameState.VarDelete("deltaTime");
        GGameState.VarDelete("accel");
        GGameState.VarDelete("velocityChange");
        GGameState.VarDelete("velocity");
        GGameState.VarDelete("totalTime");
        GGameState.VarDelete("distance");
    }
}

// ============================================================================
// Test 10: Real Mission Script Pattern
// ============================================================================

TEST_CASE("Integration - Real mission script pattern", "[evaluator][integration][realistic]")
{
    EvaluatorFixture fixture;

    SECTION("Complete unit behavior script")
    {
        // This simulates a real SQF pattern:
        // _unit = player;
        // _health = 100;
        // _enemy = nearestEnemy;
        // _distance = _unit distance _enemy;
        // if (_distance < 100) then { _health = _health - 10 };
        // if (_health < 50) then { _unit action ["HEAL"] };

        // Setup
        GGameState.VarSet("unitID", GameValue(1.0f), false, false);
        GGameState.VarSet("health", GameValue(100.0f), false, false);
        GGameState.VarSet("enemyID", GameValue(2.0f), false, false);
        GGameState.VarSet("posX", GameValue(100.0f), false, false);
        GGameState.VarSet("posY", GameValue(100.0f), false, false);
        GGameState.VarSet("enemyX", GameValue(150.0f), false, false);
        GGameState.VarSet("enemyY", GameValue(140.0f), false, false);

        // Calculate distance (simplified)
        GameValue dx = GGameState.Evaluate("enemyX - posX");
        GameValue dy = GGameState.Evaluate("enemyY - posY");
        GGameState.VarSet("dx", dx, false, false);
        GGameState.VarSet("dy", dy, false, false);

        // Distance squared
        GameValue distSq = GGameState.Evaluate("(dx * dx) + (dy * dy)");
        GGameState.VarSet("distanceSquared", distSq, false, false);

        float distSqValue = (float)GGameState.VarGet("distanceSquared");

        // If enemy is close (< 100m, so distSq < 10000)
        if (distSqValue < 10000.0f)
        {
            // Take damage
            GameValue newHealth = GGameState.Evaluate("health - 10");
            GGameState.VarSet("health", newHealth, false, false);

            // Set combat state
            GGameState.VarSet("inCombat", GameValue(1.0f), false, false);
        }

        REQUIRE((float)GGameState.VarGet("health") == Catch::Approx(90.0f));
        REQUIRE((float)GGameState.VarGet("inCombat") == Catch::Approx(1.0f));

        // Take more damage over time
        for (int i = 0; i < 5; i++)
        {
            if ((float)GGameState.VarGet("inCombat") > 0.5f)
            {
                GameValue damaged = GGameState.Evaluate("health - 10");
                GGameState.VarSet("health", damaged, false, false);
            }
        }

        REQUIRE((float)GGameState.VarGet("health") == Catch::Approx(40.0f));

        // Check if healing is needed
        if ((float)GGameState.VarGet("health") < 50.0f)
        {
            GGameState.VarSet("needsHealing", GameValue(1.0f), false, false);

            // Apply healing
            GameValue healed = GGameState.Evaluate("health + 30");
            GGameState.VarSet("health", healed, false, false);
        }

        REQUIRE((float)GGameState.VarGet("needsHealing") == Catch::Approx(1.0f));
        REQUIRE((float)GGameState.VarGet("health") == Catch::Approx(70.0f));

        // Combat ends
        GGameState.VarSet("inCombat", GameValue(0.0f), false, false);

        // Final state verification
        REQUIRE((float)GGameState.VarGet("health") == Catch::Approx(70.0f));
        REQUIRE((float)GGameState.VarGet("inCombat") == Catch::Approx(0.0f));
        REQUIRE((float)GGameState.VarGet("needsHealing") == Catch::Approx(1.0f));

        // Cleanup
        GGameState.VarDelete("unitID");
        GGameState.VarDelete("health");
        GGameState.VarDelete("enemyID");
        GGameState.VarDelete("posX");
        GGameState.VarDelete("posY");
        GGameState.VarDelete("enemyX");
        GGameState.VarDelete("enemyY");
        GGameState.VarDelete("dx");
        GGameState.VarDelete("dy");
        GGameState.VarDelete("distanceSquared");
        GGameState.VarDelete("inCombat");
        GGameState.VarDelete("needsHealing");
    }
}
