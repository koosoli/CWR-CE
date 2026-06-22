#pragma once

#include <Poseidon/Core/Application.hpp>

// Boot code shared by the CWA game binaries: the full client (PoseidonGame) and the
// dedicated server (PoseidonServer). Holds the lifecycle steps that are identical across
// both; app-specific steps stay as virtual hooks the concrete apps fill in.
class GameBase : public Poseidon::Application
{
  public:
    GameBase(Poseidon::Application::Type type) : Poseidon::Application(type) {}

  protected:
    bool InitializeMemorySystem() override;
    bool ParseCommandLine(const char* commandLine) override;
    bool ReadConfiguration() override;

    // Shared engine-core bring-up: file banks → Glob_Init → InitMan → InitFPU, with the
    // app deltas supplied via the ordered hooks below (each preserves the original boot
    // order). Re-runnable — the client mod re-mount calls it again, so the hooks must be
    // idempotent.
    bool InitializeEngineCore();

    virtual void OnPreEngineInit() {}      // before file banks (server: mimalloc tuning)
    virtual void ConfigureBankMerge() = 0; // texture-merge / HWTL bank-prefix policy
    virtual void OnGlobInitialized() {}    // after Glob_Init (player name / difficulties)
    virtual void OnManagersInit() {}       // after InitMan (AI tables, stats)

    // Common Run() entry prefix: s_instance, game modules, memory, audio backends,
    // command-line. Returns an exit code to propagate, or -1 to continue the
    // app-specific boot. Both apps' platform entry points funnel through this.
    int RunBootstrap(int argc, char** argv, const char* commandLine);

    virtual void RegisterGameModules() {}   // app module set (client full / server none)
    virtual void RegisterAudioBackends() {} // client real / server dummy + text

    // Logging domain passed to LoggingSystem::InitializeFromConfig — "app" (client) / "srv" (server).
    virtual const char* LogDomain() const = 0;

    // Command line captured by the app's Run() for the shared parse path.
    const char* m_commandLine = nullptr;
    int m_argc = 0;
    char** m_argv = nullptr;
    int m_startupExitCode = 0;
};
