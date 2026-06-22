#pragma once

#include <Poseidon/Network/NetTransport.hpp>
#include <Poseidon/Network/NetTransportProtocol.hpp>

namespace Poseidon
{

CreatePlayerInfo BuildNetTransportCreatePlayerInfo(int player, const CreatePlayerPacket& request);
bool WasNetTransportCreatePlayerNameTruncated(const CreatePlayerPacket& request, const CreatePlayerInfo& info);
DeletePlayerInfo BuildNetTransportDeletePlayerInfo(int player);

} // namespace Poseidon
