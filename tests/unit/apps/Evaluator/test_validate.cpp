// Validation tests
#include <catch2/catch_test_macros.hpp>
#include "eval_fixture.hpp"
#include "test_fixtures.hpp"
#include <Evaluator/Validate.hpp>
#include <catch2/catch_message.hpp>
#include <string>

TEST_CASE("validateBook: extracts and validates code blocks", "[validate]")
{
    EvalFixture f;
    auto sum = validateBook(GET_FIXTURE_DIR("validate_book"), f.state());
    int total = sum.passed + sum.failed + sum.skipped;
    INFO("total=" << total << " passed=" << sum.passed << " failed=" << sum.failed << " skipped=" << sum.skipped);
    REQUIRE(total == 3);
    REQUIRE(sum.passed >= 1);
    REQUIRE(sum.failed >= 1);
    REQUIRE(sum.skipped >= 1);
}

TEST_CASE("validateRef: validates function examples", "[validate]")
{
    EvalFixture f;
    auto sum = validateRef(GET_FIXTURE_DIR("validate_ref"), f.state());
    // hint.json has 2 examples that should pass
    // addEventHandler.json has 1 example that should be skipped
    REQUIRE(sum.passed >= 2);
    REQUIRE(sum.skipped >= 1);
}

TEST_CASE("validateBook: returns empty for nonexistent dir", "[validate]")
{
    EvalFixture f;
    auto sum = validateBook("nonexistent_dir", f.state());
    REQUIRE(sum.passed == 0);
    REQUIRE(sum.failed == 0);
    REQUIRE(sum.skipped == 0);
}

TEST_CASE("validateRef: known function examples inline", "[validate]")
{
    EvalFixture f;
    // Test some examples directly
    f.state().clearOutput();
    f.state().EvaluateMultiple("hint \"test\"");
    REQUIRE(f.state().GetLastError() == EvalOK);

    f.state().clearOutput();
    f.state().EvaluateMultiple("hint format [\"%1 test\", 42]");
    REQUIRE(f.state().GetLastError() == EvalOK);
}
