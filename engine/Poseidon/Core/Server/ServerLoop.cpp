#include <Poseidon/Core/Config/EngineConfig.hpp>
#include <Poseidon/Core/Application.hpp>
#include <Poseidon/Core/Config/Config.hpp>
#include <Poseidon/Core/Global.hpp>
#include <Poseidon/Input/InputSubsystem.hpp>
#include <Poseidon/World/World.hpp>
#include <Poseidon/Foundation/Common/Win.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_mouse.h>
#include <stdlib.h>
#include <Poseidon/Foundation/Common/FltOpts.hpp>
#include <Poseidon/Foundation/Framework/AppFrame.hpp>
#include <Poseidon/Foundation/Framework/Log.hpp>
#include <Poseidon/Foundation/Strings/RString.hpp>
#include <Poseidon/Foundation/Types/Memtype.h>
#include <Poseidon/Foundation/platform.hpp>

// Server configuration globals

namespace Poseidon
{
RString ServerConfig;
RString GetServerConfig()
{
    return ServerConfig;
}
void SetServerConfig(const RString& config)
{
    ServerConfig = config;
}

RString PidFileName;
RString GetPidFileName()
{
    return PidFileName;
}
void SetPidFileName(const RString& filename)
{
    PidFileName = filename;
}

RString RankingLog;
RString GetRankingLog()
{
    return RankingLog;
}
void SetRankingLog(const RString& log)
{
    RankingLog = log;
}

bool MyPidFile = false;
bool GetMyPidFile()
{
    return MyPidFile;
}
void SetMyPidFile(bool value)
{
    MyPidFile = value;
}

// GApp->m_canRender accessor - uses macro from appGlobalsShim.hpp
bool GetCanRender()
{
    return GApp->m_canRender;
}
void SetCanRender(bool value)
{
    GApp->m_canRender = value;
}

// Forward declarations for global-scope functions
} // namespace Poseidon
#include <Poseidon/Core/Game/GameLoop.hpp>
extern void ProcessKeyboard(DWORD sysTime, DWORD timeDelta);
namespace Poseidon
{

bool AppServerLoop()
{
    bool enableDraw = false;

    static DWORD lastTime;
    DWORD actTime = ::GlobalTickCount();
    DWORD deltaTMs = actTime - lastTime;

    float deltaT = deltaTMs * 0.001;
    lastTime = actTime;

    static DWORD lastSysTime;
    DWORD sysTime = ::GetTickCount();
    DWORD timeDelta = sysTime - lastSysTime;
    lastSysTime = sysTime;
    saturateMin(deltaT, 0.3);

    bool focused = GApp->m_appActive && !GApp->m_appPaused && !GApp->m_appIconic;

    if (focused)
    {
        ProcessKeyboard(sysTime, timeDelta);
    }

    if (GApp->m_canRender)
    {
        if (GWorld)
            GWorld->UpdateInputContext();
        InputSubsystem::Instance().Update();

        Poseidon::RenderFrame(deltaT, enableDraw);
        return false;
    }

    return true;
}

} // namespace Poseidon
extern void DDTerm();
namespace Poseidon
{

void DedicatedServerLoop()
{
    while (true)
    {
        AppServerLoop();

        if (GApp->m_validateQuit)
        {
            DDTerm();
            if (ENGINE_CONFIG.hideCursor)
            {
                SDL_ShowCursor();
            }
            int ret = 0;
            LOG_DEBUG(Core, "Exit {}", ret);
            Sleep(500);
            exit(ret);
        }
        Sleep(1); // yield
    }
}
} // namespace Poseidon
