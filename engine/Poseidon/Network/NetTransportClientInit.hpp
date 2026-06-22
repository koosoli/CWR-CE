#pragma once

#include <Poseidon/Network/NetTransportAddress.hpp>
#include <Poseidon/Network/NetTransportClientChannelSetup.hpp>
#include <Poseidon/Network/NetTransportClientHandshakeTransaction.hpp>
#include <Poseidon/Network/NetTransportClientSession.hpp>
#include <Poseidon/Network/NetTransportPlayerHandshake.hpp>

namespace Poseidon
{

template <class MessageFactory, class TimeFn, class SleepFn, class EnterFn, class LeaveFn>
ConnectResult ProcessNetTransportClientReInit(int& ackPlayer, NetChannel* channel, int32 playerNo,
                                              MessageFactory&& newMessage, TimeFn&& getNow, SleepFn&& sleep,
                                              EnterFn&& enterLock, LeaveFn&& leaveLock)
{
    if (!channel)
    {
        return CRError;
    }

    const ReconnectPlayerPacket packet = BuildReconnectPlayerPacket(playerNo);
    return RunNetTransportClientHandshakeAttempt(
        ackPlayer, channel, packet, nullptr, std::forward<MessageFactory>(newMessage), std::forward<TimeFn>(getNow),
        std::forward<SleepFn>(sleep), std::forward<EnterFn>(enterLock), std::forward<LeaveFn>(leaveLock));
}

template <class EnterFn, class LeaveFn, class ReInitFn>
decltype(auto) ProcessNetTransportClientReInitLocked(EnterFn&& enterLock, LeaveFn&& leaveLock, ReInitFn&& reInit)
{
    std::forward<EnterFn>(enterLock)();
    auto result = std::forward<ReInitFn>(reInit)();
    std::forward<LeaveFn>(leaveLock)();
    return result;
}

template <class MessageFactory, class TimeFn, class SleepFn, class EnterFn, class LeaveFn>
ConnectResult ProcessNetTransportClientReInitWithLock(int& ackPlayer, NetChannel* channel, int32 playerNo,
                                                      MessageFactory&& newMessage, TimeFn&& getNow, SleepFn&& sleep,
                                                      EnterFn&& enterLock, LeaveFn&& leaveLock)
{
    auto&& messageFactory = newMessage;
    auto&& now = getNow;
    auto&& sleeper = sleep;
    auto&& enter = enterLock;
    auto&& leave = leaveLock;
    return ProcessNetTransportClientReInitLocked([&]() { enter(); }, [&]() { leave(); },
                                                 [&]()
                                                 {
                                                     return ProcessNetTransportClientReInit(
                                                         ackPlayer, channel, playerNo, messageFactory, now, sleeper,
                                                         [&]() { enter(); }, [&]() { leave(); });
                                                 });
}

template <class ChannelHolder, class ProcessRoutineT, class ResolveHostFn, class PoolEnterFn, class PoolLeaveFn,
          class GetPoolFn, class GetPeerFn, class MessageFactory, class TimeFn, class SleepFn, class EnterFn,
          class LeaveFn>
ConnectResult ProcessNetTransportClientInit(ChannelHolder& channel, int& ackPlayer, bool& sessionTerminated,
                                            NetTerminationReason& whySessionTerminated, bool& amIBot, int32& magicApp,
                                            RString address, const RString& password, bool botClient, int& port,
                                            const RString& player, const MPVersionInfo& versionInfo, int magic,
                                            int uniqueToTry, ProcessRoutineT processRoutine,
                                            ResolveHostFn&& resolveHost, PoolEnterFn&& enterPool,
                                            PoolLeaveFn&& leavePool, GetPoolFn&& getPool, GetPeerFn&& getPeer,
                                            MessageFactory&& newMessage, TimeFn&& getNow, SleepFn&& sleep,
                                            EnterFn&& enterLock, LeaveFn&& leaveLock)
{
    if (channel)
    {
        return CRError;
    }

    magicApp = magic;
    struct sockaddr_in distant
    {
    };
    if (!TryResolveNetTransportClientAddress(address, port, distant, std::forward<ResolveHostFn>(resolveHost)))
    {
        return CRError;
    }

    ResetNetTransportClientTerminationState(sessionTerminated, whySessionTerminated);

    enterPool();
    channel = CreateNetTransportClientChannel(getPool(), distant, getPeer(), processRoutine);
    if (!channel)
    {
        leavePool();
        return CRError;
    }
    leavePool();

    amIBot = botClient;
    const CreatePlayerPacket packet = BuildCreatePlayerPacket(magicApp, player, password, versionInfo, botClient,
                                                              (int32)(getNow() & 0x7fffffff) + uniqueToTry);
    return RunNetTransportClientHandshakeAttempt(
        ackPlayer, channel, packet, &port, std::forward<MessageFactory>(newMessage), std::forward<TimeFn>(getNow),
        std::forward<SleepFn>(sleep), std::forward<EnterFn>(enterLock), std::forward<LeaveFn>(leaveLock));
}

template <class EnterFn, class LeaveFn, class InitFn>
decltype(auto) ProcessNetTransportClientInitLocked(EnterFn&& enterLock, LeaveFn&& leaveLock, InitFn&& init)
{
    std::forward<EnterFn>(enterLock)();
    auto result = std::forward<InitFn>(init)();
    std::forward<LeaveFn>(leaveLock)();
    return result;
}

template <class ChannelHolder, class ProcessRoutineT, class ResolveHostFn, class PoolEnterFn, class PoolLeaveFn,
          class GetPoolFn, class GetPeerFn, class MessageFactory, class TimeFn, class SleepFn, class EnterFn,
          class LeaveFn>
ConnectResult ProcessNetTransportClientInitWithLock(
    ChannelHolder& channel, int& ackPlayer, bool& sessionTerminated, NetTerminationReason& whySessionTerminated,
    bool& amIBot, int32& magicApp, RString address, const RString& password, bool botClient, int& port,
    const RString& player, const MPVersionInfo& versionInfo, int magic, int uniqueToTry, ProcessRoutineT processRoutine,
    ResolveHostFn&& resolveHost, PoolEnterFn&& enterPool, PoolLeaveFn&& leavePool, GetPoolFn&& getPool,
    GetPeerFn&& getPeer, MessageFactory&& newMessage, TimeFn&& getNow, SleepFn&& sleep, EnterFn&& enterLock,
    LeaveFn&& leaveLock)
{
    auto&& enter = enterLock;
    auto&& leave = leaveLock;
    auto&& resolver = resolveHost;
    auto&& poolEnter = enterPool;
    auto&& poolLeave = leavePool;
    auto&& poolGetter = getPool;
    auto&& peerGetter = getPeer;
    auto&& messageFactory = newMessage;
    auto&& now = getNow;
    auto&& sleeper = sleep;
    return ProcessNetTransportClientInitLocked([&]() { enter(); }, [&]() { leave(); },
                                               [&]()
                                               {
                                                   return ProcessNetTransportClientInit(
                                                       channel, ackPlayer, sessionTerminated, whySessionTerminated,
                                                       amIBot, magicApp, address, password, botClient, port, player,
                                                       versionInfo, magic, uniqueToTry, processRoutine, resolver,
                                                       poolEnter, poolLeave, poolGetter, peerGetter, messageFactory,
                                                       now, sleeper, [&]() { enter(); }, [&]() { leave(); });
                                               });
}

} // namespace Poseidon
