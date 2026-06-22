#include <Poseidon/Network/NetTransportSessionPacketState.hpp>
#include <string.h>

namespace Poseidon
{

void ResetNetTransportSessionPacket(SessionPacket& session)
{
    session.actualVersion = 0;
    session.requiredVersion = 0;
    session.gameState = 0;
    session.maxPlayers = 0;
    session.numPlayers = 0;
    session.password = false;
    session.port = 0;
    session.name[0] = 0;
    session.mission[0] = 0;
    session.mod[0] = 0;
    session.versionTag[0] = 0;
    session.equalModRequired = 0;
}

void InitializeNetTransportSessionPacket(SessionPacket& session, const MPVersionInfo& versionInfo,
                                         bool equalModRequired, int maxPlayers, bool passwordProtected,
                                         unsigned16 sessionPort, const char* sessionName)
{
    session.actualVersion = versionInfo.versionActual;
    session.requiredVersion = versionInfo.versionRequired;
    session.equalModRequired = equalModRequired ? 1 : 0;
    session.maxPlayers = maxPlayers;
    session.numPlayers = 0;
    session.password = passwordProtected;
    session.port = sessionPort;
    strncpy(session.name, sessionName != nullptr ? sessionName : "", sizeof(session.name));
    session.name[sizeof(session.name) - 1] = 0;
    strncpy(session.mod, versionInfo.mod, sizeof(session.mod));
    session.mod[sizeof(session.mod) - 1] = 0;
    strncpy(session.versionTag, versionInfo.versionTag, sizeof(session.versionTag));
    session.versionTag[sizeof(session.versionTag) - 1] = 0;
    session.mission[0] = 0;
    session.gameState = 0;
}

void UpdateNetTransportSessionPacketDescription(SessionPacket& session, int state, const char* mission)
{
    session.gameState = state;
    strncpy(session.mission, mission != nullptr ? mission : "", LEN_MISSION_NAME);
    session.mission[LEN_MISSION_NAME - 1] = 0;
}

void ApplyNetTransportSessionPlayerDelta(SessionPacket& session, int delta)
{
    session.numPlayers += delta;
    if (session.numPlayers < 0)
    {
        session.numPlayers = 0;
    }
}

} // namespace Poseidon
