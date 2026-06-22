#pragma once

#include <Poseidon/Network/NetTransportProtocol.hpp>

namespace Poseidon
{

constexpr unsigned32 NetTransportAckPlayerTimeoutMs = 8000;
constexpr unsigned32 NetTransportCreatePlayerResendMs = 2000;
constexpr unsigned32 NetTransportHandshakePollWaitMs = 100;

unsigned64 ComputeNetTransportHandshakeTimeout(unsigned64 now);
unsigned64 ComputeNetTransportHandshakeNextRetry(unsigned64 now);
double ComputeNetTransportHandshakeElapsedSeconds(unsigned64 now, unsigned64 timeout);
bool ShouldContinueNetTransportHandshakeWait(int ackPlayer, unsigned64 now, unsigned64 timeout);
bool TryApplyNetTransportAckPlayer(const AckPlayerPacket& packet, int& ackPlayer, int& playerNo);
ConnectResult ResolveNetTransportHandshakeResult(int ackPlayer);

} // namespace Poseidon
