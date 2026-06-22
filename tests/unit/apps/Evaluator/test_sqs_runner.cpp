// SQS runner tests
#include <catch2/catch_test_macros.hpp>
#include "eval_fixture.hpp"
#include <Evaluator/SqsRunner.hpp>
#include <string>
#include <vector>

TEST_CASE("SQS: empty lines and comments are skipped", "[sqs]")
{
    SqsRunner runner("test");
    runner.parse("; this is a comment\n\n; another comment\n");
    REQUIRE(runner.lines().empty());
}

TEST_CASE("SQS: labels are parsed", "[sqs]")
{
    SqsRunner runner("test");
    runner.parse("#start\nhint \"hello\"\n#end\nexit\n");
    REQUIRE(runner.labels().size() == 2);
    REQUIRE(runner.labels()[0].name == "start");
    REQUIRE(runner.labels()[0].line == 0);
    REQUIRE(runner.labels()[1].name == "end");
    REQUIRE(runner.labels()[1].line == 1);
}

TEST_CASE("SQS: simple statement is parsed", "[sqs]")
{
    SqsRunner runner("test");
    runner.parse("hint \"hello\"\n");
    REQUIRE(runner.lines().size() == 1);
    REQUIRE(runner.lines()[0].statement == "hint \"hello\"");
    REQUIRE(runner.lines()[0].condition.empty());
    REQUIRE(runner.lines()[0].waitUntil.empty());
}

TEST_CASE("SQS: conditional line is parsed", "[sqs]")
{
    SqsRunner runner("test");
    runner.parse("? _x > 5 : hint \"big\"\n");
    REQUIRE(runner.lines().size() == 1);
    REQUIRE(runner.lines()[0].condition == "_x > 5 ");
    REQUIRE(runner.lines()[0].statement == "hint \"big\"");
}

TEST_CASE("SQS: wait condition is parsed", "[sqs]")
{
    SqsRunner runner("test");
    runner.parse("@alive player\n");
    REQUIRE(runner.lines().size() == 1);
    REQUIRE(runner.lines()[0].waitUntil == "alive player");
}

TEST_CASE("SQS: delay is parsed into two lines", "[sqs]")
{
    SqsRunner runner("test");
    runner.parse("~5\n");
    REQUIRE(runner.lines().size() == 2);
    REQUIRE(runner.lines()[0].statement.find("__waituntil") != std::string::npos);
    REQUIRE(runner.lines()[1].suspendUntil == "__waituntil");
}

TEST_CASE("SQS: fork line is parsed", "[sqs]")
{
    SqsRunner runner("test");
    runner.parse("&10\n");
    REQUIRE(runner.lines().size() == 1);
    REQUIRE(runner.lines()[0].suspendUntil == "10");
}

TEST_CASE("SQS: simple script executes", "[sqs]")
{
    EvalFixture f;
    SqsRunner runner("test");
    runner.parse("hint \"hello from sqs\"\nexit\n");
    f.state().clearOutput();
    int ret = runner.run(f.state(), GameValue());
    REQUIRE(ret == 0);
    REQUIRE(f.state().getOutput() == "[HINT] hello from sqs\n");
}

TEST_CASE("SQS: goto and labels work", "[sqs]")
{
    EvalFixture f;
    SqsRunner runner("test");
    runner.parse("goto \"skip\"\n"
                 "hint \"should not print\"\n"
                 "#skip\n"
                 "hint \"after skip\"\n"
                 "exit\n");
    f.state().clearOutput();
    int ret = runner.run(f.state(), GameValue());
    REQUIRE(ret == 0);
    REQUIRE(f.state().getOutput() == "[HINT] after skip\n");
}

TEST_CASE("SQS: conditional execution", "[sqs]")
{
    EvalFixture f;
    SqsRunner runner("test");
    runner.parse("_x = 10\n"
                 "? _x > 5 : hint \"big\"\n"
                 "? _x < 5 : hint \"small\"\n"
                 "exit\n");
    f.state().clearOutput();
    int ret = runner.run(f.state(), GameValue());
    REQUIRE(ret == 0);
    REQUIRE(f.state().getOutput() == "[HINT] big\n");
}

TEST_CASE("SQS: conditional goto", "[sqs]")
{
    EvalFixture f;
    SqsRunner runner("test");
    runner.parse("_x = 3\n"
                 "? _x < 5 : goto \"less\"\n"
                 "hint \"not less\"\n"
                 "goto \"done\"\n"
                 "#less\n"
                 "hint \"less than 5\"\n"
                 "#done\n"
                 "exit\n");
    f.state().clearOutput();
    int ret = runner.run(f.state(), GameValue());
    REQUIRE(ret == 0);
    REQUIRE(f.state().getOutput() == "[HINT] less than 5\n");
}

TEST_CASE("SQS: _this parameter is available", "[sqs]")
{
    EvalFixture f;
    SqsRunner runner("test");
    runner.parse("hint str _this\nexit\n");
    f.state().clearOutput();
    GameArrayType args;
    args.Add(GameValue(42.0f));
    int ret = runner.run(f.state(), GameValue(args));
    REQUIRE(ret == 0);
    REQUIRE(f.state().getOutput().find("42") != std::string::npos);
}

TEST_CASE("SQS: delay is skipped in non-realtime", "[sqs]")
{
    EvalFixture f;
    SqsRunner runner("test", false);
    runner.parse("~5\n"
                 "hint \"after delay\"\n"
                 "exit\n");
    f.state().clearOutput();
    int ret = runner.run(f.state(), GameValue());
    REQUIRE(ret == 0);
    REQUIRE(f.state().getOutput() == "[HINT] after delay\n");
}

TEST_CASE("SQS: wait condition skipped in non-realtime", "[sqs]")
{
    EvalFixture f;
    SqsRunner runner("test", false);
    runner.parse("@true\n"
                 "hint \"after wait\"\n"
                 "exit\n");
    f.state().clearOutput();
    int ret = runner.run(f.state(), GameValue());
    REQUIRE(ret == 0);
    REQUIRE(f.state().getOutput() == "[HINT] after wait\n");
}

TEST_CASE("SQS: loop with counter and goto", "[sqs]")
{
    EvalFixture f;
    SqsRunner runner("test");
    runner.parse("_i = 0\n"
                 "#loop\n"
                 "_i = _i + 1\n"
                 "? _i >= 3 : goto \"done\"\n"
                 "goto \"loop\"\n"
                 "#done\n"
                 "hint format [\"count: %1\", _i]\n"
                 "exit\n");
    f.state().clearOutput();
    int ret = runner.run(f.state(), GameValue());
    REQUIRE(ret == 0);
    REQUIRE(f.state().getOutput() == "[HINT] count: 3\n");
}

TEST_CASE("SQS: addon undefined-global sentinel initializes call-built arrays", "[sqs][compat]")
{
    EvalFixture f;
    SqsRunner runner("heli_dust");
    runner.parse("_null = \"scalar bool array string 0xfcffffef\"\n"
                 "?format[\"%1\",HDSIN ] == _null: HDSIN = call {private[\"_i\",\"_result\"]; _result = []; _i = 0; "
                 "while \"_i < 3\" do {_result = _result + [sin (7.2*_i)*15]; _i = _i+1;}; _result}\n"
                 "hint format [\"%1\", HDSIN select 0]\n"
                 "exit\n");

    f.state().clearOutput();
    const int ret = runner.run(f.state(), GameValue());
    REQUIRE(ret == 0);
    CHECK(f.state().getOutput() == "[HINT] 0\n");
}
