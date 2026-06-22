#include <Poseidon/Network/NetTransportClientSession.hpp>

#include <Poseidon/Network/Legacy/netpch.hpp>
#include <Poseidon/Network/NetTransportMessageAge.hpp>
#include <Poseidon/Network/NetTransportTermination.hpp>

namespace Poseidon
{

DestroyPlayerPacket BuildNetTransportDestroyPlayerPacket()
{
    DestroyPlayerPacket packet{};
    packet.magic = MAGIC_DESTROY_PLAYER;
    return packet;
}

void ResetNetTransportClientTerminationState(bool& sessionTerminated, NetTerminationReason& whySessionTerminated)
{
    sessionTerminated = false;
    whySessionTerminated = NTROther;
}

float ReadNetTransportClientLastMsgAge(unsigned64 now, const NetChannel* channel)
{
    if (channel == nullptr)
    {
        return 1.0f;
    }

    return ComputeNetTransportMessageAge(now, channel->getLastMessageArrival(), "NetClient::GetLastMsgAge",
                                         "NetClient::GetLastMsgAge");
}

float ReadNetTransportClientLastMsgAgeReported(unsigned64 now, unsigned64 lastMsgReported)
{
    return 1.e-6f * static_cast<float>(now - lastMsgReported);
}

void RecordNetTransportClientLastMsgAgeReported(unsigned64 now, unsigned64& lastMsgReported)
{
    lastMsgReported = now;
}

bool UpdateNetTransportClientDroppedState(bool amIBot, bool channelDropped, bool& sessionTerminated,
                                          NetTerminationReason& whySessionTerminated)
{
    if (!amIBot && channelDropped)
    {
        ApplyNetTransportTermination(sessionTerminated, whySessionTerminated, NTRTimeout);
    }
    return sessionTerminated;
}

bool ReadNetTransportClientSessionTerminated(bool hasChannel, bool amIBot, bool channelDropped, bool& sessionTerminated,
                                             NetTerminationReason& whySessionTerminated)
{
    if (!hasChannel)
    {
        return true;
    }

    return UpdateNetTransportClientDroppedState(amIBot, channelDropped, sessionTerminated, whySessionTerminated);
}

} // namespace Poseidon
