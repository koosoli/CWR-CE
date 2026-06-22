#include <Poseidon/Network/NetTransportEnumResponse.hpp>
#include <string.h>

namespace Poseidon
{

SessionPacket BuildNetTransportEnumResponse(const SessionPacket& session, unsigned32 magic, MsgSerial request,
                                            int32 numPlayers)
{
    SessionPacket response = session;
    response.magic = magic;
    response.request = request;
    response.numPlayers = numPlayers;
    return response;
}

bool IsNetTransportEnumResponseSize(size_t length)
{
    return length == SESSION_PACKET_2 || length == SESSION_PACKET_3 || length == SESSION_PACKET_4;
}

bool TryParseNetTransportEnumResponse(const void* data, size_t length, NetTransportEnumResponseData& response)
{
    if (!IsNetTransportEnumResponseSize(length) || data == nullptr)
    {
        return false;
    }

    Zero(response.packet);
    memset(response.mod, 0, sizeof(response.mod));
    memset(response.versionTag, 0, sizeof(response.versionTag));
    response.equalModRequired = false;

    memcpy(&response.packet, data, length);
    if (length >= SESSION_PACKET_3)
    {
        strncpy(response.mod, response.packet.mod, sizeof(response.mod));
        response.mod[sizeof(response.mod) - 1] = 0;
        response.equalModRequired = (response.packet.equalModRequired & 1) != 0;
    }
    if (length >= SESSION_PACKET_4)
    {
        strncpy(response.versionTag, response.packet.versionTag, sizeof(response.versionTag));
        response.versionTag[sizeof(response.versionTag) - 1] = 0;
    }
    return true;
}

unsigned ComputeNetTransportEnumPingMs(unsigned64 receiveTime, unsigned64 requestTime)
{
    return requestTime != 0 ? static_cast<unsigned>((receiveTime - requestTime) / 1000) : 0;
}

} // namespace Poseidon
