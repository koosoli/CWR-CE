// Stub implementations for shared Poseidon runtime types used by unit tests.
// This keeps smaller test targets linkable without depending on app binaries.

// Include Catch2 FIRST to avoid macro pollution
#define CATCH_CONFIG_RUNNER
#include <catch2/catch_session.hpp>

#include "test_stubs.hpp"

// Required PoseidonBase symbols
#include <Poseidon/Foundation/platform.hpp>
#include <Poseidon/Foundation/Memory/CheckMem.hpp>
#include <Poseidon/Foundation/Framework/AppFrame.hpp>
#include <spdlog/spdlog.h>

#include <stdarg.h>
#include <cstdlib>

// GMemFunctionsPtr and GSafeMemFunctionsPtr are provided by Poseidon.lib (JimboAllocator.cpp)

// selectany: wins over JimboAllocator's selectany because test objects are linked first
class TestAppFrameFunctions : public AppFrameFunctions
{
};

static TestAppFrameFunctions testAppFrame;
#ifdef _MSC_VER
__declspec(selectany)
#elif defined(__GNUC__)
__attribute__((weak))
#endif
Poseidon::Foundation::AppFrameFunctions* Poseidon::Foundation::CurrentAppFrameFunctions = &testAppFrame;

// StatisticsByName stub implementation
// (Empty implementations - just for linking)

StatisticsByName::StatisticsByName()
{
    // Stub constructor
}

StatisticsByName::~StatisticsByName()
{
    // Stub destructor
}

void StatisticsByName::Count(const char* name, int count)
{
    // Stub - do nothing
    (void)name;
    (void)count;
}

// d5q-PoseidonGL33 stubs: test binaries don't link PoseidonGL33.lib;
// provide null stubs for the two functions tests' includes reference.
namespace Poseidon
{
class Engine;
}
extern "C"
{
} // ensure C++ context
namespace Poseidon
{
class Engine* CreateEngineGL33(int, int, bool, int)
{
    return nullptr;
}
} // namespace Poseidon
namespace Poseidon
{
void RegisterGL33GraphicsBackend() {}
} // namespace Poseidon

// Catch2 main
int main(int argc, char* argv[])
{
#ifdef _WIN32
    _putenv_s("POSEIDON_TEST", "1");
#else
    setenv("POSEIDON_TEST", "1", 1);
#endif
    spdlog::default_logger()->set_level(spdlog::level::off);
    Poseidon::Foundation::CurrentAppFrameFunctions = &testAppFrame;
    return Catch::Session().run(argc, argv);
}
