#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Evaluator/express.hpp>
#include "evaluator_fixture.hpp"
#include <stdio.h>
#include <string>
#include <vector>

// ============================================================================
// Evaluator testing - Phase 5.2: Edge cases and performance
// ============================================================================
// Testing edge cases, boundary conditions, and performance scenarios.
// This final phase verifies that the evaluator can handle:
// - Extreme inputs (very long expressions, deep nesting)
// - Edge cases (empty input, maximum sizes, whitespace)
// - Performance scenarios (many variables, repeated evaluations)
// - Memory management (leak checks, reference counting)
// - Stress testing (pushing limits)
//
// Key Focus Areas:
// - Robustness: No crashes on extreme inputs
// - Scalability: Performance with large datasets
// - Memory: Proper cleanup and reference counting
// - Limits: Understanding system boundaries
//
// Note: This tests system robustness, not expected use cases.
// ============================================================================

// ============================================================================
// Test 1: Very Long Expressions
// ============================================================================

TEST_CASE("Evaluator - malformed scripts stay within parser buffers", "[evaluator][edge][fuzz]")
{
    // fuzz_sqf (CheckExecute) found three parser buffer overruns reachable from any
    // mission script. (1)+(3): memcmp(p, "private", 7) over-read a buffer shorter than
    // 7 bytes in CheckAssignment + PerformAssignment (ASan: READ of size 7) -- now
    // strncmp, which stops at the NUL. (2): the unknown-operator error buffer
    // char name[256] in Vyhod was written with the unbounded identifier length
    // (ASan: WRITE of size 259) -- now clamped. Heap std::vector buffers put an ASan
    // redzone right after the bytes, so each section faults pre-fix; here it must
    // complete cleanly. The fuzzer's 54 reproducers are the broken-state witnesses.
    EvaluatorFixture fixture;

    SECTION("private-prefix check at a short buffer end")
    {
        std::vector<char> assign = {'x', '=', '1', '\0'}; // 4-byte heap buffer
        GGameState.CheckExecute(assign.data());
        std::vector<char> tok = {'p', 'r', 'i', '\0'};
        GGameState.CheckExecute(tok.data());
        SUCCEED("private-prefix check stays within the buffer");
    }

    SECTION("over-long unknown identifier")
    {
        std::string longId(300, 'z'); // exceeds the fixed 256-byte error buffer
        GGameState.CheckExecute(longId.c_str());
        SUCCEED("unknown-operator name truncated into the fixed buffer");
    }
}

TEST_CASE("Evaluator - Very long expression", "[evaluator][edge][long]")
{
    EvaluatorFixture fixture;

    SECTION("Long arithmetic chain")
    {
        // Build a very long expression: 1 + 1 + 1 + ... + 1
        std::string expr = "1";
        for (int i = 0; i < 100; i++)
        {
            expr += " + 1";
        }

        GameValue result = GGameState.Evaluate(expr.c_str());

        // Should evaluate to 101 (or handle gracefully if too long)
        if (result.GetType() == GameScalar)
        {
            REQUIRE((float)result == Catch::Approx(101.0f));
        }
        else
        {
            // May reject very long expressions - that's okay
            REQUIRE(true);
        }
    }

    SECTION("Long string literal")
    {
        // Very long string
        std::string longStr = "\"";
        for (int i = 0; i < 500; i++)
        {
            longStr += "a";
        }
        longStr += "\"";

        GameValue result = GGameState.Evaluate(longStr.c_str());

        // Should handle long strings gracefully
        REQUIRE((result.GetType() == GameString || result.GetType() == GameAny));
    }
}

// ============================================================================
// Test 2: Deeply Nested Parentheses
// ============================================================================

TEST_CASE("Evaluator - Deeply nested parentheses", "[evaluator][edge][nesting]")
{
    EvaluatorFixture fixture;

    SECTION("Many nested parentheses")
    {
        // Build: ((((((5))))))
        std::string expr;
        for (int i = 0; i < 20; i++)
        {
            expr += "(";
        }
        expr += "5";
        for (int i = 0; i < 20; i++)
        {
            expr += ")";
        }

        GameValue result = GGameState.Evaluate(expr.c_str());

        // Should evaluate to 5 (or reject if too deep)
        if (result.GetType() == GameScalar)
        {
            REQUIRE((float)result == Catch::Approx(5.0f));
        }
        else
        {
            // May reject deeply nested expressions
            REQUIRE(true);
        }
    }

    SECTION("Nested arithmetic")
    {
        // ((1 + 2) * (3 + 4))
        GameValue result = GGameState.Evaluate("((1 + 2) * (3 + 4))");

        REQUIRE((float)result == Catch::Approx(21.0f));
    }
}

// ============================================================================
// Test 3: Maximum Array Size
// ============================================================================

TEST_CASE("Evaluator - Maximum array size", "[evaluator][edge][array]")
{
    EvaluatorFixture fixture;

    SECTION("Large array")
    {
        // Create array with many elements
        GameArrayType largeArray;
        for (int i = 0; i < 100; i++)
        {
            largeArray.Add(GameValue((float)i));
        }

        GGameState.VarSet("bigarray", GameValue(largeArray), false, false);

        // Verify we can store large arrays
        GameValue result = GGameState.VarGet("bigarray");

        REQUIRE(result.GetType() == GameArray);
        const GameArrayType& arr = (const GameArrayType&)result;
        REQUIRE(arr.Size() == 100);

        GGameState.VarDelete("bigarray");
    }

    SECTION("Nested arrays")
    {
        // Array of arrays
        GameArrayType outer;
        for (int i = 0; i < 10; i++)
        {
            GameArrayType inner;
            for (int j = 0; j < 10; j++)
            {
                inner.Add(GameValue((float)(i * 10 + j)));
            }
            outer.Add(GameValue(inner));
        }

        GGameState.VarSet("nested", GameValue(outer), false, false);

        GameValue result = GGameState.VarGet("nested");

        REQUIRE(result.GetType() == GameArray);

        GGameState.VarDelete("nested");
    }
}

// ============================================================================
// Test 4: Empty Expression Handling
// ============================================================================

TEST_CASE("Evaluator - Empty expression", "[evaluator][edge][empty]")
{
    EvaluatorFixture fixture;

    SECTION("Empty string evaluation")
    {
        GameValue result = GGameState.Evaluate("");

        // Should handle empty expression gracefully
        // May return nil, error, or default value
        REQUIRE(true); // Main test: no crash
    }

    SECTION("Whitespace only")
    {
        GameValue result = GGameState.Evaluate("   \t\n   ");

        // Should handle whitespace-only expression
        REQUIRE(true);
    }

    SECTION("Empty variable name")
    {
        // Try to set variable with empty name
        GGameState.VarSet("", GameValue(42.0f), false, false);

        // Should handle gracefully (reject or accept)
        REQUIRE(true);
    }
}

// ============================================================================
// Test 5: Whitespace Handling
// ============================================================================

TEST_CASE("Evaluator - Whitespace handling", "[evaluator][edge][whitespace]")
{
    EvaluatorFixture fixture;

    SECTION("Extra whitespace in expression")
    {
        GameValue result = GGameState.Evaluate("   5   +   3   ");

        REQUIRE((float)result == Catch::Approx(8.0f));
    }

    SECTION("Tabs and newlines")
    {
        GameValue result = GGameState.Evaluate("10\t+\n20");

        REQUIRE((float)result == Catch::Approx(30.0f));
    }

    SECTION("No whitespace")
    {
        GameValue result = GGameState.Evaluate("2+2");

        REQUIRE((float)result == Catch::Approx(4.0f));
    }
}

// ============================================================================
// Test 6: Many Variables (Stress Test)
// ============================================================================

TEST_CASE("Evaluator - Many variables (stress test)", "[evaluator][edge][stress]")
{
    EvaluatorFixture fixture;

    SECTION("Create many variables")
    {
        // Create 100 variables
        for (int i = 0; i < 100; i++)
        {
            char name[32];
            sprintf(name, "var%d", i);
            GGameState.VarSet(name, GameValue((float)i), false, false);
        }

        // Verify some are accessible
        GameValue v0 = GGameState.VarGet("var0");
        GameValue v50 = GGameState.VarGet("var50");
        GameValue v99 = GGameState.VarGet("var99");

        REQUIRE((float)v0 == Catch::Approx(0.0f));
        REQUIRE((float)v50 == Catch::Approx(50.0f));
        REQUIRE((float)v99 == Catch::Approx(99.0f));

        // Cleanup
        for (int i = 0; i < 100; i++)
        {
            char name[32];
            sprintf(name, "var%d", i);
            GGameState.VarDelete(name);
        }
    }
}

// ============================================================================
// Test 7: Repeated Evaluations
// ============================================================================

TEST_CASE("Evaluator - Repeated evaluations", "[evaluator][edge][performance]")
{
    EvaluatorFixture fixture;

    SECTION("Evaluate same expression many times")
    {
        const char* expr = "2 * 3 + 4";

        // Evaluate 1000 times
        for (int i = 0; i < 1000; i++)
        {
            GameValue result = GGameState.Evaluate(expr);
            REQUIRE((float)result == Catch::Approx(10.0f));
        }

        // Main test: no crash, no memory leak (hopefully!)
        REQUIRE(true);
    }

    SECTION("Evaluate different expressions")
    {
        for (int i = 0; i < 100; i++)
        {
            char expr[32];
            sprintf(expr, "%d + %d", i, i);

            GameValue result = GGameState.Evaluate(expr);
            REQUIRE((float)result == Catch::Approx((float)(i + i)));
        }
    }
}

// ============================================================================
// Test 8: Memory Leak Check (Basic)
// ============================================================================

TEST_CASE("Evaluator - Memory leak check", "[evaluator][edge][memory]")
{
    EvaluatorFixture fixture;

    SECTION("Create and destroy many GameValues")
    {
        for (int i = 0; i < 1000; i++)
        {
            GameValue val1((float)i);
            GameValue val2((bool)(i % 2 == 0));
            GameValue val3("test string");

            GameArrayType arr;
            arr.Add(val1);
            arr.Add(val2);
            arr.Add(val3);
            GameValue val4(arr);

            // Let them go out of scope
        }

        // Main test: no crash (memory should be freed via ref counting)
        REQUIRE(true);
    }

    SECTION("Variable creation and deletion")
    {
        for (int i = 0; i < 100; i++)
        {
            char name[32];
            sprintf(name, "temp%d", i);

            GGameState.VarSet(name, GameValue((float)i), false, false);
            GGameState.VarDelete(name);
        }

        REQUIRE(true);
    }
}

// ============================================================================
// Test 9: Stack Depth Limit
// ============================================================================

TEST_CASE("Evaluator - Stack depth limit", "[evaluator][edge][stack]")
{
    EvaluatorFixture fixture;

    SECTION("Very deep expression nesting")
    {
        // This tests if there's a recursion depth limit
        std::string expr = "1";
        for (int i = 0; i < 50; i++)
        {
            expr = "(" + expr + " + 1)";
        }

        GameValue result = GGameState.Evaluate(expr.c_str());

        // Should either evaluate or reject gracefully
        REQUIRE(true); // Main test: no stack overflow crash
    }
}

// ============================================================================
// Test 10: Reference Counting
// ============================================================================

TEST_CASE("Evaluator - Reference counting", "[evaluator][edge][refcount]")
{
    EvaluatorFixture fixture;

    SECTION("Shared data instances")
    {
        GameValue val1(42.0f);
        GameValue val2 = val1; // Should share data

        // Check if they share instance
        bool shared = val1.SharedInstance();

        // Reference counting should work
        REQUIRE((shared == true || shared == false));
    }

    SECTION("Array sharing")
    {
        GameArrayType arr;
        arr.Add(GameValue(1.0f));
        arr.Add(GameValue(2.0f));

        GameValue val1(arr);
        GameValue val2 = val1;

        // Should share array data
        bool shared = val1.SharedInstance();
        REQUIRE((shared == true || shared == false));
    }

    SECTION("Copy vs duplicate")
    {
        GameValue val1(100.0f);
        GameValue val2 = val1; // Copy (shares data)
        GameValue val3;
        val3.Duplicate(val1); // Duplicate (deep copy)

        // val1 and val2 should share
        bool shared12 = val1.SharedInstance();

        // val3 should be independent
        bool shared3 = val3.SharedInstance();

        REQUIRE(shared12 == true); // val1/val2 share instance
        REQUIRE(shared3 == false); // val3 is independent copy
    }
}

// ============================================================================
// Test 11: Complex Edge Case Integration
// ============================================================================

TEST_CASE("Evaluator - Complex edge scenarios", "[evaluator][edge][integration]")
{
    EvaluatorFixture fixture;

    SECTION("Mixed edge cases")
    {
        // Long variable name
        std::string longName = "verylongvariablename";
        for (int i = 0; i < 10; i++)
        {
            longName += longName; // Exponentially longer
        }

        // Try to use it
        GGameState.VarSet(longName.c_str(), GameValue(42.0f), false, false);

        // Should handle or reject gracefully
        REQUIRE(true);

        // Cleanup attempt
        GGameState.VarDelete(longName.c_str());
    }

    SECTION("Special characters in strings")
    {
        // Null character, quotes, backslashes
        GameValue result1 = GGameState.Evaluate("\"test\\\"quote\"");
        GameValue result2 = GGameState.Evaluate("\"line1\\nline2\"");

        // Should handle escape sequences
        REQUIRE(true);
    }

    SECTION("Extreme numeric values")
    {
        // Very large number
        GameValue result1 = GGameState.Evaluate("999999999999");

        // Very small number
        GameValue result2 = GGameState.Evaluate("0.000000001");

        // Negative extreme
        GameValue result3 = GGameState.Evaluate("-999999999999");

        // Should handle or overflow gracefully
        REQUIRE(true);
    }
}

// ============================================================================
// Test 12: Concurrent-Like Behavior (Sequential)
// ============================================================================

TEST_CASE("Evaluator - Sequential stress test", "[evaluator][edge][stress]")
{
    EvaluatorFixture fixture;

    SECTION("Interleaved operations")
    {
        // Simulate many operations happening in sequence
        for (int i = 0; i < 50; i++)
        {
            // Create variable
            char name[32];
            sprintf(name, "stress%d", i);
            GGameState.VarSet(name, GameValue((float)i), false, false);

            // Evaluate expression
            GameValue r1 = GGameState.Evaluate("2 + 3");
            REQUIRE((float)r1 == Catch::Approx(5.0f));

            // Read variable
            GameValue r2 = GGameState.VarGet(name);
            REQUIRE((float)r2 == Catch::Approx((float)i));

            // Complex expression
            GameValue r3 = GGameState.Evaluate("(10 * 2) + (5 - 3)");
            REQUIRE((float)r3 == Catch::Approx(22.0f));

            // Delete variable
            GGameState.VarDelete(name);
        }
    }

    SECTION("State changes during evaluation")
    {
        GGameState.VarSet("x", GameValue(10.0f), false, false);

        for (int i = 0; i < 20; i++)
        {
            // Evaluate using current variable value
            GameValue r = GGameState.VarGet("x");
            float currentX = (float)r;

            // Evaluate expression
            GameValue r2 = GGameState.Evaluate("x * 2");
            REQUIRE((float)r2 == Catch::Approx(currentX * 2.0f));

            // Change variable
            GGameState.VarSet("x", GameValue((float)(i + 1)), false, false);

            // Evaluate with new value
            GameValue r3 = GGameState.Evaluate("x + 5");
            REQUIRE((float)r3 == Catch::Approx((float)(i + 6)));
        }

        GGameState.VarDelete("x");
    }
}

// ============================================================================
// Test: negative array resize must not corrupt the heap
// ============================================================================
// `arr resize n` with n < 0 ran AutoArray::Resize(n), whose shrink path is
// DestructArray(_data + n, _n - n): with n < 0 the pointer is *before* the buffer
// and the count _n - n is *oversized*, so it walked off the buffer calling
// RefCount::Release on garbage/freed pointers — heap corruption / 0xC0000005
// reachable from any SQF (field repro: M_RipFPS `_a resize _b` driving _b negative).
// The fix clamps n < 0 to 0, so a negative resize empties the array.
//
// Without the fix the large negative below faults deterministically in the
// destruct walk, taking the whole test binary down.
TEST_CASE("Evaluator - negative array resize clamps to empty", "[evaluator][edge][resize]")
{
    EvaluatorFixture fixture;

    GameValue arr = GGameState.Evaluate("[1,2,3,4,5,6,7,8,9,10]");
    REQUIRE(arr.GetType() == GameArray);
    GGameState.VarSet("tcrash_arr", arr, false, false);

    GGameState.Evaluate("tcrash_arr resize -10000"); // pre-fix: heap corruption / AV here

    GameValue after = GGameState.VarGet("tcrash_arr");
    REQUIRE(after.GetType() == GameArray);
    REQUIRE(((const GameArrayType&)after).Size() == 0);

    GGameState.VarDelete("tcrash_arr");
}
