#include <Poseidon/Core/Config/Configuration.hpp>
#include <Poseidon/Core/Config/EngineConfig.hpp>
#include <Poseidon/Core/RuntimeFlags.hpp>
#include <Poseidon/Foundation/Platform/InitBridge.hpp>
#include <Poseidon/Foundation/Platform/AppConfig.hpp>
#include <Poseidon/Foundation/Common/GamePaths.hpp>
#include <Poseidon/IO/ParamFile/ParamFile.hpp>
#include <string>
#include <Poseidon/Foundation/Strings/RString.hpp>

// Global-scope vars referenced from inside Poseidon namespace
extern bool GUseFileBanks;
extern bool GEnableCaching;
extern Poseidon::ParamFile Pars;

namespace Poseidon
{
RString& GetFlashpointCfgInternal()
{
    static RString instance;
    return instance;
}

void ApplyGamePathsToLegacyGlobals()
{
    const auto& gp = GamePaths::Instance();
    GetFlashpointCfgInternal() = RString(gp.UserDir().c_str()) + RString(gp.CfgName().c_str());
}

// Helper accessors for ConfigurationSystem
ParamClass& GetPars()
{
    return Pars;
}
// GetGStats() is defined in AI/aiCenter.cpp — returns AIStats& (macro in ai.hpp)

void ApplyConfigToGlobConfig(const ConfigurationSystem& config)
{
    // Sync the 3 standalone global variables that exist outside ENGINE_CONFIG.
    const auto& rf = config.GetRuntimeFlags();

    ::netLogValid = config.GetEngineConfig().netLogEnabled;
    ::GUseFileBanks = rf.gUseFileBanks;
    ::GEnableCaching = rf.gEnableCaching;
}
} // namespace Poseidon
