#pragma once

#include <Poseidon/Network/NetTransport.hpp>
#include <Poseidon/Network/NetTransportProtocol.hpp>

namespace Poseidon
{

TerminateSessionPacket BuildNetTransportTerminateSessionPacket(NetTerminationReason reason);
NetTerminationReason ParseNetTransportTerminateReason(const void* data, size_t length);
void ApplyNetTransportTermination(bool& sessionTerminated, NetTerminationReason& whySessionTerminated,
                                  NetTerminationReason reason);

} // namespace Poseidon
