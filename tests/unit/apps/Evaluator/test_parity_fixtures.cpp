#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <Evaluator/EvaluatorHost.hpp>
#include "test_fixtures.hpp"
#include <catch2/catch_message.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <string>
#include <vector>

using Catch::Matchers::ContainsSubstring;

namespace
{
struct SqfParityCase
{
    const char* fixture;
    std::vector<std::string> args;
    int exitCode;
    const char* stdoutSubstring;
    const char* stderrSubstring;
};

struct SqsParityCase
{
    const char* fixture;
    std::vector<std::string> args;
    int exitCode;
    const char* stdoutSubstring;
};
} // namespace

TEST_CASE("EvaluatorHost parity fixtures cover representative SQF cases", "[parity][host][sqf]")
{
    static const SqfParityCase cases[] = {
        {"hello.sqf", {}, 0, "[HINT] hello from sqf", ""},     {"clean.sqf", {}, 0, "[HINT] 10 + 20 = 30", ""},
        {"distance.sqf", {}, 0, "[HINT] distance: 5", ""},     {"variables.sqf", {}, 0, "[HINT] action ids:", ""},
        {"this_test.sqf", {"alpha", "bravo"}, 0, "alpha", ""}, {"error.sqf", {}, 1, "", "Error:"},
    };

    for (const auto& parityCase : cases)
    {
        CAPTURE(parityCase.fixture);

        EvaluatorHost host;
        auto result = host.EvaluateSqfFile(GET_FIXTURE(parityCase.fixture), false, parityCase.args);

        REQUIRE(result.exitCode == parityCase.exitCode);
        if (parityCase.stdoutSubstring[0] != '\0')
            REQUIRE_THAT(result.stdoutText, ContainsSubstring(parityCase.stdoutSubstring));
        if (parityCase.stderrSubstring[0] != '\0')
            REQUIRE_THAT(result.stderrText, ContainsSubstring(parityCase.stderrSubstring));
    }
}

TEST_CASE("EvaluatorHost parity fixtures cover representative SQS cases", "[parity][host][sqs]")
{
    static const SqsParityCase cases[] = {
        {"mission_init.sqs", {}, 0, "West group: 3 units"},
        {"delay.sqs", {}, 0, ""},
        {"this_sqs.sqs", {"alpha"}, 0, "[HINT] arg: alpha"},
    };

    for (const auto& parityCase : cases)
    {
        CAPTURE(parityCase.fixture);

        EvaluatorHost host;
        auto result = host.EvaluateSqsFile(GET_FIXTURE(parityCase.fixture), parityCase.args);

        REQUIRE(result.exitCode == parityCase.exitCode);
        if (parityCase.stdoutSubstring[0] != '\0')
            REQUIRE_THAT(result.stdoutText, ContainsSubstring(parityCase.stdoutSubstring));
    }
}
