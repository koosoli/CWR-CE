#include <catch2/catch_test_macros.hpp>
#include "eval_fixture.hpp"
#include <string.h>
#include <string>

TEST_CASE("Format - basic substitution", "[evaluator][format]")
{
    EvalFixture f;
    GameValue r = f.state().Evaluate("format [\"%1 + %2 = %3\", 1, 2, 3]");
    REQUIRE(r.GetType() == GameString);
    REQUIRE(strcmp(((GameStringType)r).Data(), "1 + 2 = 3") == 0);
}

TEST_CASE("Format - string args", "[evaluator][format]")
{
    EvalFixture f;
    GameValue r = f.state().Evaluate("format [\"%1=%2\", \"x\", \"y\"]");
    REQUIRE(strcmp(((GameStringType)r).Data(), "x=y") == 0);
}

TEST_CASE("Format - no args", "[evaluator][format]")
{
    EvalFixture f;
    GameValue r = f.state().Evaluate("format [\"hello world\"]");
    REQUIRE(strcmp(((GameStringType)r).Data(), "hello world") == 0);
}

TEST_CASE("Format - mixed types", "[evaluator][format]")
{
    EvalFixture f;
    GameValue r = f.state().Evaluate("format [\"%1 is %2\", \"answer\", 42]");
    REQUIRE(strcmp(((GameStringType)r).Data(), "answer is 42") == 0);
}

TEST_CASE("Format - with hint", "[evaluator][format]")
{
    EvalFixture f;
    f.state().EvaluateMultiple("hint format [\"%1=%2\", 1, 2]");
    REQUIRE(f.state().getOutput() == "[HINT] 1=2\n");
}
