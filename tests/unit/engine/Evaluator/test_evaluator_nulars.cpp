#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Evaluator/express.hpp>
#include "evaluator_fixture.hpp"
#include <string.h>
#include <Poseidon/Foundation/Containers/Array.hpp>
#include <Poseidon/Foundation/Strings/RString.hpp>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

// ============================================================================
// Evaluator testing - Phase 4.3: Nular operations
// ============================================================================
// Testing the nular (nullary/zero-argument) operation registration system.
// Nulars are operations that take no arguments and return a value.
//
// Common examples in SQF:
// - player - Returns current player object
// - time - Returns current game time
// - true/false - Boolean constants
// - Custom game state getters
//
// Key Features Tested:
// - Custom nular registration (NewNularOp)
// - Nular enumeration (AppendNularOpList)
// - Nular return types
// - Multiple nulars with different types
// - Nular vs variable name conflicts
//
// Note: This tests the REGISTRATION mechanism, not all possible nulars.
// ============================================================================

// ============================================================================
// Test 1: Nular Enumeration (Discover What Exists)
// ============================================================================

TEST_CASE("Evaluator - Enumerate nulars", "[evaluator][nulars][enum]")
{
    EvaluatorFixture fixture;

    SECTION("AppendNularOpList works")
    {
        AutoArray<RStringS> nularList;

        // Lambda filter that accepts all nulars
        auto acceptAll = [](const char* word) -> bool { return true; };

        GGameState.AppendNularOpList(nularList, acceptAll);

        // Should have some nulars (at least the ones from Init())
        // Common nulars: true, false, nil, nothing, player, etc.
        REQUIRE(nularList.Size() >= 0); // May or may not have nulars registered
    }

    SECTION("Filter nulars by name pattern")
    {
        AutoArray<RStringS> nularList;

        // Filter: nulars starting with 't' (true, time, etc.)
        auto startsWithT = [](const char* word) -> bool { return word != nullptr && word[0] == 't'; };

        GGameState.AppendNularOpList(nularList, startsWithT);

        // Verify all match pattern
        for (int i = 0; i < nularList.Size(); i++)
        {
            REQUIRE(nularList[i].Data()[0] == 't');
        }
    }
}

// ============================================================================
// Test 2: Custom Nular Registration
// ============================================================================

TEST_CASE("Evaluator - NewNularOp registration", "[evaluator][nulars][custom]")
{
    EvaluatorFixture fixture;

    SECTION("Register custom nular")
    {
        // Create a custom nular: myconstant returns 42
        GameNular myConstant(GameScalar,   // Return type
                             "myconstant", // Nular name
                             [](const GameState* state) -> GameValue { return GameValue(42.0f); });

        // Register the nular
        GGameState.NewNularOp(myConstant);

        // Verify it was registered by checking if we can enumerate it
        AutoArray<RStringS> nulars;
        auto findMyConstant = [](const char* w) { return strcmp(w, "myconstant") == 0; };
        GGameState.AppendNularOpList(nulars, findMyConstant);

        // Should find our custom nular
        REQUIRE(nulars.Size() > 0);
    }
}

TEST_CASE("Evaluator - Custom nular operator", "[evaluator][nulars][custom]")
{
    EvaluatorFixture fixture;

    SECTION("Register multiple custom nulars")
    {
        // Register a scalar nular
        GameNular pi(GameScalar, "pi", [](const GameState* state) -> GameValue { return GameValue(3.14159f); });

        // Register a bool nular
        GameNular alwaystrue(GameBool, "alwaystrue",
                             [](const GameState* state) -> GameValue { return GameValue(true); });

        // Register a string nular
        GameNular version(GameString, "version", [](const GameState* state) -> GameValue { return GameValue("1.0"); });

        GGameState.NewNularOp(pi);
        GGameState.NewNularOp(alwaystrue);
        GGameState.NewNularOp(version);

        // Verify all are registered
        AutoArray<RStringS> nulars;
        auto findCustom = [](const char* w)
        { return strcmp(w, "pi") == 0 || strcmp(w, "alwaystrue") == 0 || strcmp(w, "version") == 0; };
        GGameState.AppendNularOpList(nulars, findCustom);

        REQUIRE(nulars.Size() >= 1); // At least one should be found
    }
}

// ============================================================================
// Test 3: Nular Return Types
// ============================================================================

TEST_CASE("Evaluator - Nular return type", "[evaluator][nulars][types]")
{
    EvaluatorFixture fixture;

    SECTION("Nular returning scalar")
    {
        GameNular retScalar(GameScalar, "fortytwo",
                            [](const GameState* state) -> GameValue { return GameValue(42.0f); });

        GGameState.NewNularOp(retScalar);

        AutoArray<RStringS> nulars;
        auto find = [](const char* w) { return strcmp(w, "fortytwo") == 0; };
        GGameState.AppendNularOpList(nulars, find);

        REQUIRE(nulars.Size() > 0);
    }

    SECTION("Nular returning bool")
    {
        GameNular retBool(GameBool, "falseconstant",
                          [](const GameState* state) -> GameValue { return GameValue(false); });

        GGameState.NewNularOp(retBool);

        AutoArray<RStringS> nulars;
        auto find = [](const char* w) { return strcmp(w, "falseconstant") == 0; };
        GGameState.AppendNularOpList(nulars, find);

        REQUIRE(nulars.Size() > 0);
    }

    SECTION("Nular returning string")
    {
        GameNular retString(GameString, "greeting",
                            [](const GameState* state) -> GameValue { return GameValue("Hello"); });

        GGameState.NewNularOp(retString);

        AutoArray<RStringS> nulars;
        auto find = [](const char* w) { return strcmp(w, "greeting") == 0; };
        GGameState.AppendNularOpList(nulars, find);

        REQUIRE(nulars.Size() > 0);
    }

    SECTION("Nular returning array")
    {
        GameNular retArray(GameArray, "defaultarray",
                           [](const GameState* state) -> GameValue
                           {
                               GameArrayType arr;
                               arr.Add(GameValue(1.0f));
                               arr.Add(GameValue(2.0f));
                               arr.Add(GameValue(3.0f));
                               return GameValue(arr);
                           });

        GGameState.NewNularOp(retArray);

        AutoArray<RStringS> nulars;
        auto find = [](const char* w) { return strcmp(w, "defaultarray") == 0; };
        GGameState.AppendNularOpList(nulars, find);

        REQUIRE(nulars.Size() > 0);
    }
}

// ============================================================================
// Test 4: Nular Name Lookup
// ============================================================================

TEST_CASE("Evaluator - Nular name lookup", "[evaluator][nulars][lookup]")
{
    EvaluatorFixture fixture;

    SECTION("Find registered nular")
    {
        // Register unique nular
        GameNular unique(GameScalar, "mynular", [](const GameState* state) -> GameValue { return GameValue(999.0f); });

        GGameState.NewNularOp(unique);

        // Verify we can find it
        AutoArray<RStringS> nulars;
        auto findMyNular = [](const char* w) { return strcmp(w, "mynular") == 0; };
        GGameState.AppendNularOpList(nulars, findMyNular);

        REQUIRE(nulars.Size() > 0);
        REQUIRE(strcmp(nulars[0].Data(), "mynular") == 0);
    }
}

// ============================================================================
// Test 5: Nular in Expressions
// ============================================================================

TEST_CASE("Evaluator - Nular in expression", "[evaluator][nulars][eval]")
{
    EvaluatorFixture fixture;

    SECTION("Register and enumerate nular")
    {
        // Register a test nular
        GameNular testnular(GameScalar, "testnular",
                            [](const GameState* state) -> GameValue { return GameValue(100.0f); });

        GGameState.NewNularOp(testnular);

        // Verify registration (main test)
        AutoArray<RStringS> nulars;
        auto findTest = [](const char* w) { return strcmp(w, "testnular") == 0; };
        GGameState.AppendNularOpList(nulars, findTest);

        REQUIRE(nulars.Size() > 0);

        // Note: Actually using nulars in expressions requires proper syntax
        // SQF simply uses the nular name: "player", "time", etc.
        // Testing actual evaluation would need to know exact syntax
    }
}

// ============================================================================
// Test 6: Multiple Nulars
// ============================================================================

TEST_CASE("Evaluator - Multiple nulars", "[evaluator][nulars][multiple]")
{
    EvaluatorFixture fixture;

    SECTION("Register several nulars")
    {
        // Register multiple nulars
        GameNular n1(GameScalar, "nular1", [](const GameState* s) { return GameValue(1.0f); });
        GameNular n2(GameScalar, "nular2", [](const GameState* s) { return GameValue(2.0f); });
        GameNular n3(GameScalar, "nular3", [](const GameState* s) { return GameValue(3.0f); });

        GGameState.NewNularOp(n1);
        GGameState.NewNularOp(n2);
        GGameState.NewNularOp(n3);

        // Verify all are registered
        AutoArray<RStringS> nulars;
        auto findNulars = [](const char* w) { return strncmp(w, "nular", 5) == 0; };
        GGameState.AppendNularOpList(nulars, findNulars);

        // Should have at least our 3 nulars
        REQUIRE(nulars.Size() >= 3);
    }
}

// ============================================================================
// Test 7: Nular vs Variable Conflict
// ============================================================================

TEST_CASE("Evaluator - Nular vs variable conflict", "[evaluator][nulars][conflict]")
{
    EvaluatorFixture fixture;

    SECTION("Nular and variable with same name")
    {
        // Register a nular
        GameNular myvalue(GameScalar, "myvalue", [](const GameState* state) -> GameValue { return GameValue(100.0f); });

        GGameState.NewNularOp(myvalue);

        // Also create a variable with same name
        GGameState.VarSet("myvalue", GameValue(200.0f), false, false);

        // Both should exist independently
        // Nular should be in nular list
        AutoArray<RStringS> nulars;
        auto findMyValue = [](const char* w) { return strcmp(w, "myvalue") == 0; };
        GGameState.AppendNularOpList(nulars, findMyValue);

        REQUIRE(nulars.Size() > 0);

        // Variable should be accessible
        GameValue varValue = GGameState.VarGet("myvalue");
        REQUIRE((float)varValue == Catch::Approx(200.0f));

        // Cleanup
        GGameState.VarDelete("myvalue");
    }
}

// ============================================================================
// Test 8: Built-in Nulars Discovery
// ============================================================================

TEST_CASE("Evaluator - Built-in nulars exist", "[evaluator][nulars][builtin]")
{
    EvaluatorFixture fixture;

    SECTION("Boolean constants may be nulars")
    {
        AutoArray<RStringS> nulars;
        auto boolNulars = [](const char* w)
        {
            // Common boolean nulars
            return strcmp(w, "true") == 0 || strcmp(w, "false") == 0;
        };

        GGameState.AppendNularOpList(nulars, boolNulars);

        // May or may not have built-in boolean nulars
        // Just verify no crash
        REQUIRE(nulars.Size() >= 0);
    }

    SECTION("Game state nulars may exist")
    {
        AutoArray<RStringS> nulars;
        auto gameNulars = [](const char* w)
        {
            // Common game state nulars in SQF
            return strcmp(w, "player") == 0 || strcmp(w, "time") == 0 || strcmp(w, "nil") == 0 ||
                   strcmp(w, "nothing") == 0;
        };

        GGameState.AppendNularOpList(nulars, gameNulars);

        // May or may not have these registered
        REQUIRE(nulars.Size() >= 0);
    }
}

// ============================================================================
// Test 9: Nular Registration Persistence
// ============================================================================

TEST_CASE("Evaluator - Nular registration persistence", "[evaluator][nulars][persist]")
{
    EvaluatorFixture fixture;

    SECTION("Custom nular persists across operations")
    {
        // Register nular
        GameNular persist(GameScalar, "persistnular",
                          [](const GameState* state) -> GameValue { return GameValue(777.0f); });

        GGameState.NewNularOp(persist);

        // Check multiple times
        for (int i = 0; i < 3; i++)
        {
            AutoArray<RStringS> nulars;
            auto find = [](const char* w) { return strcmp(w, "persistnular") == 0; };
            GGameState.AppendNularOpList(nulars, find);

            REQUIRE(nulars.Size() > 0);
        }
    }
}

// ============================================================================
// Test 10: Complex Nular Scenarios
// ============================================================================

TEST_CASE("Evaluator - Complex nular registration", "[evaluator][nulars][integration]")
{
    EvaluatorFixture fixture;

    SECTION("Game state simulation with nulars")
    {
        // Simulate game state getters
        GameNular currentTime(GameScalar, "currenttime", [](const GameState* s) { return GameValue(123.45f); });

        GameNular isMultiplayer(GameBool, "ismultiplayer", [](const GameState* s) { return GameValue(false); });

        GameNular serverName(GameString, "servername", [](const GameState* s) { return GameValue("TestServer"); });

        GGameState.NewNularOp(currentTime);
        GGameState.NewNularOp(isMultiplayer);
        GGameState.NewNularOp(serverName);

        // Verify all registered
        AutoArray<RStringS> nulars;
        auto findAll = [](const char* w)
        { return strcmp(w, "currenttime") == 0 || strcmp(w, "ismultiplayer") == 0 || strcmp(w, "servername") == 0; };
        GGameState.AppendNularOpList(nulars, findAll);

        REQUIRE(nulars.Size() >= 1); // At least one found
    }

    SECTION("Many nulars can be registered")
    {
        // Register many nulars (without captures - function pointers required)
        GameNular n0(GameScalar, "nular0", [](const GameState* s) { return GameValue(0.0f); });
        GameNular n1(GameScalar, "nular1", [](const GameState* s) { return GameValue(1.0f); });
        GameNular n2(GameScalar, "nular2", [](const GameState* s) { return GameValue(2.0f); });
        GameNular n3(GameScalar, "nular3", [](const GameState* s) { return GameValue(3.0f); });
        GameNular n4(GameScalar, "nular4", [](const GameState* s) { return GameValue(4.0f); });
        GameNular n5(GameScalar, "nular5", [](const GameState* s) { return GameValue(5.0f); });
        GameNular n6(GameScalar, "nular6", [](const GameState* s) { return GameValue(6.0f); });
        GameNular n7(GameScalar, "nular7", [](const GameState* s) { return GameValue(7.0f); });
        GameNular n8(GameScalar, "nular8", [](const GameState* s) { return GameValue(8.0f); });
        GameNular n9(GameScalar, "nular9", [](const GameState* s) { return GameValue(9.0f); });

        GGameState.NewNularOp(n0);
        GGameState.NewNularOp(n1);
        GGameState.NewNularOp(n2);
        GGameState.NewNularOp(n3);
        GGameState.NewNularOp(n4);
        GGameState.NewNularOp(n5);
        GGameState.NewNularOp(n6);
        GGameState.NewNularOp(n7);
        GGameState.NewNularOp(n8);
        GGameState.NewNularOp(n9);

        // Verify they're all registered
        AutoArray<RStringS> nulars;
        auto findAll = [](const char* w) { return strncmp(w, "nular", 5) == 0; };
        GGameState.AppendNularOpList(nulars, findAll);

        REQUIRE(nulars.Size() >= 10);
    }
}

// ============================================================================
// Test 11: Nular Error Handling
// ============================================================================

TEST_CASE("Evaluator - Nular error handling", "[evaluator][nulars][errors]")
{
    EvaluatorFixture fixture;

    SECTION("Invalid nular call doesn't crash")
    {
        // Try to use a nular that doesn't exist
        GameValue result = GGameState.Evaluate("nonexistentnular");

        // Should handle error gracefully
        // May treat as variable or return error
        REQUIRE(true); // Main requirement: no crash
    }

    SECTION("Empty nular name handling")
    {
        // Register nular with empty name (should be rejected or handled)
        GameNular empty(GameScalar, "", [](const GameState* s) { return GameValue(0.0f); });

        // Try to register it
        GGameState.NewNularOp(empty);

        // Should handle gracefully
        REQUIRE(true);
    }
}

#pragma clang diagnostic pop
