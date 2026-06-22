#include "fuzz_init.hpp"

#include <Evaluator/SqsRunner.hpp>

#include <cstddef>
#include <cstdint>
#include <string>

// libFuzzer harness for the SQS script parser (the line-based .sqs format the
// older OFP missions ship). SqsRunner::parse tokenizes the script into lines and
// labels -- the `#label`, `&`/`~`/`@` timing, `?cond:stmt`, and statement forms --
// without executing it (run() is the executor; parse() is pure and stateless, so
// no GameState, no goto loops, no accumulated global vars). A thrown error is
// handled; only a memory fault reaches ASan.

extern "C" int LLVMFuzzerInitialize(int*, char***)
{
    fuzz::InitEngine();
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (size > (64u << 10)) // keep inputs bounded
        return 0;
    std::string content(reinterpret_cast<const char*>(data), size);
    try
    {
        SqsRunner runner("fuzz");
        runner.parse(content);
    }
    catch (...)
    {
    }
    return 0;
}
