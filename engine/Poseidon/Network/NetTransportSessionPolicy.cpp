#include <Poseidon/Network/NetTransportSessionPolicy.hpp>
#include <stdio.h>
#include <string.h>

namespace Poseidon
{

bool IsNetTransportSessionFull(int maxPlayers, unsigned userCount)
{
    return maxPlayers > 0 && userCount + 2 > static_cast<unsigned>(maxPlayers);
}

std::string BuildNetTransportSessionName(const char* sessionNameInit, const char* sessionNameFormat,
                                         const char* playerName, const char* hostname)
{
    char buffer[512];
    if (hostname != nullptr && hostname[0] != 0)
    {
        if (playerName != nullptr && playerName[0] != 0)
        {
            snprintf(buffer, sizeof(buffer), sessionNameFormat != nullptr ? sessionNameFormat : "%s", playerName,
                     hostname);
        }
        else
        {
            strncpy(buffer, hostname, sizeof(buffer));
            buffer[sizeof(buffer) - 1] = 0;
        }
    }
    else
    {
        strncpy(buffer, sessionNameInit != nullptr ? sessionNameInit : "", sizeof(buffer));
        buffer[sizeof(buffer) - 1] = 0;
    }

    return buffer;
}

} // namespace Poseidon
