#pragma once

#include <Poseidon/Network/NetTransportChannelInfo.hpp>
#include <Poseidon/Network/NetTransportPlayerMonitor.hpp>
#include <Poseidon/Network/NetTransportMetrics.hpp>

namespace Poseidon
{

struct NetTransportServerConnectionInfoResult
{
    bool found = false;
    bool destroyedDroppedPlayer = false;
};

template <class UserMapT>
void ReadNetTransportServerSendQueueInfo(UserMapT& users, int player, int& nMsg, int& nBytes, int& nMsgGuaranteed,
                                         int& nBytesGuaranteed)
{
    RefD<NetChannel> channel;
    ReadNetTransportSendQueueInfo(users.get(player, channel) ? (NetChannel*)channel : nullptr, nMsg, nBytes,
                                  nMsgGuaranteed, nBytesGuaranteed);
}

template <class UserMapT, class EnterFn, class LeaveFn>
void ReadNetTransportServerSendQueueInfoLocked(UserMapT& users, int player, int& nMsg, int& nBytes, int& nMsgGuaranteed,
                                               int& nBytesGuaranteed, EnterFn&& enterLock, LeaveFn&& leaveLock)
{
    std::forward<EnterFn>(enterLock)();
    ReadNetTransportServerSendQueueInfo(users, player, nMsg, nBytes, nMsgGuaranteed, nBytesGuaranteed);
    std::forward<LeaveFn>(leaveLock)();
}

template <class UserMapT, class EnterFn, class LeaveFn>
void GetNetTransportServerSendQueueInfoLocked(UserMapT& users, int player, int& nMsg, int& nBytes, int& nMsgGuaranteed,
                                              int& nBytesGuaranteed, EnterFn&& enterLock, LeaveFn&& leaveLock)
{
    ReadNetTransportServerSendQueueInfoLocked(users, player, nMsg, nBytes, nMsgGuaranteed, nBytesGuaranteed,
                                              std::forward<EnterFn>(enterLock), std::forward<LeaveFn>(leaveLock));
}

template <class UserMapT>
bool ReadNetTransportServerChannelMetrics(UserMapT& users, int player, NetworkTransportMetrics& metrics)
{
    RefD<NetChannel> channel;
    return ReadNetTransportChannelMetrics(users.get(player, channel) ? (NetChannel*)channel : nullptr, metrics);
}

template <class UserMapT, class EnterFn, class LeaveFn>
bool ReadNetTransportServerChannelMetricsLocked(UserMapT& users, int player, NetworkTransportMetrics& metrics,
                                                EnterFn&& enterLock, LeaveFn&& leaveLock)
{
    std::forward<EnterFn>(enterLock)();
    const bool result = ReadNetTransportServerChannelMetrics(users, player, metrics);
    std::forward<LeaveFn>(leaveLock)();
    return result;
}

template <class UserMapT, class FinishDestroyFn>
NetTransportServerConnectionInfoResult TryReadNetTransportServerConnectionInfo(UserMapT& users, int player, int botId,
                                                                               int& latencyMS, int& throughputBPS,
                                                                               FinishDestroyFn&& finishDestroy)
{
    RefD<NetChannel> channel;
    if (!users.get(player, channel))
    {
        return {};
    }

    ReadNetTransportConnectionInfo((NetChannel*)channel, latencyMS, throughputBPS);
    const bool destroyDroppedPlayer = ShouldNetTransportDestroyDroppedPlayer(player, botId, channel->dropped());
    if (destroyDroppedPlayer)
    {
        finishDestroy(player);
    }

    return {
        .found = true,
        .destroyedDroppedPlayer = destroyDroppedPlayer,
    };
}

template <class UserMapT, class OnChanged, class FinishDestroyFn>
NetTransportServerConnectionInfoResult
TryReadNetTransportServerConnectionInfo(UserMapT& users, int player, int botId, int& latencyMS, int& throughputBPS,
                                        int& oldLatency, int& oldThroughput, OnChanged&& onChanged,
                                        FinishDestroyFn&& finishDestroy)
{
    RefD<NetChannel> channel;
    if (!users.get(player, channel))
    {
        return {};
    }

    ReadNetTransportConnectionInfo((NetChannel*)channel, latencyMS, throughputBPS, oldLatency, oldThroughput,
                                   std::forward<OnChanged>(onChanged));
    const bool destroyDroppedPlayer = ShouldNetTransportDestroyDroppedPlayer(player, botId, channel->dropped());
    if (destroyDroppedPlayer)
    {
        finishDestroy(player);
    }

    return {
        .found = true,
        .destroyedDroppedPlayer = destroyDroppedPlayer,
    };
}

template <class UserMapT, class FinishDestroyFn, class GetNowFn, class LogPlayersFn>
bool ProcessNetTransportServerConnectionInfo(UserMapT& users, int player, int botId, int& latencyMS, int& throughputBPS,
#ifdef DEDICATED_STAT_LOG
                                             unsigned64& nextConsoleLog, unsigned64 consoleLogInterval,
#endif
                                             FinishDestroyFn&& finishDestroy, GetNowFn&& getNow,
                                             LogPlayersFn&& logPlayers)
{
    const NetTransportServerConnectionInfoResult info = TryReadNetTransportServerConnectionInfo(
        users, player, botId, latencyMS, throughputBPS, std::forward<FinishDestroyFn>(finishDestroy));
    if (!info.found)
    {
        return false;
    }

#ifdef DEDICATED_STAT_LOG
    const unsigned64 now = getNow();
    if (now >= nextConsoleLog)
    {
        const bool doConsole = true;
        if (doConsole)
        {
            nextConsoleLog = now + consoleLogInterval;
        }
        logPlayers(doConsole);
    }
#else
    (void)getNow;
    (void)logPlayers;
#endif

    return true;
}

template <class UserMapT, class IsDedicatedFn, class GetStatisticsFn, class LogPlayerFn>
void LogNetTransportServerConnectionInfoPlayers(UserMapT& users, int botId, bool doConsole, IsDedicatedFn&& isDedicated,
                                                GetStatisticsFn&& getStatistics, LogPlayerFn&& logPlayer)
{
    auto&& dedicatedServer = isDedicated;
    if (!dedicatedServer())
    {
        return;
    }

    auto&& statsGetter = getStatistics;
    auto&& playerLogger = logPlayer;
    ForEachNetTransportNonBotPlayer(users, botId,
                                    [&](int player, NetChannel*)
                                    {
                                        RString stat = statsGetter(player);
                                        if (doConsole)
                                        {
                                            playerLogger(player, stat);
                                        }
                                    });
}

template <class UserMapT, class EnterFn, class LeaveFn, class FinishDestroyFn, class GetNowFn, class LogPlayersFn>
bool ProcessNetTransportServerConnectionInfoLocked(UserMapT& users, int player, int botId, int& latencyMS,
                                                   int& throughputBPS,
#ifdef DEDICATED_STAT_LOG
                                                   unsigned64& nextConsoleLog, unsigned64 consoleLogInterval,
#endif
                                                   EnterFn&& enterLock, LeaveFn&& leaveLock,
                                                   FinishDestroyFn&& finishDestroy, GetNowFn&& getNow,
                                                   LogPlayersFn&& logPlayers)
{
    std::forward<EnterFn>(enterLock)();
    const bool result =
        ProcessNetTransportServerConnectionInfo(users, player, botId, latencyMS, throughputBPS,
#ifdef DEDICATED_STAT_LOG
                                                nextConsoleLog, consoleLogInterval,
#endif
                                                std::forward<FinishDestroyFn>(finishDestroy),
                                                std::forward<GetNowFn>(getNow), std::forward<LogPlayersFn>(logPlayers));
    std::forward<LeaveFn>(leaveLock)();
    return result;
}

template <class UserMapT, class EnterFn, class LeaveFn, class FinishDestroyFn, class GetNowFn, class IsDedicatedFn,
          class GetStatisticsFn, class LogPlayerFn>
bool GetNetTransportServerConnectionInfoLocked(UserMapT& users, int player, int botId, int& latencyMS,
                                               int& throughputBPS,
#ifdef DEDICATED_STAT_LOG
                                               unsigned64& nextConsoleLog, unsigned64 consoleLogInterval,
#endif
                                               EnterFn&& enterLock, LeaveFn&& leaveLock,
                                               FinishDestroyFn&& finishDestroy, GetNowFn&& getNow,
                                               IsDedicatedFn&& isDedicated, GetStatisticsFn&& getStatistics,
                                               LogPlayerFn&& logPlayer)
{
    return ProcessNetTransportServerConnectionInfoLocked(
        users, player, botId, latencyMS, throughputBPS,
#ifdef DEDICATED_STAT_LOG
        nextConsoleLog, consoleLogInterval,
#endif
        std::forward<EnterFn>(enterLock), std::forward<LeaveFn>(leaveLock),
        std::forward<FinishDestroyFn>(finishDestroy), std::forward<GetNowFn>(getNow),
        [&users, botId, &isDedicated, &getStatistics, &logPlayer](bool doConsole) {
            LogNetTransportServerConnectionInfoPlayers(users, botId, doConsole, isDedicated, getStatistics, logPlayer);
        });
}

} // namespace Poseidon
