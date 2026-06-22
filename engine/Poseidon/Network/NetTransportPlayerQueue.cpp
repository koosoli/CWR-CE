#include <Poseidon/Network/NetTransportPlayerQueue.hpp>
#include <string.h>

namespace Poseidon
{

CreatePlayerInfo BuildNetTransportCreatePlayerInfo(int player, const CreatePlayerPacket& request)
{
    CreatePlayerInfo info{};
    info.player = player;
    info.botClient = request.botClient != 0;
    strncpy(info.name, request.name, sizeof(info.name));
    info.name[sizeof(info.name) - 1] = 0;
    strncpy(info.mod, request.mod, sizeof(info.mod));
    info.mod[sizeof(info.mod) - 1] = 0;
    strncpy(info.versionTag, request.versionTag, sizeof(info.versionTag));
    info.versionTag[sizeof(info.versionTag) - 1] = 0;
    return info;
}

bool WasNetTransportCreatePlayerNameTruncated(const CreatePlayerPacket& request, const CreatePlayerInfo& info)
{
    return strcmp(request.name, info.name) != 0;
}

DeletePlayerInfo BuildNetTransportDeletePlayerInfo(int player)
{
    DeletePlayerInfo info{};
    info.player = player;
    return info;
}

} // namespace Poseidon
