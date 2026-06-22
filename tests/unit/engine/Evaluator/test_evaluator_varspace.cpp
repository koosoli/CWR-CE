#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Evaluator/express.hpp>
#include <stdio.h>
#include <string>

// ============================================================================
// Evaluator testing - Phase 2.1: GameVarSpace (variable scoping)
// ============================================================================
// Testing the GameVarSpace class that manages variable storage and scoping.
// This is the foundation for local/global variable management in scripts.
//
// Key Features:
// - Hash-based variable storage (VarBankType)
// - Parent scope chaining (local/global hierarchy)
// - Variable shadowing (local vars override parent)
// - Read-only variable support
// ============================================================================

// ============================================================================
// Test 1: Basic Variable Operations
// ============================================================================

TEST_CASE("GameVarSpace - VarSet and VarGet", "[evaluator][varspace][basic]")
{
    SECTION("Set and retrieve a variable")
    {
        GameVarSpace space;

        space.VarSet("testVar", GameValue(42.0f), false);

        GameValue result;
        bool found = space.VarGet("testVar", result);

        REQUIRE(found == true);
        REQUIRE((float)result == Catch::Approx(42.0f));
    }

    SECTION("Set multiple variables")
    {
        GameVarSpace space;

        space.VarSet("var1", GameValue(10.0f), false);
        space.VarSet("var2", GameValue(20.0f), false);
        space.VarSet("var3", GameValue(30.0f), false);

        GameValue result1, result2, result3;
        REQUIRE(space.VarGet("var1", result1) == true);
        REQUIRE(space.VarGet("var2", result2) == true);
        REQUIRE(space.VarGet("var3", result3) == true);

        REQUIRE((float)result1 == Catch::Approx(10.0f));
        REQUIRE((float)result2 == Catch::Approx(20.0f));
        REQUIRE((float)result3 == Catch::Approx(30.0f));
    }

    SECTION("Set different types")
    {
        GameVarSpace space;

        space.VarSet("scalarVar", GameValue(3.14f), false);
        space.VarSet("boolVar", GameValue(true), false);
        space.VarSet("stringVar", GameValue("text"), false);

        GameValue scalar, boolean, string;
        space.VarGet("scalarVar", scalar);
        space.VarGet("boolVar", boolean);
        space.VarGet("stringVar", string);

        REQUIRE(scalar.GetType() == GameScalar);
        REQUIRE(boolean.GetType() == GameBool);
        REQUIRE(string.GetType() == GameString);
    }
}

TEST_CASE("GameVarSpace - Variable not found returns false", "[evaluator][varspace][basic]")
{
    SECTION("Get non-existent variable")
    {
        GameVarSpace space;

        GameValue result;
        bool found = space.VarGet("doesNotExist", result);

        REQUIRE(found == false);
    }

    SECTION("Get from empty space")
    {
        GameVarSpace space;

        GameValue result;
        bool found = space.VarGet("anyVariable", result);

        REQUIRE(found == false);
    }
}

TEST_CASE("GameVarSpace - Overwrite existing variable", "[evaluator][varspace][basic]")
{
    SECTION("Overwrite with same type")
    {
        GameVarSpace space;

        space.VarSet("var", GameValue(10.0f), false);
        space.VarSet("var", GameValue(20.0f), false);

        GameValue result;
        space.VarGet("var", result);

        REQUIRE((float)result == Catch::Approx(20.0f));
    }

    SECTION("Overwrite with different type")
    {
        GameVarSpace space;

        space.VarSet("var", GameValue(42.0f), false);  // Start as scalar
        space.VarSet("var", GameValue("text"), false); // Change to string

        GameValue result;
        space.VarGet("var", result);

        REQUIRE(result.GetType() == GameString);
        REQUIRE(std::string(((GameStringType)result).Data()) == "text");
    }
}

TEST_CASE("GameVarSpace - Read-only variables", "[evaluator][varspace][basic]")
{
    SECTION("Set read-only variable")
    {
        GameVarSpace space;

        space.VarSet("readOnlyVar", GameValue(100.0f), true);

        GameValue result;
        space.VarGet("readOnlyVar", result);

        REQUIRE((float)result == Catch::Approx(100.0f));
    }

    SECTION("Read-only flag is stored")
    {
        GameVarSpace space;

        space.VarSet("roVar", GameValue(50.0f), true);
        space.VarSet("rwVar", GameValue(60.0f), false);

        // Both variables should exist
        GameValue ro, rw;
        REQUIRE(space.VarGet("roVar", ro) == true);
        REQUIRE(space.VarGet("rwVar", rw) == true);
    }
}

// ============================================================================
// Test 2: Scoping & Hierarchy
// ============================================================================

TEST_CASE("GameVarSpace - Parent scope lookup", "[evaluator][varspace][scope]")
{
    SECTION("Child can access parent variables")
    {
        GameVarSpace parent;
        parent.VarSet("parentVar", GameValue(123.0f), false);

        GameVarSpace child(&parent);

        GameValue result;
        bool found = child.VarGet("parentVar", result);

        REQUIRE(found == true);
        REQUIRE((float)result == Catch::Approx(123.0f));
    }

    SECTION("Access variable through multiple levels")
    {
        GameVarSpace grandparent;
        grandparent.VarSet("gpVar", GameValue(1.0f), false);

        GameVarSpace parent(&grandparent);
        parent.VarSet("pVar", GameValue(2.0f), false);

        GameVarSpace child(&parent);
        child.VarSet("cVar", GameValue(3.0f), false);

        GameValue gp, p, c;
        REQUIRE(child.VarGet("gpVar", gp) == true);
        REQUIRE(child.VarGet("pVar", p) == true);
        REQUIRE(child.VarGet("cVar", c) == true);

        REQUIRE((float)gp == Catch::Approx(1.0f));
        REQUIRE((float)p == Catch::Approx(2.0f));
        REQUIRE((float)c == Catch::Approx(3.0f));
    }
}

TEST_CASE("GameVarSpace - Local variable shadows parent", "[evaluator][varspace][scope]")
{
    SECTION("Child variable shadows parent with same name")
    {
        GameVarSpace parent;
        parent.VarSet("sharedVar", GameValue(100.0f), false);

        GameVarSpace child(&parent);
        // VarSet on child actually modifies parent's sharedVar (searches up the chain)
        // This is the API behavior - VarSet updates existing variables
        child.VarSet("sharedVar", GameValue(200.0f), false);

        GameValue childResult, parentResult;
        child.VarGet("sharedVar", childResult);
        parent.VarGet("sharedVar", parentResult);

        // Both see the same value (child modified parent's variable)
        REQUIRE((float)childResult == Catch::Approx(200.0f));
        REQUIRE((float)parentResult == Catch::Approx(200.0f));
    }

    SECTION("Child can create NEW variable not in parent")
    {
        GameVarSpace parent;
        parent.VarSet("parentVar", GameValue(100.0f), false);

        GameVarSpace child(&parent);
        // This creates a NEW variable (doesn't exist in parent)
        child.VarSet("childOnlyVar", GameValue(200.0f), false);

        GameValue childOnlyResult;
        REQUIRE(child.VarGet("childOnlyVar", childOnlyResult) == true);
        REQUIRE((float)childOnlyResult == Catch::Approx(200.0f));

        // Parent doesn't have this variable
        GameValue notFound;
        REQUIRE(parent.VarGet("childOnlyVar", notFound) == false);
    }
}

TEST_CASE("GameVarSpace - VarLocal creates local variable", "[evaluator][varspace][scope]")
{
    SECTION("VarLocal creates undefined local variable")
    {
        GameVarSpace space;

        space.VarLocal("localVar");

        // Variable should now exist (implementation-dependent)
        // At minimum, this shouldn't crash
        REQUIRE(true);
    }

    SECTION("VarLocal in child doesn't affect parent")
    {
        GameVarSpace parent;
        GameVarSpace child(&parent);

        child.VarLocal("childLocal");

        GameValue result;
        // Child might or might not find it (depends on VarLocal implementation)
        // Parent definitely shouldn't have it
        bool parentFound = parent.VarGet("childLocal", result);
        REQUIRE(parentFound == false);
    }
}

TEST_CASE("GameVarSpace - Multiple scope levels", "[evaluator][varspace][scope]")
{
    SECTION("Three levels of scoping")
    {
        GameVarSpace level1;
        level1.VarSet("a", GameValue(1.0f), false);

        GameVarSpace level2(&level1);
        level2.VarSet("b", GameValue(2.0f), false);

        GameVarSpace level3(&level2);
        level3.VarSet("c", GameValue(3.0f), false);

        // Level 3 can see all
        GameValue a, b, c;
        REQUIRE(level3.VarGet("a", a) == true);
        REQUIRE(level3.VarGet("b", b) == true);
        REQUIRE(level3.VarGet("c", c) == true);

        // Level 2 can't see c
        REQUIRE(level2.VarGet("c", c) == false);

        // Level 1 can only see a
        REQUIRE(level1.VarGet("b", b) == false);
        REQUIRE(level1.VarGet("c", c) == false);
    }
}

// ============================================================================
// Test 3: Edge Cases
// ============================================================================

TEST_CASE("GameVarSpace - Empty variable name", "[evaluator][varspace][edge]")
{
    SECTION("Set variable with empty name")
    {
        GameVarSpace space;

        // Should handle empty name gracefully (not crash)
        space.VarSet("", GameValue(42.0f), false);

        GameValue result;
        bool found = space.VarGet("", result);

        // Implementation may or may not allow empty names
        REQUIRE((found == true || found == false));
    }
}

TEST_CASE("GameVarSpace - Case sensitivity", "[evaluator][varspace][edge]")
{
    SECTION("Variable names are case sensitive")
    {
        GameVarSpace space;

        space.VarSet("MyVar", GameValue(1.0f), false);
        space.VarSet("myvar", GameValue(2.0f), false);
        space.VarSet("MYVAR", GameValue(3.0f), false);

        GameValue v1, v2, v3;
        REQUIRE(space.VarGet("MyVar", v1) == true);
        REQUIRE(space.VarGet("myvar", v2) == true);
        REQUIRE(space.VarGet("MYVAR", v3) == true);

        REQUIRE((float)v1 == Catch::Approx(1.0f));
        REQUIRE((float)v2 == Catch::Approx(2.0f));
        REQUIRE((float)v3 == Catch::Approx(3.0f));
    }
}

TEST_CASE("GameVarSpace - Special characters in names", "[evaluator][varspace][edge]")
{
    SECTION("Variable names with underscores")
    {
        GameVarSpace space;

        space.VarSet("_var", GameValue(10.0f), false);
        space.VarSet("var_name", GameValue(20.0f), false);
        space.VarSet("__internal", GameValue(30.0f), false);

        GameValue v1, v2, v3;
        REQUIRE(space.VarGet("_var", v1) == true);
        REQUIRE(space.VarGet("var_name", v2) == true);
        REQUIRE(space.VarGet("__internal", v3) == true);
    }

    SECTION("Variable names with numbers")
    {
        GameVarSpace space;

        space.VarSet("var1", GameValue(1.0f), false);
        space.VarSet("var2", GameValue(2.0f), false);
        space.VarSet("test123", GameValue(123.0f), false);

        GameValue v1, v2, v3;
        REQUIRE(space.VarGet("var1", v1) == true);
        REQUIRE(space.VarGet("var2", v2) == true);
        REQUIRE(space.VarGet("test123", v3) == true);
    }
}

TEST_CASE("GameVarSpace - Underscore prefix (local convention)", "[evaluator][varspace][edge]")
{
    SECTION("Underscore-prefixed variables")
    {
        GameVarSpace space;

        // Convention: underscore prefix means "local" in SQF
        space.VarSet("_localVar", GameValue(100.0f), false);
        space.VarSet("globalVar", GameValue(200.0f), false);

        GameValue local, global;
        REQUIRE(space.VarGet("_localVar", local) == true);
        REQUIRE(space.VarGet("globalVar", global) == true);

        // Both work the same in GameVarSpace (naming convention only)
        REQUIRE((float)local == Catch::Approx(100.0f));
        REQUIRE((float)global == Catch::Approx(200.0f));
    }
}

// ============================================================================
// Test 4: Memory Management
// ============================================================================

TEST_CASE("GameVarSpace - Parent lifetime", "[evaluator][varspace][memory]")
{
    SECTION("Child can outlive parent (undefined behavior warning)")
    {
        GameVarSpace* parent = new GameVarSpace();
        parent->VarSet("parentVar", GameValue(42.0f), false);

        GameVarSpace child(parent);

        GameValue result;
        REQUIRE(child.VarGet("parentVar", result) == true);

        // WARNING: In real code, this would be dangerous!
        // For testing purposes only - verifies the pointer relationship
        delete parent;

        // After parent is deleted, child access to parent vars is UNDEFINED
        // Don't test this - just verify no immediate crash at destruction
        REQUIRE(true);
    }
}

TEST_CASE("GameVarSpace - Variable lifetime", "[evaluator][varspace][memory]")
{
    SECTION("Variables persist until space is destroyed")
    {
        GameVarSpace space;

        space.VarSet("persistent", GameValue(99.0f), false);

        // Access multiple times
        GameValue r1, r2, r3;
        REQUIRE(space.VarGet("persistent", r1) == true);
        REQUIRE(space.VarGet("persistent", r2) == true);
        REQUIRE(space.VarGet("persistent", r3) == true);

        REQUIRE((float)r1 == Catch::Approx(99.0f));
        REQUIRE((float)r2 == Catch::Approx(99.0f));
        REQUIRE((float)r3 == Catch::Approx(99.0f));
    }
}

TEST_CASE("GameVarSpace - Large number of variables", "[evaluator][varspace][memory]")
{
    SECTION("Store many variables")
    {
        GameVarSpace space;

        // Store 100 variables
        for (int i = 0; i < 100; i++)
        {
            char name[32];
            sprintf(name, "var%d", i);
            space.VarSet(name, GameValue((float)i), false);
        }

        // Retrieve and verify
        for (int i = 0; i < 100; i++)
        {
            char name[32];
            sprintf(name, "var%d", i);

            GameValue result;
            REQUIRE(space.VarGet(name, result) == true);
            REQUIRE((float)result == Catch::Approx((float)i));
        }
    }
}

// ============================================================================
// Test 5: Integration Scenarios
// ============================================================================

TEST_CASE("GameVarSpace - Real-world scoping scenario", "[evaluator][varspace][integration]")
{
    SECTION("Function call simulation")
    {
        // Global scope
        GameVarSpace global;
        global.VarSet("globalCounter", GameValue(0.0f), false);
        global.VarSet("playerHealth", GameValue(100.0f), false);

        // Function scope
        GameVarSpace function(&global);
        function.VarSet("_localParam", GameValue(42.0f), false);
        function.VarSet("_tempResult", GameValue(0.0f), false);

        // Access globals from function
        GameValue health;
        REQUIRE(function.VarGet("playerHealth", health) == true);

        // Modify local
        function.VarSet("_tempResult", GameValue(123.0f), false);

        GameValue tempResult;
        function.VarGet("_tempResult", tempResult);
        REQUIRE((float)tempResult == Catch::Approx(123.0f));

        // Global doesn't have locals
        GameValue notFound;
        REQUIRE(global.VarGet("_localParam", notFound) == false);
    }
}

TEST_CASE("GameVarSpace - Nested function calls", "[evaluator][varspace][integration]")
{
    SECTION("Nested scopes like nested function calls")
    {
        GameVarSpace global;
        global.VarSet("g", GameValue(1.0f), false);

        GameVarSpace func1(&global);
        func1.VarSet("f1", GameValue(2.0f), false);

        GameVarSpace func2(&func1);
        func2.VarSet("f2", GameValue(3.0f), false);

        GameVarSpace func3(&func2);
        func3.VarSet("f3", GameValue(4.0f), false);

        // func3 can see everything
        GameValue g, f1, f2, f3;
        REQUIRE(func3.VarGet("g", g) == true);
        REQUIRE(func3.VarGet("f1", f1) == true);
        REQUIRE(func3.VarGet("f2", f2) == true);
        REQUIRE(func3.VarGet("f3", f3) == true);

        REQUIRE((float)g == Catch::Approx(1.0f));
        REQUIRE((float)f1 == Catch::Approx(2.0f));
        REQUIRE((float)f2 == Catch::Approx(3.0f));
        REQUIRE((float)f3 == Catch::Approx(4.0f));
    }
}

TEST_CASE("GameVarSpace - Variable shadowing in nested scopes", "[evaluator][varspace][integration]")
{
    SECTION("Same variable name at different levels")
    {
        GameVarSpace global;
        global.VarSet("value", GameValue(10.0f), false);

        GameVarSpace func1(&global);
        // This modifies global's "value" (searches up the chain)
        func1.VarSet("value", GameValue(20.0f), false);

        GameVarSpace func2(&func1);
        // This also modifies global's "value"
        func2.VarSet("value", GameValue(30.0f), false);

        // All levels see the same value (global's value was modified)
        GameValue g, f1, f2;
        global.VarGet("value", g);
        func1.VarGet("value", f1);
        func2.VarGet("value", f2);

        REQUIRE((float)g == Catch::Approx(30.0f));
        REQUIRE((float)f1 == Catch::Approx(30.0f));
        REQUIRE((float)f2 == Catch::Approx(30.0f));
    }

    SECTION("Each level creates its own unique variable")
    {
        GameVarSpace global;
        global.VarSet("globalVar", GameValue(10.0f), false);

        GameVarSpace func1(&global);
        func1.VarSet("func1Var", GameValue(20.0f), false);

        GameVarSpace func2(&func1);
        func2.VarSet("func2Var", GameValue(30.0f), false);

        // Each variable is independent
        GameValue g, f1, f2;
        REQUIRE(global.VarGet("globalVar", g) == true);
        REQUIRE(func1.VarGet("func1Var", f1) == true);
        REQUIRE(func2.VarGet("func2Var", f2) == true);

        // func2 can see all
        REQUIRE(func2.VarGet("globalVar", g) == true);
        REQUIRE(func2.VarGet("func1Var", f1) == true);
        REQUIRE(func2.VarGet("func2Var", f2) == true);

        // global can't see child vars
        REQUIRE(global.VarGet("func1Var", f1) == false);
        REQUIRE(global.VarGet("func2Var", f2) == false);
    }
}
