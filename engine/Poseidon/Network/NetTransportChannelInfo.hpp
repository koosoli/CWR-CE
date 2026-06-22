#pragma once

#include <Poseidon/Network/Legacy/netpch.hpp>
#include <Poseidon/Network/NetTransport.hpp>
#include <Poseidon/Network/NetTransportMetrics.hpp>

namespace Poseidon
{

inline void ClearNetTransportQueueInfo(int& nMsg, int& nBytes, int& nMsgGuaranteed, int& nBytesGuaranteed)
{
    nMsg = 0;
    nBytes = 0;
    nMsgGuaranteed = 0;
    nBytesGuaranteed = 0;
}

template <class ChannelT>
void ReadNetTransportSendQueueInfo(ChannelT* channel, int& nMsg, int& nBytes, int& nMsgGuaranteed,
                                   int& nBytesGuaranteed)
{
    if (channel == nullptr)
    {
        ClearNetTransportQueueInfo(nMsg, nBytes, nMsgGuaranteed, nBytesGuaranteed);
        return;
    }

    channel->getOutputQueueStatistics(nMsg, nBytes, nMsgGuaranteed, nBytesGuaranteed);
}

template <class ChannelT, class OnChanged>
bool ReadNetTransportConnectionInfo(ChannelT* channel, int& latencyMS, int& throughputBPS, int& oldLatency,
                                    int& oldThroughput, OnChanged&& onChanged)
{
    if (channel == nullptr)
    {
        return false;
    }

    latencyMS = static_cast<int>(channel->getLatency() / 1000);
    throughputBPS = static_cast<int>(channel->getOutputBandWidth());
    if (latencyMS != oldLatency || throughputBPS != oldThroughput)
    {
        onChanged();
        oldLatency = latencyMS;
        oldThroughput = throughputBPS;
    }
    return true;
}

template <class ChannelT>
bool ReadNetTransportConnectionInfo(ChannelT* channel, int& latencyMS, int& throughputBPS)
{
    if (channel == nullptr)
    {
        return false;
    }

    latencyMS = static_cast<int>(channel->getLatency() / 1000);
    throughputBPS = static_cast<int>(channel->getOutputBandWidth());
    return true;
}

template <class ChannelT>
void CollectNetTransportChannelMetrics(ChannelT* channel, NetworkTransportMetrics& metrics)
{
    Zero(metrics);

    unsigned latencyActual = 0;
    unsigned latencyMinimum = 0;
    metrics.latencyAverageMs = static_cast<int>(channel->getLatency(&latencyActual, &latencyMinimum) / 1000);
    metrics.latencyActualMs = latencyActual / 1000;
    metrics.latencyMinimumMs = latencyMinimum / 1000;

    EnhancedBWInfo enhanced{};
    metrics.throughputAverageBps = static_cast<int>(channel->getOutputBandWidth(&enhanced));
    metrics.throughputActualBps = enhanced.actBW;
    metrics.throughputGoodBps = enhanced.goodBW;
    metrics.throughputSentBps = enhanced.sentBW;
    metrics.throughputGrowMode = static_cast<char>(enhanced.growMode + 'c');
    metrics.throughputPingMode = static_cast<char>(enhanced.growModePing + 'c');
    metrics.throughputLossMode = static_cast<char>(enhanced.growModeLost + 'c');

    int nMsg = 0;
    int nBytes = 0;
    int nMsgGuaranteed = 0;
    int nBytesGuaranteed = 0;
    ReadNetTransportSendQueueInfo(channel, nMsg, nBytes, nMsgGuaranteed, nBytesGuaranteed);
    metrics.queueBytes = nBytes;
    metrics.guaranteedQueueBytes = nBytesGuaranteed;

    ChannelStatistics stat{};
    channel->getInternalStatistics(stat);
    metrics.ackTotal = stat.ackTotal;
    metrics.ackLost = stat.ackLost;
    metrics.ackWaitCount = stat.revisitedNo;
    metrics.ackWaitAverageSec = 1e-6f * stat.revisitedAveAge;
    metrics.ackWaitMaxSec = 1e-6f * stat.revisitedMaxAge;
}

template <class ChannelT>
bool ReadNetTransportChannelMetrics(ChannelT* channel, NetworkTransportMetrics& metrics)
{
    if (channel == nullptr)
    {
        return false;
    }

    CollectNetTransportChannelMetrics(channel, metrics);
    return true;
}

template <class ChannelT>
bool WriteNetTransportChannelParams(ChannelT* channel, const NetworkParams& params)
{
    if (channel == nullptr)
    {
        return false;
    }

    channel->setNetworkParams(params);
    return true;
}

} // namespace Poseidon
