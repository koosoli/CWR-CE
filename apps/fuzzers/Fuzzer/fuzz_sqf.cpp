#include "fuzz_init.hpp"

#include <Evaluator/express.hpp>

#include <cstddef>
#include <cstdint>
#include <string>

// libFuzzer harness for the SQF lexer/parser (the Evaluator). Mission scripts
// (init.sqf, triggers, radio commands, the console) are attacker-controlled text
// handed to the GameState parser, so every downloaded mission can reach it.
//
// CheckExecute runs the full multi-statement parser (the `;`-separated Execute
// loop -- tokenizer, assignment scan, recursive-descent expression parser, type
// checker) in check-only mode: operators yield typed nil values instead of
// executing, so there are no side effects and no infinite while/for loops -- this
// isolates parser memory-safety from runtime/timeout noise. A thrown EvalError is
// handled; only a memory fault reaches ASan.

extern "C" int LLVMFuzzerInitialize(int*, char***)
{
    fuzz::InitEngine();
    GGameState.Init(); // register the default nular/unary/binary operators
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (size > (64u << 10)) // the parser caps line length; keep inputs bounded
        return 0;
    std::string src(reinterpret_cast<const char*>(data), size);
    try
    {
        GGameState.CheckExecute(src.c_str());
    }
    catch (...)
    {
    }
    return 0;
}
