#pragma once

#include <Poseidon/Network/NetTransportProtocol.hpp>

#include <utility>

namespace Poseidon
{

void ResetNetTransportSessionPacket(SessionPacket& session);
void InitializeNetTransportSessionPacket(SessionPacket& session, const MPVersionInfo& versionInfo,
                                         bool equalModRequired, int maxPlayers, bool passwordProtected,
                                         unsigned16 sessionPort, const char* sessionName);
void UpdateNetTransportSessionPacketDescription(SessionPacket& session, int state, const char* mission);
void ApplyNetTransportSessionPlayerDelta(SessionPacket& session, int delta);

template <class EnterFn, class LeaveFn>
void UpdateNetTransportSessionPacketDescriptionLocked(SessionPacket& session, int state, const char* mission,
                                                      EnterFn&& enterLock, LeaveFn&& leaveLock)
{
    std::forward<EnterFn>(enterLock)();
    UpdateNetTransportSessionPacketDescription(session, state, mission);
    std::forward<LeaveFn>(leaveLock)();
}

} // namespace Poseidon
