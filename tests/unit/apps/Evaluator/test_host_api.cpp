#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <Evaluator/EvaluatorHost.hpp>
#include "test_fixtures.hpp"
#include <catch2/matchers/catch_matchers.hpp>
#include <string>

using Catch::Matchers::ContainsSubstring;

TEST_CASE("EvaluatorHost evaluates inline expressions", "[host]")
{
    EvaluatorHost host;

    auto result = host.EvaluateExpression("hint \"hello\"");

    REQUIRE(result.exitCode == 0);
    REQUIRE(result.stderrText.empty());
    REQUIRE_THAT(result.stdoutText, ContainsSubstring("[HINT] hello"));
}

TEST_CASE("EvaluatorHost runs SQF fixtures", "[host]")
{
    EvaluatorHost host;

    auto result = host.EvaluateSqfFile(GET_FIXTURE("utility_functions.sqf"));

    REQUIRE(result.exitCode == 0);
    REQUIRE_THAT(result.stdoutText, ContainsSubstring("Sum of [1..5]: 15"));
}

TEST_CASE("EvaluatorHost runs SQS fixtures", "[host]")
{
    EvaluatorHost host;

    auto result = host.EvaluateSqsFile(GET_FIXTURE("mission_init.sqs"));

    REQUIRE(result.exitCode == 0);
    REQUIRE_THAT(result.stdoutText, ContainsSubstring("West group: 3 units"));
}
