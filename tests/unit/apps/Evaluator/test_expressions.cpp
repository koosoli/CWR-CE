#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "eval_fixture.hpp"
#include <string.h>

TEST_CASE("Eval - arithmetic", "[evaluator][expressions]")
{
    EvalFixture f;

    SECTION("addition")
    {
        GameValue r = f.state().Evaluate("2+3");
        REQUIRE(r.GetType() == GameScalar);
        REQUIRE((float)r == Catch::Approx(5.0f));
    }
    SECTION("subtraction")
    {
        GameValue r = f.state().Evaluate("10-7");
        REQUIRE((float)r == Catch::Approx(3.0f));
    }
    SECTION("multiplication")
    {
        GameValue r = f.state().Evaluate("4*5");
        REQUIRE((float)r == Catch::Approx(20.0f));
    }
    SECTION("division")
    {
        GameValue r = f.state().Evaluate("15/3");
        REQUIRE((float)r == Catch::Approx(5.0f));
    }
    SECTION("modulo")
    {
        GameValue r = f.state().Evaluate("17 % 5");
        REQUIRE((float)r == Catch::Approx(2.0f));
    }
    SECTION("complex expression")
    {
        GameValue r = f.state().Evaluate("(2+3)*4-1");
        REQUIRE((float)r == Catch::Approx(19.0f));
    }
}

TEST_CASE("Eval - string concat", "[evaluator][expressions]")
{
    EvalFixture f;
    GameValue r = f.state().Evaluate("\"hello\" + \" \" + \"world\"");
    REQUIRE(r.GetType() == GameString);
    REQUIRE(strcmp(((GameStringType)r).Data(), "hello world") == 0);
}

TEST_CASE("Eval - array ops", "[evaluator][expressions]")
{
    EvalFixture f;

    SECTION("array creation")
    {
        GameValue r = f.state().Evaluate("[1,2,3]");
        REQUIRE(r.GetType() == GameArray);
        const GameArrayType& arr = r;
        REQUIRE(arr.Size() == 3);
        REQUIRE((float)arr[0] == Catch::Approx(1.0f));
        REQUIRE((float)arr[2] == Catch::Approx(3.0f));
    }
    SECTION("array select")
    {
        GameValue r = f.state().Evaluate("[10,20,30] select 1");
        REQUIRE((float)r == Catch::Approx(20.0f));
    }
    SECTION("array count")
    {
        GameValue r = f.state().Evaluate("count [1,2,3,4]");
        REQUIRE((float)r == Catch::Approx(4.0f));
    }
}

TEST_CASE("Eval - boolean logic", "[evaluator][expressions]")
{
    EvalFixture f;

    SECTION("true and true")
    {
        GameValue r = f.state().Evaluate("true && true");
        REQUIRE((bool)r == true);
    }
    SECTION("true and false")
    {
        GameValue r = f.state().Evaluate("true && false");
        REQUIRE((bool)r == false);
    }
    SECTION("not true")
    {
        GameValue r = f.state().Evaluate("!true");
        REQUIRE((bool)r == false);
    }
    SECTION("or logic")
    {
        GameValue r = f.state().Evaluate("false || true");
        REQUIRE((bool)r == true);
    }
}

TEST_CASE("Eval - comparison", "[evaluator][expressions]")
{
    EvalFixture f;

    SECTION("equal")
    {
        GameValue r = f.state().Evaluate("5 == 5");
        REQUIRE((bool)r == true);
    }
    SECTION("not equal")
    {
        GameValue r = f.state().Evaluate("5 != 3");
        REQUIRE((bool)r == true);
    }
    SECTION("less than")
    {
        GameValue r = f.state().Evaluate("3 < 5");
        REQUIRE((bool)r == true);
    }
    SECTION("greater than")
    {
        GameValue r = f.state().Evaluate("5 > 3");
        REQUIRE((bool)r == true);
    }
}

TEST_CASE("Eval - variable assignment", "[evaluator][expressions]")
{
    EvalFixture f;

    SECTION("global variable")
    {
        f.state().Execute("x = 42");
        GameValue r = f.state().Evaluate("x");
        REQUIRE((float)r == Catch::Approx(42.0f));
    }
}

TEST_CASE("Eval - control flow", "[evaluator][expressions]")
{
    EvalFixture f;

    SECTION("if-then")
    {
        GameValue r = f.state().EvaluateMultiple("if (true) then {1}");
        REQUIRE((float)r == Catch::Approx(1.0f));
    }
    SECTION("if-then-else false branch")
    {
        GameValue r = f.state().EvaluateMultiple("if (false) then {1} else {2}");
        REQUIRE((float)r == Catch::Approx(2.0f));
    }
    SECTION("while loop")
    {
        f.state().Execute("wc = 0");
        f.state().Execute("while {wc < 5} do {wc = wc + 1}");
        GameValue r = f.state().Evaluate("wc");
        REQUIRE((float)r == Catch::Approx(5.0f));
    }
    SECTION("forEach")
    {
        f.state().Execute("s = 0");
        f.state().Execute("{s = s + _x} forEach [1,2,3]");
        GameValue r = f.state().Evaluate("s");
        REQUIRE((float)r == Catch::Approx(6.0f));
    }
}

TEST_CASE("Eval - scoping with private", "[evaluator][expressions]")
{
    EvalFixture f;

    SECTION("private keeps local variable")
    {
        f.state().Execute("pv = 100");
        f.state().EvaluateMultiple("{private \"_pv\"; _pv = 999} call []");
        // _pv should not leak to outer scope
        GameValue r = f.state().Evaluate("pv");
        REQUIRE((float)r == Catch::Approx(100.0f));
    }
}

TEST_CASE("Eval - unary call evaluates code block", "[evaluator][expressions][compat]")
{
    EvalFixture f;

    GameValue r = f.state().EvaluateMultiple("call {1}");
    REQUIRE(r.GetType() == GameScalar);
    CHECK((float)r == Catch::Approx(1.0f));
}

TEST_CASE("Eval - undefined values format with legacy addon sentinel", "[evaluator][expressions][compat]")
{
    EvalFixture f;

    GameValue r = f.state().Evaluate("format[\"%1\", HDSIN]");
    REQUIRE(r.GetType() == GameString);
    CHECK(strcmp(((GameStringType)r).Data(), "scalar bool array string 0xfcffffef") == 0);
}

TEST_CASE("Eval - unary call returns result after private and old while string", "[evaluator][expressions][compat]")
{
    EvalFixture f;

    GameValue r = f.state().EvaluateMultiple("call {private[\"_i\",\"_result\"]; _result = []; _i = 0; while \"_i < "
                                             "3\" do {_result = _result + [_i]; _i = _i+1;}; _result}");
    REQUIRE(r.GetType() == GameArray);
    const GameArrayType& array = r;
    REQUIRE(array.Size() == 3);
    CHECK((float)array[0] == Catch::Approx(0.0f));
    CHECK((float)array[2] == Catch::Approx(2.0f));
}

TEST_CASE("Eval - assignment stores unary call result with old while string", "[evaluator][expressions][compat]")
{
    EvalFixture f;

    f.state().Execute("HDSIN = call {private[\"_i\",\"_result\"]; _result = []; _i = 0; while \"_i < 3\" do {_result = "
                      "_result + [_i]; _i = _i+1;}; _result}");
    GameValue r = f.state().Evaluate("HDSIN");
    REQUIRE(r.GetType() == GameArray);
    const GameArrayType& array = r;
    REQUIRE(array.Size() == 3);
    CHECK((float)array[1] == Catch::Approx(1.0f));
}

TEST_CASE("Eval - nulars", "[evaluator][expressions]")
{
    EvalFixture f;

    SECTION("time returns 0")
    {
        GameValue r = f.state().Evaluate("time");
        REQUIRE((float)r == Catch::Approx(0.0f));
    }
    SECTION("isServer returns true")
    {
        GameValue r = f.state().Evaluate("isServer");
        REQUIRE((bool)r == true);
    }
    SECTION("pi")
    {
        GameValue r = f.state().Evaluate("pi");
        REQUIRE((float)r == Catch::Approx(3.14159f).epsilon(0.001f));
    }
}
