#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Core/FSM/Fsm.hpp>
#include <string.h>

// Test context tracking state transitions
struct TestContext
{
    int initCount = 0;
    int checkCount = 0;
    int lastInitState = -1;
    Poseidon::FSM* fsm = nullptr;
};

static void stateAInit(TestContext* ctx)
{
    ctx->initCount++;
    ctx->lastInitState = 0;
}
static void stateACheck(TestContext* ctx)
{
    ctx->checkCount++;
    ctx->fsm->SetState(1, ctx);
}
static void stateBInit(TestContext* ctx)
{
    ctx->initCount++;
    ctx->lastInitState = 1;
}
static void stateBCheck(TestContext* ctx)
{
    ctx->checkCount++;
    ctx->fsm->SetState(Poseidon::FSM::FinalState);
}

using TestFSM = Poseidon::FSMTyped<TestContext>;
static const TestFSM::StateInfo testStates[] = {
    {"StateA", stateAInit, stateACheck},
    {"StateB", stateBInit, stateBCheck},
};

TEST_CASE("FSM: default constructor starts in FinalState", "[FSM]")
{
    Poseidon::FSM fsm;
    REQUIRE(fsm.GetState() == Poseidon::FSM::FinalState);
}

TEST_CASE("FSM: 2-state transition", "[FSM]")
{
    TestFSM fsm(testStates, 2);
    TestContext ctx;
    ctx.fsm = &fsm;

    // Start in state A
    fsm.SetState(0, &ctx);
    REQUIRE(fsm.GetState() == 0);
    REQUIRE(ctx.initCount == 1);
    REQUIRE(ctx.lastInitState == 0);
    REQUIRE(strcmp(fsm.GetStateName(), "StateA") == 0);

    // Update: stateACheck transitions to B
    bool done = fsm.Update(&ctx);
    REQUIRE_FALSE(done);
    REQUIRE(fsm.GetState() == 1);
    REQUIRE(ctx.initCount == 2);
    REQUIRE(ctx.lastInitState == 1);
    REQUIRE(strcmp(fsm.GetStateName(), "StateB") == 0);

    // Update: stateBCheck transitions to FinalState
    done = fsm.Update(&ctx);
    REQUIRE(done);
    REQUIRE(fsm.GetState() == Poseidon::FSM::FinalState);
}

TEST_CASE("FSM: variables", "[FSM]")
{
    TestFSM fsm(testStates, 2);
    fsm.Var(0) = 42;
    fsm.Var(7) = 99;
    REQUIRE(fsm.Var(0) == 42);
    REQUIRE(fsm.Var(7) == 99);
}
