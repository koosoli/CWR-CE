#pragma once

#include <Poseidon/Network/Legacy/netpch.hpp>
#include <Poseidon/Network/NetTransport.hpp>
#include <Poseidon/Network/NetTransportMetrics.hpp>

namespace Poseidon
{

constexpr unsigned64 NetTransportStatLogInterval = 80000;

inline RString FormatNetTransportChannelMetrics(const NetworkTransportMetrics& metrics)
{
    char buf[256];
    const bool kbps = metrics.throughputAverageBps < (9 << 17);
    snprintf(buf, sizeof(buf),
             "ping%4dms(%4u,%4u) BW%c%c%c%5d%cb(%4u,%4u,%4u) lost%4.1f%%%%(%3u) queue%4dB(%4d) ackWait%3u(%3.1f,%3.1f)",
             metrics.latencyAverageMs, metrics.latencyActualMs, metrics.latencyMinimumMs, metrics.throughputGrowMode,
             metrics.throughputPingMode, metrics.throughputLossMode,
             kbps ? ((metrics.throughputAverageBps + 64) >> 7) : ((metrics.throughputAverageBps + 65536) >> 17),
             kbps ? 'K' : 'M', (metrics.throughputActualBps + 64) >> 7, (metrics.throughputGoodBps + 64) >> 7,
             (metrics.throughputSentBps + 64) >> 7,
             metrics.ackTotal ? (metrics.ackLost * 100.0) / metrics.ackTotal : 0.0, metrics.ackLost, metrics.queueBytes,
             metrics.guaranteedQueueBytes, metrics.ackWaitCount, metrics.ackWaitAverageSec, metrics.ackWaitMaxSec);
    return RString(buf);
}

template <class LogFn>
RString FormatNetTransportStatistics(const NetworkTransportMetrics& metrics, bool shouldLog, unsigned64 now,
                                     bool advanceNextStatLog, unsigned64& nextStatLog, int& getStatisticsCount,
                                     LogFn&& logFn)
{
    const RString stats = FormatNetTransportChannelMetrics(metrics);
    if (shouldLog)
    {
        logFn(stats, getStatisticsCount);
        getStatisticsCount = 0;
        if (advanceNextStatLog)
        {
            nextStatLog = now + NetTransportStatLogInterval;
        }
    }
    else
    {
        getStatisticsCount++;
    }
    return stats;
}

} // namespace Poseidon
