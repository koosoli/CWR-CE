#include <Poseidon/Network/NetTransportTermination.hpp>
#include <Poseidon/Foundation/Common/Global.hpp>

namespace Poseidon
{

TerminateSessionPacket BuildNetTransportTerminateSessionPacket(NetTerminationReason reason)
{
    TerminateSessionPacket packet{};
    packet.magic = MAGIC_TERMINATE_SESSION;
    packet.reason = static_cast<unsigned32>(reason);
    return packet;
}

NetTerminationReason ParseNetTransportTerminateReason(const void* data, size_t length)
{
    if (data == nullptr || length < sizeof(TerminateSessionPacket))
    {
        return NTROther;
    }

    const auto* packet = static_cast<const TerminateSessionPacket*>(data);
    return static_cast<NetTerminationReason>(packet->reason);
}

void ApplyNetTransportTermination(bool& sessionTerminated, NetTerminationReason& whySessionTerminated,
                                  NetTerminationReason reason)
{
    sessionTerminated = true;
    whySessionTerminated = reason;
}

} // namespace Poseidon
