#pragma once

#include <string>

namespace Poseidon
{

bool IsNetTransportSessionFull(int maxPlayers, unsigned userCount);
std::string BuildNetTransportSessionName(const char* sessionNameInit, const char* sessionNameFormat,
                                         const char* playerName, const char* hostname);

} // namespace Poseidon
