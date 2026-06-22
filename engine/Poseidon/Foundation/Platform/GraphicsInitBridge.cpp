#include <Poseidon/Game/Scripting/Scripts.hpp>
#include <Poseidon/Foundation/Platform/InitBridge.hpp>
#include <Poseidon/Foundation/Platform/AppConfig.hpp> // For AppConfig::Instance()
#include <Poseidon/Core/Config/EngineConfig.hpp>
#include <Poseidon/Network/NetworkConfig.hpp>

// Full headers go here, not in InitBridge.hpp, so Core consumers stay light.
#include <Poseidon/AI/AI.hpp>
#include <Poseidon/Core/Global.hpp>
#include <Poseidon/Core/Application.hpp>
#include <Poseidon/World/Scene/Scene.hpp>
#include <Poseidon/World/World.hpp>
#include <Poseidon/World/Terrain/Landscape.hpp>
#include <Poseidon/Graphics/Core/Engine.hpp>
#include <Poseidon/Graphics/GraphicsEngineFactory.hpp> // For GraphicsEngineParams
#include <Poseidon/World/Entities/Vehicles/Vehicle.hpp>
#include <Poseidon/Game/Scripting/Scripts.hpp> // Script class is here
#include <Poseidon/Game/Commands/GameStateExt.hpp>
#include <Poseidon/Dev/Debug/DebugTrap.hpp> // For Debugger class
#include <Poseidon/Graphics/Dummy/GraphicsEngineDummy.hpp>
#include <Poseidon/IO/FileServer.hpp> // For GFileServer
#include <Poseidon/Core/Progress.hpp> // For ProgressStart
#include <Poseidon/UI/Locale/Stringtable/Stringtable.hpp>
#include <Poseidon/IO/Streams/QBStream.hpp> // For GFileBankPrefix
#include <stdint.h>
#include <string>
#include <Poseidon/Foundation/Strings/RString.hpp>
#include <Poseidon/Foundation/Types/Pointers.hpp>
#include <Poseidon/Foundation/platform.hpp>

using namespace Poseidon::Dev;

// Separate symbol from AI::InitTables() to avoid an AI namespace/class conflict.

using namespace Poseidon;
void AI_InitTables()
{
    AI::InitTables();
}

// GStats is a macro, so it needs a free-function wrapper.
void GStats_ClearAll()
{
    GStats.ClearAll();
}

void Glob_Init()
{
    Glob.Init();
}

int Glob_GetWantW()
{
    return ENGINE_CONFIG.wantW;
}
int Glob_GetWantH()
{
    return ENGINE_CONFIG.wantH;
}
int Glob_GetWantBpp()
{
    return ENGINE_CONFIG.wantBpp;
}
void Glob_SetWantW(int w)
{
    ENGINE_CONFIG.wantW = w;
}
void Glob_SetWantH(int h)
{
    ENGINE_CONFIG.wantH = h;
}
void Glob_SetPlayerName(const RString& name)
{
    Glob.header.playerName = name;
}

void GPreloadedTextures_Preload(bool b)
{
    GPreloadedTextures.Preload(b);
}

World* CreateWorld(Engine* engine, bool landEditor)
{
    return new World(engine, landEditor);
}

Landscape* CreateLandscape(Engine* engine, World* world, bool b)
{
    return new Landscape(engine, world, b);
}

void World_InitLandscape(World* world, Landscape* landscape)
{
    world->InitLandscape(landscape);
}

void VehicleTypes_Preload()
{
    VehicleTypes.Preload();
}

int VehicleTypes_AuditEditorVisibleModels()
{
    return VehicleTypes.AuditEditorVisibleModels();
}

void GFileServer_FlushBank()
{
    GFileServer->FlushBank(nullptr);
}

Poseidon::Script* CreateProgressScript(const char* filename)
{
    GameValue val;
    return new Poseidon::Script(filename, val);
}

void SetProgressScript(Poseidon::Script* script)
{
    ::Poseidon::ProgressScript = script;
}

// Localizes stringId before calling ProgressStart.
void ProgressStart_Wrapper(int stringId)
{
    RString str = LocalizeString(stringId);
    ProgressStart(str.Data());
}

int RString_GetLength(const void* rstring)
{
    const RString* str = static_cast<const RString*>(rstring);
    return str->GetLength();
}

const void* ClientIP_GetPtr()
{
    return &ClientIP;
}

void SetGFileBankPrefix(const char* prefix)
{
    GFileBankPrefix = prefix;
}

// HINSTANCE / showCmd are vestigial — kept on the signature for now
// because the call sites still pass them, but neither GL33 nor the
// Dummy backend reads them.
Engine* CreateEngineWithParams(void* hInstance, int showCmd)
{
    if (ENGINE_CONFIG.doCreateDedicatedServer)
    {
        GDebugger.PauseCheckingAlive();
        return CreateEngineDummy();
    }

    GraphicsEngineParams glPars;
    glPars.hInst = (HINSTANCE)(intptr_t)hInstance;
    glPars.hPrev = 0;
    glPars.showCmd = showCmd;
    glPars.width = ENGINE_CONFIG.wantW;
    glPars.height = ENGINE_CONFIG.wantH;
    glPars.bitsPerPixel = ENGINE_CONFIG.wantBpp;
    glPars.useWindow = ENGINE_CONFIG.useWindow;
    glPars.displayMode = ENGINE_CONFIG.displayMode;

    Engine* engine = GApp->CreateGraphicsEngine(glPars);
    if (engine)
    {
        // Apply startup --fps/--show-fps flag after engine creation.
        engine->ToggleFps(ENGINE_CONFIG.showFps);
    }

    // Output parameters (set by the backend during create).
    ENGINE_CONFIG.hideCursor = glPars.hideCursor;
    ENGINE_CONFIG.useWindow = glPars.useWindow;
    ENGINE_CONFIG.noRedrawWindow = glPars.noRedrawWindow;

    return engine;
}
