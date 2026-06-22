#pragma once

// Per-channel transport metrics collected by the NetTransport statistics path.

namespace Poseidon
{

struct NetworkTransportMetrics
{
    int latencyAverageMs = 0;
    int latencyActualMs = 0;
    int latencyMinimumMs = 0;
    int throughputAverageBps = 0;
    int throughputActualBps = 0;
    int throughputGoodBps = 0;
    int throughputSentBps = 0;
    char throughputGrowMode = 0;
    char throughputPingMode = 0;
    char throughputLossMode = 0;
    float ackWaitAverageSec = 0;
    float ackWaitMaxSec = 0;
    int ackWaitCount = 0;
    int ackTotal = 0;
    int ackLost = 0;
    int queueBytes = 0;
    int guaranteedQueueBytes = 0;
};

} // namespace Poseidon
