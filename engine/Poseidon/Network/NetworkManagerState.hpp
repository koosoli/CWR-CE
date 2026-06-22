#pragma once

#include <Poseidon/Network/Network.hpp>

namespace Poseidon
{

inline NetworkGameState ResolveNetworkManagerServerState(bool simulateMode, bool hasServer,
                                                         NetworkGameState serverState, bool hasClient,
                                                         NetworkGameState clientServerState)
{
    if (hasServer && simulateMode)
    {
        return serverState;
    }

    if (hasClient)
    {
        return clientServerState;
    }

    return NGSNone;
}

} // namespace Poseidon
