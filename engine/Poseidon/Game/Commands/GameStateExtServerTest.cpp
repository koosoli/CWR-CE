#include <Evaluator/express.hpp>
#include <Poseidon/Foundation/Logging/Logging.hpp>
#include <Poseidon/Core/Application.hpp>
#include <Poseidon/Network/Network.hpp>
#include <Poseidon/Network/NetworkServerCommon.hpp>
#include <cstdio>
#include <cstdlib>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <Poseidon/Foundation/Framework/Log.hpp>
#include <Poseidon/Foundation/Strings/RString.hpp>

// Server-safe Trident test verbs. These are GL/UI-free (network state + clean exit only),
// so the dedicated server (Core-only, no graphics) can register them via RegisterServerTriCommands
// without dragging in the client-only command module's GL/UI dependencies. The full client
// registers the same handlers from its INIT_MODULE in GameStateExtTestAudio.cpp.

/// triEndTest — request a clean shutdown. Preserves a non-zero exit code from a prior FAIL.
GameValue TriEndTest(const GameState* /*state*/)
{
    if (GApp)
    {
        if (GApp->m_exitCode == 0)
            LOG_INFO(Core, "[tri] triEndTest — requesting clean shutdown (exit code 0)");
        else
            LOG_INFO(Core, "[tri] triEndTest — exit code already {} from prior FAIL, preserving", GApp->m_exitCode);
        GApp->m_closeRequest = true;
    }
    else
    {
        _exit(0);
    }
    return GameValue();
}

/// triNetCommand <command> — run a chat command (e.g. "#login x") through the real
/// NetworkManager::ProcessCommand path, exactly as typing it in the chat box does. Returns "1"
/// if the command was recognized/dispatched, "0" otherwise (and "0" on the dedicated server,
/// which has no client). Lets a test drive the stock #login admin path end to end.
GameValue TriNetCommand(const GameState* /*state*/, GameValuePar arg)
{
    RString command = static_cast<GameStringType>(arg);
    bool ok = false;
    try
    {
        ok = GetNetworkManager().ProcessCommand(command);
    }
    catch (...)
    {
    }
    return GameValue(ok ? "1" : "0");
}

/// Register the server-safe Trident verbs into the global game state. Called explicitly by the
/// dedicated server once its GameState exists; the client registers them via its INIT_MODULE.
void RegisterServerTriCommands()
{
    GGameState.NewNularOp(GameNular(GameNothing, "triEndTest", TriEndTest));
    GGameState.NewFunction(GameFunction(GameString, "triNetCommand", TriNetCommand, GameString));
}
