#include <catch2/catch_test_macros.hpp>
#include "eval_fixture.hpp"

TEST_CASE("Error - type mismatch", "[evaluator][errors]")
{
    EvalFixture f;
    f.state().Evaluate("1 + true");
    REQUIRE(f.state().GetLastError() != EvalOK);
}

TEST_CASE("Error - unterminated string", "[evaluator][errors]")
{
    EvalFixture f;
    f.state().Evaluate("\"hello");
    REQUIRE(f.state().GetLastError() != EvalOK);
}

TEST_CASE("Error - missing paren", "[evaluator][errors]")
{
    EvalFixture f;
    f.state().Evaluate("(1 + 2");
    REQUIRE(f.state().GetLastError() != EvalOK);
}

TEST_CASE("Error - clean expression no error", "[evaluator][errors]")
{
    EvalFixture f;
    f.state().EvaluateMultiple("hint \"test\"");
    REQUIRE(f.state().GetLastError() == EvalOK);
}

TEST_CASE("Error - error position available", "[evaluator][errors]")
{
    EvalFixture f;
    f.state().Evaluate("1 +");
    EvalError err = f.state().GetLastError();
    REQUIRE(err != EvalOK);
}
