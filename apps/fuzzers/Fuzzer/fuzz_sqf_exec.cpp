#include "fuzz_init.hpp"

#include <Evaluator/express.hpp>

#include <cstddef>
#include <cstdint>
#include <string>

// libFuzzer harness for the SQF EXECUTE path (the Evaluator), the companion to
// fuzz_sqf's check-only parse. EvaluateMultiple actually runs the script the way
// a radio/trigger/console command does: it applies the operators (array
// select/set/resize, string ops, arithmetic, type coercion), compiles and calls
// code blocks {...}, and drives the dispatch + value stack -- runtime machinery
// the check-only pass never reaches.
//
// SHORT-RUN ONLY. Unlike fuzz_sqf (check-only parse), this actually executes the
// script, and SQF is Turing-complete: a 25-byte input like `while{true}do{x=x+[0]}`
// allocates without bound. libFuzzer's RSS limit is polled, not a hard per-alloc
// cap, so such an input balloons past the ceiling before the abort fires and (no
// fork mode on Windows) can OOM the box. Run it bounded and single-worker, e.g.
//   run-weekend.ps1 -Target sqf_exec -Workers 1 -RssLimitMb 512 -Timeout 3 -Hours 0.02
//
// Triage note: SQF can legitimately loop (while/for) and allocate, so timeouts and
// OOMs are expected resource-exhaustion noise here, not memory-corruption bugs --
// look at crash artifacts. The headless host stubs the world-coupled commands, so a
// fault inside a stub is a test-stub bug, not an engine bug; the real targets are
// the operator/array/string/stack handlers in express.cpp and the GameData types.
//
// localOnly + a fresh per-iteration GameVarSpace: global assignments are rejected
// and the local scope is destroyed each run, so nothing accumulates in the
// GGameState singleton across iterations (otherwise a long campaign exhausts the
// commit charge). The RHS of every statement still executes, so the operator
// machinery is fully exercised.

extern "C" int LLVMFuzzerInitialize(int*, char***)
{
    fuzz::InitEngine();
    GGameState.Init(); // register the default nular/unary/binary operators
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (size > (8u << 10)) // execution amplifies; keep inputs small
        return 0;
    std::string src(reinterpret_cast<const char*>(data), size);
    GameVarSpace local(GGameState.GetContext());
    GGameState.BeginContext(&local);
    try
    {
        GGameState.EvaluateMultiple(src.c_str(), true);
    }
    catch (...)
    {
    }
    GGameState.EndContext(); // always pop, even on a thrown error, so the stack stays sane
    return 0;
}
