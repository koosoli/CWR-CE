// Stub implementations for Poseidon dependencies not needed in tests

#include <catch2/reporters/catch_reporter_event_listener.hpp>
#include <catch2/reporters/catch_reporter_registrars.hpp>

#include <Poseidon/IO/FileServer.hpp>
#include <Poseidon/Graphics/Textures/TextureBank.hpp>
#include <Poseidon/Graphics/Dummy/EngineDummy.hpp>
#include <Poseidon/Graphics/Core/Engine.hpp>
#include <Poseidon/Graphics/Core/EngineFactory.hpp>
#include <limits.h>
#include <catch2/catch_section_info.hpp>
#include <Poseidon/Foundation/Types/Pointers.hpp>

using Poseidon::EngineDummy;
using Poseidon::GEngine;

// Wrapper to alias DrawTextA to DrawText (DirectX macro causes name mismatch)
// The mangled name must match what the linker is looking for
#pragma comment( \
    linker,      \
    "/alternatename:?DrawTextA@Engine@@QAEXABUPoint2DFloat@@MPAVFont@@VPackedColor@@PBD@Z=?DrawText@Engine@@QAEXABUPoint2DFloat@@MPAVFont@@VPackedColor@@PBD@Z")

// Minimal FileServer stub for testing
// Bypasses the game's virtual file system and uses direct file I/O
class TestFileServer : public FileServer
{
  public:
    void Open(QIFStream& stream, const char* path) override { stream.open(path); }

    void Request(const char*, float, int = 0, int = INT_MAX) override {}
    void CancelRequest(const char*, int = 0, int = INT_MAX) override {}
    void Start() override {}
    void Stop() override {}
    void FlushBank(QFBank*) override {}
};

// Initialize minimal test subsystems
// Called automatically before tests run
struct TestSubsystemsInitializer
{
    EngineDummy* dummyEngine;

    TestSubsystemsInitializer() : dummyEngine(nullptr)
    {
        // Done lazily in ensureInitialized()
    }

    void ensureInitialized()
    {
        // Initialize GFileServer if not already set
        if (GFileServer.IsNull())
        {
            GFileServer = new TestFileServer();
        }

        // Initialize GEngine with dummy implementation for texture loading
        if (!GEngine)
        {
            dummyEngine = new EngineDummy();
            GEngine = dummyEngine;
        }
    }

    ~TestSubsystemsInitializer()
    {
        // Cleanup in reverse order
        if (GEngine == dummyEngine)
        {
            GEngine = nullptr;
            delete dummyEngine;
        }
    }
} g_testInit;

// Hook into Catch2's global setup
// This ensures initialization happens AFTER all static globals are zero-initialized
// but BEFORE any test runs
struct Catch2GlobalSetup : Catch::EventListenerBase
{
    using EventListenerBase::EventListenerBase;

    void testRunStarting(Catch::TestRunInfo const&) override { g_testInit.ensureInitialized(); }
};

CATCH_REGISTER_LISTENER(Catch2GlobalSetup)

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
