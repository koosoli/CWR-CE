#include <Poseidon/Network/NetTransportPlayerHandshake.hpp>
#include <string.h>

namespace Poseidon
{

AckPlayerPacket BuildAckPlayerPacket(ConnectResult result, int32 playerNo)
{
    AckPlayerPacket packet{};
    packet.magic = MAGIC_ACK_PLAYER;
    packet.result = result;
    packet.playerNo = playerNo;
    return packet;
}

ReconnectPlayerPacket BuildReconnectPlayerPacket(int32 playerNo)
{
    ReconnectPlayerPacket packet{};
    packet.magic = MAGIC_RECONNECT_PLAYER;
    packet.playerNo = playerNo;
    return packet;
}

CreatePlayerPacket BuildCreatePlayerPacket(int32 magicApplication, const char* playerName, const char* password,
                                           const MPVersionInfo& versionInfo, bool botClient, int32 uniqueId)
{
    CreatePlayerPacket packet{};
    packet.magic = MAGIC_CREATE_PLAYER;
    packet.magicApplication = magicApplication;
    strncpy(packet.name, playerName != nullptr ? playerName : "", LEN_PLAYER_NAME);
    packet.name[LEN_PLAYER_NAME - 1] = 0;
    strncpy(packet.password, password != nullptr ? password : "", LEN_PASSWORD_NAME);
    packet.password[LEN_PASSWORD_NAME - 1] = 0;
    strncpy(packet.mod, versionInfo.mod, MOD_LENGTH);
    packet.mod[MOD_LENGTH - 1] = 0;
    strncpy(packet.versionTag, versionInfo.versionTag, VERSION_TAG_LENGTH);
    packet.versionTag[VERSION_TAG_LENGTH - 1] = 0;
    packet.actualVersion = versionInfo.versionActual;
    packet.requiredVersion = versionInfo.versionRequired;
    packet.botClient = botClient ? 1 : 0;
    packet.uniqueID = uniqueId;
    return packet;
}

} // namespace Poseidon
