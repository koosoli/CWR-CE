#pragma once

#include <Poseidon/Network/NetTransportClientHandshake.hpp>
#include <Poseidon/Network/NetTransportTermination.hpp>

namespace Poseidon
{

inline bool TryHandleNetTransportClientControlMessage(unsigned32 magic, const void* data, size_t length, int& ackPlayer,
                                                      int& playerNo, bool& sessionTerminated,
                                                      NetTerminationReason& whySessionTerminated)
{
    switch (magic)
    {
        case MAGIC_ACK_PLAYER:
            if (length != sizeof(AckPlayerPacket))
            {
                return false;
            }
            return TryApplyNetTransportAckPlayer(*static_cast<const AckPlayerPacket*>(data), ackPlayer, playerNo);

        case MAGIC_TERMINATE_SESSION:
            ApplyNetTransportTermination(sessionTerminated, whySessionTerminated,
                                         ParseNetTransportTerminateReason(data, length));
            return true;

        default:
            return false;
    }
}

} // namespace Poseidon
