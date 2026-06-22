#pragma once

#include <Poseidon/Network/NetTransportChannelInfo.hpp>

#include <utility>
#include <Poseidon/Network/NetTransportMetrics.hpp>

namespace Poseidon
{

template <class ChannelT>
void ReadNetTransportClientSendQueueInfo(ChannelT* channel, int& nMsg, int& nBytes, int& nMsgGuaranteed,
                                         int& nBytesGuaranteed)
{
    ReadNetTransportSendQueueInfo(channel, nMsg, nBytes, nMsgGuaranteed, nBytesGuaranteed);
}

template <class ChannelT, class EnterFn, class LeaveFn>
void ReadNetTransportClientSendQueueInfoLocked(ChannelT* channel, int& nMsg, int& nBytes, int& nMsgGuaranteed,
                                               int& nBytesGuaranteed, EnterFn&& enterLock, LeaveFn&& leaveLock)
{
    std::forward<EnterFn>(enterLock)();
    ReadNetTransportClientSendQueueInfo(channel, nMsg, nBytes, nMsgGuaranteed, nBytesGuaranteed);
    std::forward<LeaveFn>(leaveLock)();
}

template <class ChannelT, class EnterFn, class LeaveFn>
void GetNetTransportClientSendQueueInfoLocked(ChannelT* channel, int& nMsg, int& nBytes, int& nMsgGuaranteed,
                                              int& nBytesGuaranteed, EnterFn&& enterLock, LeaveFn&& leaveLock)
{
    ReadNetTransportClientSendQueueInfoLocked(channel, nMsg, nBytes, nMsgGuaranteed, nBytesGuaranteed,
                                              std::forward<EnterFn>(enterLock), std::forward<LeaveFn>(leaveLock));
}

template <class ChannelT, class OnChanged>
bool ReadNetTransportClientConnectionInfo(ChannelT* channel, int& latencyMS, int& throughputBPS, int& oldLatency,
                                          int& oldThroughput, OnChanged&& onChanged)
{
#ifdef NET_LOG_INFO
    return ReadNetTransportConnectionInfo(channel, latencyMS, throughputBPS, oldLatency, oldThroughput,
                                          std::forward<OnChanged>(onChanged));
#else
    (void)oldLatency;
    (void)oldThroughput;
    (void)onChanged;
    return ReadNetTransportConnectionInfo(channel, latencyMS, throughputBPS);
#endif
}

template <class ChannelT>
bool ReadNetTransportClientConnectionInfo(ChannelT* channel, int& latencyMS, int& throughputBPS)
{
    return ReadNetTransportConnectionInfo(channel, latencyMS, throughputBPS);
}

template <class ChannelT, class EnterFn, class LeaveFn>
bool ReadNetTransportClientConnectionInfoLocked(ChannelT* channel, int& latencyMS, int& throughputBPS,
                                                EnterFn&& enterLock, LeaveFn&& leaveLock)
{
    std::forward<EnterFn>(enterLock)();
    const bool result = ReadNetTransportClientConnectionInfo(channel, latencyMS, throughputBPS);
    std::forward<LeaveFn>(leaveLock)();
    return result;
}

template <class ChannelT, class EnterFn, class LeaveFn>
bool ReadNetTransportClientMetricsLocked(ChannelT* channel, NetworkTransportMetrics& metrics, EnterFn&& enterLock,
                                         LeaveFn&& leaveLock)
{
    std::forward<EnterFn>(enterLock)();
    const bool result = ReadNetTransportChannelMetrics(channel, metrics);
    std::forward<LeaveFn>(leaveLock)();
    return result;
}

template <class ChannelT, class EnterFn, class LeaveFn>
void WriteNetTransportClientParamsLocked(ChannelT* channel, const NetworkParams& params, EnterFn&& enterLock,
                                         LeaveFn&& leaveLock)
{
    std::forward<EnterFn>(enterLock)();
    WriteNetTransportChannelParams(channel, params);
    std::forward<LeaveFn>(leaveLock)();
}

template <class ChannelT, class EnterFn, class LeaveFn>
void SetNetTransportClientNetworkParamsLocked(ChannelT* channel, const NetworkParams& params, EnterFn&& enterLock,
                                              LeaveFn&& leaveLock)
{
    WriteNetTransportClientParamsLocked(channel, params, std::forward<EnterFn>(enterLock),
                                        std::forward<LeaveFn>(leaveLock));
}

} // namespace Poseidon
