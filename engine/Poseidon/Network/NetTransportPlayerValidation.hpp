#pragma once

#include <Poseidon/Network/NetTransportProtocol.hpp>

namespace Poseidon
{

ConnectResult ValidateNetTransportCreatePlayerRequest(const SessionPacket& session, const char* serverPassword,
                                                      const CreatePlayerPacket& request);

} // namespace Poseidon
