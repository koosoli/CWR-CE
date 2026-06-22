#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Evaluator/express.hpp>
#include "evaluator_fixture.hpp"
#include <Poseidon/Foundation/Strings/RString.hpp>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

// ============================================================================
// Evaluator testing — reentrancy regression
// ============================================================================
// Background: GameState owns one EvalState (`_e`) used as a shared
// scratchpad by the SQF parser.  Vyhod unconditionally resets
// `_e->SP = 0`, `_e->_pos = expression`, etc. on entry.  That is fine
// for atomic top-level evals, but the engine routinely re-enters the
// evaluator from C++ operator handlers (UI menu condition checks,
// animation rotations, mission scripts, harness eval, …).  Each
// nested Vyhod reset clobbered the outer caller's mid-parse state.
//
// Symptom on production_options_capture: triSimFrames N → AppIdle
// loop runs hundreds of nested evals per second; outer Vyhod's next
// `_e->_operStack[_e->SP - 2]` access reads `_operStack[-2]` (SP was
// 2 before the nested call, 0 after) — undefined behaviour, walking
// into _priorStack memory.  Pre-d91447e7 the off-array bytes were
// zero (BSS singleton), Release()-skipped harmlessly; post-d91447e7
// the heap-allocated GameState had live RString-shaped neighbours
// and the same UB started crashing.
//
// Fix (commit 91b28590): VyhodFrameGuard RAII at every public eval
// entry — Evaluate, EvaluateMultiple, Execute — snapshots `_e` on
// entry and restores it on exit.
//
// These tests register an operator that re-enters the evaluator from
// inside its handler, runs an outer expression that depends on
// surviving state across the re-entry, and asserts the outer result
// is correct.  Without the guard the assertions fail (or the
// interpreter crashes); with the guard they pass.
// ============================================================================

namespace
{
// Counter so tests can verify the reentrant operator was actually
// called.  Reset per test.
int s_reentrantCalls = 0;

// Operator handler: triggers a nested Evaluate, then returns its
// input untouched.  This is the simplest operator that exercises
// the reentrancy path — by the time it returns, _e->SP has been
// reset to 0 by the nested Vyhod (without the guard).
GameValue ReentrantPassthrough(const GameState* state, GameValuePar arg)
{
    ++s_reentrantCalls;
    // Side eval — pretend this is a UI menu condition check or
    // animation rotation matrix.  Discard the result; we only
    // want the side-effect of clobbering _e->SP / _pos / etc.
    const_cast<GameState*>(state)->Evaluate("1 + 1");
    return arg;
}

// Variant that nests deeper.
GameValue ReentrantDouble(const GameState* state, GameValuePar arg)
{
    ++s_reentrantCalls;
    const_cast<GameState*>(state)->Evaluate("2 + 2");
    const_cast<GameState*>(state)->Evaluate("3 + 3 + 3");
    return arg;
}

void RegisterReentrantOps()
{
    static bool registered = false;
    if (registered)
        return;
    registered = true;
    GGameState.NewFunction(GameFunction(GameScalar, "tri_reentrant_passthrough", ReentrantPassthrough, GameScalar));
    GGameState.NewFunction(GameFunction(GameScalar, "tri_reentrant_double", ReentrantDouble, GameScalar));
}
} // namespace

// ============================================================================
// Test 1: Operator handler does one nested Evaluate
// ============================================================================
//
// `5 + tri_reentrant_passthrough 7 + 11`
// = 5 + 7 + 11 = 23
//
// Without the fix: the nested Evaluate("1 + 1") inside the operator
// resets _e->SP to 0; outer's continuation past the operator reads
// _operStack[-2] / _stack[-2] and either crashes or computes a wildly
// wrong value.  With the fix: outer's state is restored, parsing
// resumes correctly, sum is exactly 23.

TEST_CASE("Evaluator reentrancy - single nested call", "[evaluator][reentrancy]")
{
    EvaluatorFixture fixture;
    RegisterReentrantOps();
    s_reentrantCalls = 0;

    GameValue result = GGameState.Evaluate("5 + tri_reentrant_passthrough 7 + 11");

    REQUIRE(s_reentrantCalls == 1);
    REQUIRE(result.GetType() & GameScalar);
    REQUIRE((float)result == Catch::Approx(23.0f));
}

// ============================================================================
// Test 2: Operator handler does multiple nested Evaluates
// ============================================================================
//
// `100 + tri_reentrant_double 50 + 200`
// = 100 + 50 + 200 = 350
//
// The handler does two nested Evaluates back-to-back.  Without the
// fix, both reset SP=0 and the outer collapse logic sees garbage
// twice; the second usually finishes the job (corrupts in a way that
// ends the parse early).  With the fix, both nested calls are fully
// isolated.

TEST_CASE("Evaluator reentrancy - multiple nested calls per operator", "[evaluator][reentrancy]")
{
    EvaluatorFixture fixture;
    RegisterReentrantOps();
    s_reentrantCalls = 0;

    GameValue result = GGameState.Evaluate("100 + tri_reentrant_double 50 + 200");

    REQUIRE(s_reentrantCalls == 1);
    REQUIRE(result.GetType() & GameScalar);
    REQUIRE((float)result == Catch::Approx(350.0f));
}

// ============================================================================
// Test 3: Two reentrant operators in one expression
// ============================================================================
//
// `tri_reentrant_passthrough 1 + tri_reentrant_passthrough 2 +
//  tri_reentrant_passthrough 4 + tri_reentrant_passthrough 8`
// = 1 + 2 + 4 + 8 = 15
//
// Four nested-eval bursts in a single expression.  Each one wipes
// outer's SP to 0; outer must survive all four and return the right
// sum.  This is closer to the production scenario where AppIdle
// triggers many condition evals per frame.

TEST_CASE("Evaluator reentrancy - many nested calls in one expression", "[evaluator][reentrancy]")
{
    EvaluatorFixture fixture;
    RegisterReentrantOps();
    s_reentrantCalls = 0;

    GameValue result = GGameState.Evaluate("tri_reentrant_passthrough 1 + tri_reentrant_passthrough 2 + "
                                           "tri_reentrant_passthrough 4 + tri_reentrant_passthrough 8");

    REQUIRE(s_reentrantCalls == 4);
    REQUIRE(result.GetType() & GameScalar);
    REQUIRE((float)result == Catch::Approx(15.0f));
}

// ============================================================================
// Test 4: Doubly-nested reentrancy (operator → eval → operator → eval)
// ============================================================================
//
// The nested Evaluate's expression itself contains a reentrant
// operator call, so the engine sees a 3-deep eval stack at one
// point.  Without the guard, the middle level corrupts both ends;
// with the guard each level is fully isolated.

namespace
{
GameValue ReentrantDeep(const GameState* state, GameValuePar arg)
{
    ++s_reentrantCalls;
    // This nested Evaluate ITSELF contains a reentrant operator,
    // so we get depth-3 eval stack.
    const_cast<GameState*>(state)->Evaluate("tri_reentrant_passthrough 99");
    return arg;
}
} // namespace

TEST_CASE("Evaluator reentrancy - nested evals inside nested evals", "[evaluator][reentrancy]")
{
    EvaluatorFixture fixture;
    RegisterReentrantOps();
    static bool registered = false;
    if (!registered)
    {
        GGameState.NewFunction(GameFunction(GameScalar, "tri_reentrant_deep", ReentrantDeep, GameScalar));
        registered = true;
    }
    s_reentrantCalls = 0;

    GameValue result = GGameState.Evaluate("1000 + tri_reentrant_deep 500");

    // tri_reentrant_deep increments once; the inner
    // tri_reentrant_passthrough increments once more.
    REQUIRE(s_reentrantCalls == 2);
    REQUIRE(result.GetType() & GameScalar);
    REQUIRE((float)result == Catch::Approx(1500.0f));
}

// ============================================================================
// Test 5: Outer error state survives nested success
// ============================================================================
//
// The guard restores _e->_error on scope exit.  If outer is in an
// EvalOK state and a nested call also succeeds, outer's GetLastError
// stays EvalOK.  More importantly: a successful outer eval should
// report no error even after dozens of nested evals.

TEST_CASE("Evaluator reentrancy - error state is independent across nesting", "[evaluator][reentrancy]")
{
    EvaluatorFixture fixture;
    RegisterReentrantOps();

    // Run a clean expression first to set _error = EvalOK.
    (void)GGameState.Evaluate("1 + 1");
    REQUIRE(GGameState.GetLastError() == EvalOK);

    // Now an expression with reentrant operators — should still report OK.
    GameValue result = GGameState.Evaluate("tri_reentrant_passthrough 10 + tri_reentrant_passthrough 20");

    REQUIRE(GGameState.GetLastError() == EvalOK);
    REQUIRE((float)result == Catch::Approx(30.0f));
}

#pragma clang diagnostic pop
