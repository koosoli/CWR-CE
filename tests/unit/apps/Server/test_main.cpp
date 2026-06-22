// Catch2 test main entry point for PoseidonServerTests
#define CATCH_CONFIG_RUNNER
#include <catch2/catch_session.hpp>

#include <Poseidon/Foundation/Framework/AppFrame.hpp>
#include <Poseidon/Foundation/Platform/InitBridge.hpp>
#include <stdarg.h>
#include <Poseidon/Foundation/platform.hpp>

#pragma init_seg(compiler)
namespace
{
struct EarlyMemoryInit
{
    EarlyMemoryInit() { SetMemorySystemReady(true); }
} g_earlyMemoryInit;
} // namespace

#include <Poseidon/Core/Application.hpp>
#include <Poseidon/Graphics/Core/Engine.hpp>
#include <Poseidon/Graphics/Dummy/GraphicsEngineDummy.hpp>
#include <Poseidon/IO/ParamFile/InitLibraryElement.hpp>

class TestApplication : public Poseidon::Application
{
  public:
    TestApplication() : Poseidon::Application(Poseidon::Application::DEDICATED_SERVER) {}
    int Run(const char*) override { return 0; }

  protected:
    bool InitializeMemorySystem() override { return true; }
    bool ParseCommandLine(const char*) override { return true; }
    bool ReadConfiguration() override { return true; }
    bool InitializeSubsystems() override { return true; }
    void RunMainLoop() override {}
    void ShutdownSubsystems() override {}
};

class TestAppFrameFunctions : public Poseidon::Foundation::AppFrameFunctions
{
  public:
    void ErrorMessage(Poseidon::Foundation::ErrorMessageLevel, const char*, va_list) override {}
    void ErrorMessage(const char*, va_list) override {}
    void WarningMessage(const char*, va_list) override {}
    void ShowMessage(int, const char*, va_list) override {}
    DWORD TickCount() override { return 0; }
};

static TestAppFrameFunctions testAppFrame;

int main(int argc, char* argv[])
{
    Poseidon::Foundation::CurrentAppFrameFunctions = &testAppFrame;
    SetMemorySystemReady(true);
    InitLibraryElement();
    Poseidon::GEngine = Poseidon::CreateEngineDummy();
    TestApplication testApp;
    return Catch::Session().run(argc, argv);
}
