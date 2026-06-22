#include <Poseidon/Network/NetTransportClientHandshake.hpp>

namespace Poseidon
{

unsigned64 ComputeNetTransportHandshakeTimeout(unsigned64 now)
{
    return now + 1000 * NetTransportAckPlayerTimeoutMs;
}

unsigned64 ComputeNetTransportHandshakeNextRetry(unsigned64 now)
{
    return now + 1000 * NetTransportCreatePlayerResendMs;
}

double ComputeNetTransportHandshakeElapsedSeconds(unsigned64 now, unsigned64 timeout)
{
    return 1.e-6 * (now - (timeout - 1000 * NetTransportAckPlayerTimeoutMs));
}

bool ShouldContinueNetTransportHandshakeWait(int ackPlayer, unsigned64 now, unsigned64 timeout)
{
    return ackPlayer == CRNone && now < timeout;
}

bool TryApplyNetTransportAckPlayer(const AckPlayerPacket& packet, int& ackPlayer, int& playerNo)
{
    if (ackPlayer != CRNone)
    {
        return false;
    }

    playerNo = packet.playerNo;
    ackPlayer = packet.result;
    return true;
}

ConnectResult ResolveNetTransportHandshakeResult(int ackPlayer)
{
    return ackPlayer == CRNone ? CRError : static_cast<ConnectResult>(ackPlayer);
}

} // namespace Poseidon
