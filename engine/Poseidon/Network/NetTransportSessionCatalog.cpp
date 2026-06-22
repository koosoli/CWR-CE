#include <Poseidon/Network/NetTransportSessionCatalog.hpp>

#include <cstdio>
#include <cstring>
#include <Poseidon/Foundation/Strings/RString.hpp>

namespace Poseidon
{

int NetTransportSessionCatalog::Add()
{
    if (_size >= MaxNetTransportSessions)
    {
        return -1;
    }

    return _size++;
}

void NetTransportSessionCatalog::Delete(int index)
{
    _size--;
    for (int i = index; i < _size; ++i)
    {
        _data[i] = _data[i + 1];
    }
}

void NetTransportSessionCatalog::Clear()
{
    _size = 0;
}

int NetTransportSessionCatalog::FindByEndpoint(unsigned32 ip, unsigned16 port) const
{
    for (int i = 0; i < _size; ++i)
    {
        if (_data[i].ip == ip && _data[i].port == port)
        {
            return i;
        }
    }

    return -1;
}

bool NetTransportSessionCatalog::HasRecentPortCandidate(unsigned32 ip, unsigned16 port, unsigned16 portInterval,
                                                        int portsToTry, unsigned32 nowSeconds,
                                                        unsigned32 minRetrySeconds) const
{
    for (int i = 0; i < _size; ++i)
    {
        const NetSessionDescription& session = _data[i];
        if (session.ip == ip && session.port >= port && session.port < port + portsToTry * portInterval &&
            (session.port - port) % portInterval == 0)
        {
            return (nowSeconds - session.lastTime) < minRetrySeconds;
        }
    }

    return false;
}

void NetTransportSessionCatalog::RemoveExpired(unsigned32 nowSeconds, unsigned32 maxAgeSeconds)
{
    for (int i = 0; i < _size;)
    {
        const unsigned32 age = nowSeconds - _data[i].lastTime;
        if (age > maxAgeSeconds)
        {
            Delete(i);
        }
        else
        {
            ++i;
        }
    }
}

void NetTransportSessionCatalog::ExportSessions(AutoArray<SessionInfo>& sessions, unsigned32 nowSeconds,
                                                DWORD nowTickCount) const
{
    sessions.Resize(_size);
    for (int i = 0; i < _size; ++i)
    {
        const NetSessionDescription& src = _data[i];
        SessionInfo& dst = sessions[i];
        dst.guid = src.address;
        dst.name = src.name;
        dst.lastTime = nowTickCount - (nowSeconds - src.lastTime);
        dst.password = (src.password != 0);
        dst.actualVersion = src.actualVersion;
        dst.requiredVersion = src.requiredVersion;
        dst.badActualVersion = false;
        dst.badRequiredVersion = false;
        dst.gameState = src.gameState;
        dst.mission = src.mission;
        dst.numPlayers = src.numPlayers;
        dst.maxPlayers = src.maxPlayers;
        dst.ping = src.pingTime;
        dst.timeleft = 0;
        dst.mod = src.mod;
        dst.versionTag = src.versionTag;
        dst.equalModRequired = (src.equalModRequired & 1) != 0;
        dst.badMod = false;
    }
}

int NetTransportSessionCatalog::UpsertFromResponse(unsigned32 ip, unsigned16 port, const char* address,
                                                   const SessionPacket& packet, const char* mod, const char* versionTag,
                                                   bool equalModRequired, unsigned32 nowSeconds, unsigned pingTime)
{
    int index = FindByEndpoint(ip, port);
    if (index < 0)
    {
        index = Add();
        if (index < 0)
        {
            return -1;
        }

        NetSessionDescription& created = _data[index];
        created.ip = ip;
        created.port = port;
        created.pingTime = pingTime;
        std::snprintf(created.address, sizeof(created.address), "%s:%u", address, static_cast<unsigned>(port));
    }

    NetSessionDescription& session = _data[index];
    std::snprintf(session.name, sizeof(session.name), packet.name, session.address);
    session.actualVersion = packet.actualVersion;
    session.requiredVersion = packet.requiredVersion;
    std::strncpy(session.mission, packet.mission, LEN_MISSION_NAME);
    session.mission[LEN_MISSION_NAME - 1] = 0;
    std::strncpy(session.mod, mod, MOD_LENGTH);
    session.mod[MOD_LENGTH - 1] = 0;
    std::strncpy(session.versionTag, versionTag, VERSION_TAG_LENGTH);
    session.versionTag[VERSION_TAG_LENGTH - 1] = 0;
    session.equalModRequired = equalModRequired ? 1 : 0;
    session.gameState = packet.gameState;
    session.maxPlayers = packet.maxPlayers;
    session.numPlayers = packet.numPlayers;
    session.password = packet.password;
    session.lastTime = nowSeconds;
    session.pingTime = (3 * session.pingTime + pingTime + 2) >> 2;
    return index;
}

} // namespace Poseidon
