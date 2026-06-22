// SQS integration tests: exec, call, exit, exitWith
#include <catch2/catch_test_macros.hpp>
#include "eval_fixture.hpp"
#include "test_fixtures.hpp"
#include <Evaluator/SqsRunner.hpp>
#include <string>

TEST_CASE("call executes code block with _this", "[sqs-integration]")
{
    EvalFixture f;
    f.state().clearOutput();
    GameValue result = f.state().EvaluateMultiple("42 call {_this + 1}");
    REQUIRE(f.state().GetLastError() == EvalOK);
    REQUIRE((float)result == 43.0f);
}

TEST_CASE("call with array argument", "[sqs-integration]")
{
    EvalFixture f;
    f.state().clearOutput();
    GameValue result = f.state().EvaluateMultiple("[10, 20] call {(_this select 0) + (_this select 1)}");
    REQUIRE(f.state().GetLastError() == EvalOK);
    REQUIRE((float)result == 30.0f);
}

TEST_CASE("exitWith returns value", "[sqs-integration]")
{
    EvalFixture f;
    f.state().clearOutput();
    GameValue result = f.state().EvaluateMultiple("exitWith 42");
    REQUIRE((float)result == 42.0f);
}

TEST_CASE("exit is recognized as nular", "[sqs-integration]")
{
    EvalFixture f;
    f.state().clearOutput();
    // exit should not crash
    f.state().Execute("exit");
    REQUIRE(f.state().GetLastError() == EvalOK);
}

TEST_CASE("SQS runner via exec from SQF context", "[sqs-integration]")
{
    EvalFixture f;
    f.state().clearOutput();
    // exec expects object exec "file" - we use nil exec "file"
    // This tests the command registration
    f.state().Execute((std::string("[] exec \"") + GET_FIXTURE("loop.sqs") + "\"").c_str());
    // If the file doesn't exist, that's ok - it should not crash
    // The command is registered and callable
    REQUIRE(f.state().GetLastError() == EvalOK);
}
