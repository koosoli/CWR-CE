#pragma once

#include <Poseidon/Network/NetTransportProtocol.hpp>

namespace Poseidon
{

AckPlayerPacket BuildAckPlayerPacket(ConnectResult result, int32 playerNo);
ReconnectPlayerPacket BuildReconnectPlayerPacket(int32 playerNo);
CreatePlayerPacket BuildCreatePlayerPacket(int32 magicApplication, const char* playerName, const char* password,
                                           const MPVersionInfo& versionInfo, bool botClient, int32 uniqueId);

} // namespace Poseidon
