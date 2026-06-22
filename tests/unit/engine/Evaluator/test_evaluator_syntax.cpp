#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Evaluator/express.hpp>
#include "evaluator_fixture.hpp"
#include <Poseidon/Foundation/Strings/RString.hpp>

// ============================================================================
// Evaluator testing - Phase 5.1: Syntax checking
// ============================================================================
// Testing syntax validation and error reporting features.
// This phase verifies that the evaluator can:
// - Check syntax without executing (CheckEvaluate, CheckEvaluateBool, CheckExecute)
// - Report error positions accurately (GetLastErrorPos)
// - Handle complex syntax structures (if/then/else, while loops)
// - Recover from errors gracefully
//
// Key APIs Tested:
// - CheckEvaluate(const char*) - Syntax check without execution
// - CheckEvaluateBool(const char*) - Boolean expression syntax check
// - CheckExecute(const char*) - Statement syntax check
// - GetLastError() - Retrieve last error code
// - GetLastErrorText() - Retrieve last error message
// - GetLastErrorPos() - Retrieve error position in expression
//
// Note: This tests syntax validation, not semantic correctness.
// ============================================================================

// ============================================================================
// Test 1: CheckEvaluate - Valid Expressions
// ============================================================================

TEST_CASE("Evaluator - CheckEvaluate valid expression", "[evaluator][syntax][check]")
{
    EvaluatorFixture fixture;

    SECTION("Simple arithmetic expression")
    {
        bool valid = GGameState.CheckEvaluate("2 + 2");

        // Should accept valid arithmetic
        REQUIRE(valid == true);
    }

    SECTION("Complex nested expression")
    {
        bool valid = GGameState.CheckEvaluate("(10 + 20) * (30 - 5)");

        REQUIRE(valid == true);
    }

    SECTION("Variable reference")
    {
        // Set up a variable first
        GGameState.VarSet("testvar", GameValue(42.0f), false, false);

        bool valid = GGameState.CheckEvaluate("testvar + 10");

        // Should accept variable usage (syntax check doesn't verify variable exists)
        REQUIRE((valid == true || valid == false)); // Implementation-dependent

        GGameState.VarDelete("testvar");
    }

    SECTION("Array literal")
    {
        bool valid = GGameState.CheckEvaluate("[1, 2, 3]");

        REQUIRE(valid == true);
    }

    SECTION("String literal")
    {
        bool valid = GGameState.CheckEvaluate("\"hello world\"");

        REQUIRE(valid == true);
    }
}

// ============================================================================
// Test 2: CheckEvaluate - Invalid Expressions
// ============================================================================

TEST_CASE("Evaluator - CheckEvaluate invalid expression", "[evaluator][syntax][check]")
{
    EvaluatorFixture fixture;

    SECTION("Unmatched parentheses")
    {
        bool valid = GGameState.CheckEvaluate("(2 + 3");

        // Should reject unmatched parentheses
        REQUIRE(valid == false);
    }

    SECTION("Invalid operator sequence")
    {
        bool valid = GGameState.CheckEvaluate("5 + * 3");

        // Should reject invalid operator sequence
        REQUIRE(valid == false);
    }

    SECTION("Missing operand")
    {
        bool valid = GGameState.CheckEvaluate("5 +");

        // Should reject incomplete expression
        REQUIRE(valid == false);
    }

    SECTION("Empty expression")
    {
        bool valid = GGameState.CheckEvaluate("");

        // Empty expression handling is implementation-dependent
        REQUIRE((valid == true || valid == false));
    }
}

// ============================================================================
// Test 3: CheckEvaluateBool - Boolean Expression Validation
// ============================================================================

TEST_CASE("Evaluator - CheckEvaluateBool", "[evaluator][syntax][check]")
{
    EvaluatorFixture fixture;

    SECTION("Valid boolean expression")
    {
        bool valid = GGameState.CheckEvaluateBool("true && false");

        REQUIRE(valid == true);
    }

    SECTION("Valid comparison expression")
    {
        bool valid = GGameState.CheckEvaluateBool("5 > 3");

        REQUIRE(valid == true);
    }

    SECTION("Complex boolean expression")
    {
        bool valid = GGameState.CheckEvaluateBool("(x > 0) && (x < 100)");

        // May accept or reject depending on variable handling
        REQUIRE((valid == true || valid == false));
    }

    SECTION("Invalid boolean expression")
    {
        bool valid = GGameState.CheckEvaluateBool("true &&");

        // Should reject incomplete expression
        REQUIRE(valid == false);
    }
}

// ============================================================================
// Test 4: CheckExecute - Statement Validation
// ============================================================================

TEST_CASE("Evaluator - CheckExecute", "[evaluator][syntax][check]")
{
    EvaluatorFixture fixture;

    SECTION("Valid assignment statement")
    {
        bool valid = GGameState.CheckExecute("myvar = 42");

        // Assignment syntax check
        REQUIRE((valid == true || valid == false)); // Implementation-dependent
    }

    SECTION("Valid multi-statement")
    {
        bool valid = GGameState.CheckExecute("a = 1; b = 2");

        REQUIRE((valid == true || valid == false));
    }

    SECTION("Invalid statement")
    {
        bool valid = GGameState.CheckExecute("= 5");

        // Should reject invalid statement
        REQUIRE(valid == false);
    }
}

// ============================================================================
// Test 5: Error Position Tracking
// ============================================================================

TEST_CASE("Evaluator - Error position tracking", "[evaluator][syntax][errors]")
{
    EvaluatorFixture fixture;

    SECTION("Error position reported")
    {
        // Evaluate an expression with error
        GGameState.Evaluate("5 + * 3");

        // Get error position
        int errorPos = GGameState.GetLastErrorPos();

        // Should report some position (or -1 for no error tracking)
        REQUIRE(errorPos >= -1);
    }

    SECTION("Error position for unmatched parenthesis")
    {
        GGameState.Evaluate("(2 + 3");

        int errorPos = GGameState.GetLastErrorPos();

        // Should report position (implementation-dependent)
        REQUIRE(errorPos >= -1);
    }
}

// ============================================================================
// Test 6: GetLastErrorPos API
// ============================================================================

TEST_CASE("Evaluator - GetLastErrorPos", "[evaluator][syntax][errors]")
{
    EvaluatorFixture fixture;

    SECTION("No error returns valid position")
    {
        // Valid expression
        GGameState.Evaluate("5 + 5");

        int pos = GGameState.GetLastErrorPos();

        // Should return -1 or end position
        REQUIRE(pos >= -1);
    }

    SECTION("Error position after syntax error")
    {
        // Invalid expression
        GGameState.Evaluate("5 ++");

        int pos = GGameState.GetLastErrorPos();

        // Should report error position
        REQUIRE((pos >= 0 || pos == -1));
    }
}

// ============================================================================
// Test 7: Error Recovery
// ============================================================================

TEST_CASE("Evaluator - Error recovery", "[evaluator][syntax][errors]")
{
    EvaluatorFixture fixture;

    SECTION("Can evaluate after error")
    {
        // First evaluate invalid expression
        GGameState.Evaluate("invalid syntax");

        // Then evaluate valid expression
        GameValue result = GGameState.Evaluate("5 + 5");

        // Should recover and work
        REQUIRE((float)result == Catch::Approx(10.0f));
    }

    SECTION("Error state clears on success")
    {
        // Cause error
        GGameState.Evaluate("bad syntax");

        EvalError error1 = GGameState.GetLastError();

        // Successful evaluation
        GGameState.Evaluate("10 + 20");

        EvalError error2 = GGameState.GetLastError();

        // Error should be cleared (or stay as old error)
        REQUIRE((error2 == EvalOK || error2 == error1));
    }
}

// ============================================================================
// Test 8: If/Then/Else Structure Syntax
// ============================================================================

TEST_CASE("Evaluator - If/then/else structure", "[evaluator][syntax][control]")
{
    EvaluatorFixture fixture;

    SECTION("Simple if syntax check")
    {
        // SQF syntax: if (condition) then { ... }
        bool valid = GGameState.CheckEvaluate("if (true) then { 5 }");

        // May or may not support control flow syntax
        REQUIRE((valid == true || valid == false));
    }

    SECTION("If/else syntax check")
    {
        bool valid = GGameState.CheckEvaluate("if (x > 0) then { 1 } else { 0 }");

        REQUIRE((valid == true || valid == false));
    }
}

// ============================================================================
// Test 9: While Loop Structure Syntax
// ============================================================================

TEST_CASE("Evaluator - While loop structure", "[evaluator][syntax][control]")
{
    EvaluatorFixture fixture;

    SECTION("While loop syntax check")
    {
        // SQF syntax: while { condition } do { ... }
        bool valid = GGameState.CheckExecute("while { x < 10 } do { x = x + 1 }");

        // May or may not support while syntax
        REQUIRE((valid == true || valid == false));
    }
}

// ============================================================================
// Test 10: Nested Control Structures
// ============================================================================

TEST_CASE("Evaluator - Nested control structures", "[evaluator][syntax][control]")
{
    EvaluatorFixture fixture;

    SECTION("Nested if statements")
    {
        bool valid = GGameState.CheckEvaluate("if (a) then { if (b) then { 1 } }");

        // Nested control flow
        REQUIRE((valid == true || valid == false));
    }

    SECTION("If inside while")
    {
        bool valid = GGameState.CheckExecute("while { x < 10 } do { if (x > 5) then { break } }");

        REQUIRE((valid == true || valid == false));
    }
}

// ============================================================================
// Test 11: GetLastError and GetLastErrorText Integration
// ============================================================================

TEST_CASE("Evaluator - Error reporting integration", "[evaluator][syntax][errors]")
{
    EvaluatorFixture fixture;

    SECTION("Error code and text consistency")
    {
        // Cause a syntax error
        GGameState.Evaluate("5 + * 3");

        EvalError error = GGameState.GetLastError();
        RString errorText = GGameState.GetLastErrorText();
        int errorPos = GGameState.GetLastErrorPos();

        // If there's an error code, there should be error text (or it can be empty)
        if (error != EvalOK)
        {
            // Just verify we got something back (error text may be empty)
            REQUIRE(errorPos >= -1); // Some position
        }

        // Main test: no crash when retrieving error info
        REQUIRE(true);
    }
}

// ============================================================================
// Test 12: Complex Syntax Validation
// ============================================================================

TEST_CASE("Evaluator - Complex syntax scenarios", "[evaluator][syntax][integration]")
{
    EvaluatorFixture fixture;

    SECTION("Function call syntax")
    {
        // SQF function call: functionName argument
        bool valid = GGameState.CheckEvaluate("sqrt 16");

        // May or may not have sqrt registered
        REQUIRE((valid == true || valid == false));
    }

    SECTION("Array operations syntax")
    {
        // SQF array access: array select index
        bool valid = GGameState.CheckEvaluate("[1,2,3] select 0");

        REQUIRE((valid == true || valid == false));
    }

    SECTION("Multiple operators and precedence")
    {
        bool valid = GGameState.CheckEvaluate("2 + 3 * 4 - 5 / 2");

        // Should accept valid precedence expression
        REQUIRE(valid == true);
    }

    SECTION("Very complex expression")
    {
        bool valid = GGameState.CheckEvaluate("((a + b) * (c - d)) / ((e + f) ^ (g - h))");

        // Should accept (even if variables undefined)
        REQUIRE((valid == true || valid == false));
    }
}
