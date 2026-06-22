#pragma once

#include "GameBase.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

class ServerApplication : public GameBase
{
  public:
    ServerApplication() : GameBase(Poseidon::Application::DEDICATED_SERVER) {}
    virtual ~ServerApplication();

#ifdef _WIN32
    int Run(HINSTANCE hInstance, LPSTR commandLine, int showCmd);
#endif

    virtual int Run(const char* commandLine) override;

    int Run(int argc, char** argv);

  protected:
    const char* LogDomain() const override { return "srv"; }
    virtual bool InitializeSubsystems() override;
    virtual void RunMainLoop() override;
    virtual void ShutdownSubsystems() override;

  private:
    void OnPreEngineInit() override;
    void ConfigureBankMerge() override;
    void OnGlobInitialized() override;
    void RegisterAudioBackends() override; // dummy + text backends

    // Post-bootstrap server boot (config → engine → world → network → main loop);
    // shared by both Run() entry points. Returns the process exit code.
    int RunServerStages();

    bool InitializeServerEngine(); // delegates to GameBase::InitializeEngineCore
    bool CreateDummyEngine();      // CreateEngineDummy, GEngine
    bool CreateServerConsole();    // Dialog or console
    bool InitializeSound();        // Dummy sound system (needed for weapon init)
    bool InitializeServerWorld();  // World, Landscape, InitWorld

    void DedicatedServerLoop();

#ifdef _WIN32
    HINSTANCE m_hInstance = nullptr;
    int m_showCmd = 0;
#endif
};
