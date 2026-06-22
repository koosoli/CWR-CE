#include <catch2/catch_test_macros.hpp>
#include "eval_fixture.hpp"
#include <string>

TEST_CASE("Stubs - hint", "[evaluator][stubs]")
{
    EvalFixture f;
    f.state().EvaluateMultiple("hint \"hello\"");
    REQUIRE(f.state().getOutput() == "[HINT] hello\n");
}

TEST_CASE("Stubs - hintC", "[evaluator][stubs]")
{
    EvalFixture f;
    f.state().EvaluateMultiple("hintC \"test message\"");
    REQUIRE(f.state().getOutput() == "[HINT] test message\n");
}

TEST_CASE("Stubs - logInfo", "[evaluator][stubs]")
{
    EvalFixture f;
    f.state().EvaluateMultiple("logInfo \"some info\"");
    REQUIRE(f.state().getOutput() == "[LOG] some info\n");
}

TEST_CASE("Stubs - debugLog", "[evaluator][stubs]")
{
    EvalFixture f;
    f.state().EvaluateMultiple("debugLog \"debug msg\"");
    REQUIRE(f.state().getOutput() == "[DEBUG] debug msg\n");
}

TEST_CASE("Stubs - textLog", "[evaluator][stubs]")
{
    EvalFixture f;
    f.state().EvaluateMultiple("textLog \"log msg\"");
    REQUIRE(f.state().getOutput() == "[LOG] log msg\n");
}

TEST_CASE("Stubs - diag_log", "[evaluator][stubs]")
{
    EvalFixture f;
    f.state().EvaluateMultiple("diag_log \"diag msg\"");
    REQUIRE(f.state().getOutput() == "[LOG] diag msg\n");
}
