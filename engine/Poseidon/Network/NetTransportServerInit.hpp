#pragma once

#include <Poseidon/Network/NetTransportSessionPacketState.hpp>
#include <Poseidon/Network/NetTransportSessionPolicy.hpp>
#include <Poseidon/Network/NetTransportUserIteration.hpp>

#include <array>
#include <string>
#include <utility>
using Poseidon::Foundation::IteratorState;

namespace Poseidon
{

template <class ReceivedQueueT, class SentQueueT, class EnterUsrFn, class LeaveUsrFn, class SetServerFn,
          class GetSessionPortFn>
void InitializeNetTransportServerState(SessionPacket& session, ReceivedQueueT& received, SentQueueT& sent, int& botId,
                                       int32& magicApp, unsigned16& sessionPort, EnterUsrFn&& enterUsers,
                                       LeaveUsrFn&& leaveUsers, SetServerFn&& registerServer,
                                       GetSessionPortFn&& getSessionPort)
{
    std::forward<EnterUsrFn>(enterUsers)();
    received = nullptr;
    sent = nullptr;
    ResetNetTransportSessionPacket(session);
    botId = 0;
    magicApp = 0;
    std::forward<SetServerFn>(registerServer)();
    sessionPort = std::forward<GetSessionPortFn>(getSessionPort)();
    std::forward<LeaveUsrFn>(leaveUsers)();
}

template <class DestroyPeerFn, class CancelAllMessagesFn, class UnsetServerFn, class RemoveUserMessagesFn,
          class RemoveSendCompleteFn, class RemovePlayersFn, class RemoveVoicePlayersFn, class CheckPoolFn,
          class FreeMemoryFn>
void FinalizeNetTransportServerShutdown(DestroyPeerFn&& destroyPeer, CancelAllMessagesFn&& cancelAllMessages,
                                        UnsetServerFn&& unsetServer, RemoveUserMessagesFn&& removeUserMessages,
                                        RemoveSendCompleteFn&& removeSendComplete, RemovePlayersFn&& removePlayers,
                                        RemoveVoicePlayersFn&& removeVoicePlayers, CheckPoolFn&& checkPool,
                                        FreeMemoryFn&& freeMemory)
{
    std::forward<DestroyPeerFn>(destroyPeer)();
    std::forward<CancelAllMessagesFn>(cancelAllMessages)();
    std::forward<UnsetServerFn>(unsetServer)();
    std::forward<RemoveUserMessagesFn>(removeUserMessages)();
    std::forward<RemoveSendCompleteFn>(removeSendComplete)();
    std::forward<RemovePlayersFn>(removePlayers)();
    std::forward<RemoveVoicePlayersFn>(removeVoicePlayers)();
    std::forward<CheckPoolFn>(checkPool)();
    std::forward<FreeMemoryFn>(freeMemory)();
}

template <class UserMapT, class EnterUsrFn, class LeaveUsrFn, class DisconnectFn, class SleepFn, class DestroyPeerFn,
          class CancelAllMessagesFn, class UnsetServerFn, class RemoveUserMessagesFn, class RemoveSendCompleteFn,
          class RemovePlayersFn, class RemoveVoicePlayersFn, class CheckPoolFn, class FreeMemoryFn>
void ProcessNetTransportServerShutdown(UserMapT& users, EnterUsrFn&& enterUsers, LeaveUsrFn&& leaveUsers,
                                       DisconnectFn&& disconnectPlayer, unsigned32 waitMs, SleepFn&& sleepMs,
                                       DestroyPeerFn&& destroyPeer, CancelAllMessagesFn&& cancelAllMessages,
                                       UnsetServerFn&& unsetServer, RemoveUserMessagesFn&& removeUserMessages,
                                       RemoveSendCompleteFn&& removeSendComplete, RemovePlayersFn&& removePlayers,
                                       RemoveVoicePlayersFn&& removeVoicePlayers, CheckPoolFn&& checkPool,
                                       FreeMemoryFn&& freeMemory)
{
    NotifyNetTransportServerDisconnectAll(
        users, std::forward<EnterUsrFn>(enterUsers), std::forward<LeaveUsrFn>(leaveUsers),
        std::forward<DisconnectFn>(disconnectPlayer), waitMs, std::forward<SleepFn>(sleepMs));
    FinalizeNetTransportServerShutdown(
        std::forward<DestroyPeerFn>(destroyPeer), std::forward<CancelAllMessagesFn>(cancelAllMessages),
        std::forward<UnsetServerFn>(unsetServer), std::forward<RemoveUserMessagesFn>(removeUserMessages),
        std::forward<RemoveSendCompleteFn>(removeSendComplete), std::forward<RemovePlayersFn>(removePlayers),
        std::forward<RemoveVoicePlayersFn>(removeVoicePlayers), std::forward<CheckPoolFn>(checkPool),
        std::forward<FreeMemoryFn>(freeMemory));
}

template <class GetLocalNameFn>
std::string InitializeNetTransportServerSession(SessionPacket& session, const MPVersionInfo& versionInfo,
                                                bool equalModRequired, int maxPlayers, bool hasPassword,
                                                int sessionPort, const char* sessionNameInit,
                                                const char* sessionNameFormat, const char* playerName,
                                                const char* hostname, GetLocalNameFn&& getLocalName)
{
    std::array<char, 128> localName{};
    const char* resolvedHostname = hostname;
    if (!resolvedHostname || !resolvedHostname[0])
    {
        resolvedHostname =
            getLocalName(localName.data(), static_cast<int>(localName.size())) ? localName.data() : nullptr;
    }

    std::string sessionName =
        BuildNetTransportSessionName(sessionNameInit, sessionNameFormat, playerName, resolvedHostname);
    InitializeNetTransportSessionPacket(session, versionInfo, equalModRequired, maxPlayers, hasPassword, sessionPort,
                                        sessionName.c_str());
    return sessionName;
}

template <class GetLocalNameFn
#ifdef DEDICATED_STAT_LOG
          ,
          class GetNowFn
#endif
          >
void ApplyNetTransportServerInit(SessionPacket& session, RString& storedPassword, int32& magicApp, RString& sessionName,
                                 const RString& password, int magic, const MPVersionInfo& versionInfo,
                                 bool equalModRequired, int maxPlayers, int sessionPort, const char* sessionNameInit,
                                 const char* sessionNameFormat, const char* playerName, const char* hostname,
                                 GetLocalNameFn&& getLocalName
#ifdef DEDICATED_STAT_LOG
                                 ,
                                 unsigned64& nextConsoleLog, GetNowFn&& getNow
#endif
)
{
    storedPassword = password;
    magicApp = magic;
    sessionName =
        InitializeNetTransportServerSession(session, versionInfo, equalModRequired, maxPlayers,
                                            password.GetLength() > 0, sessionPort, sessionNameInit, sessionNameFormat,
                                            playerName, hostname, std::forward<GetLocalNameFn>(getLocalName))
            .c_str();
#ifdef DEDICATED_STAT_LOG
    nextConsoleLog = getNow();
#endif
}

template <class EnterFn, class LeaveFn, class ApplyFn>
void ApplyNetTransportServerInitLocked(EnterFn&& enterLock, LeaveFn&& leaveLock, ApplyFn&& apply)
{
    std::forward<EnterFn>(enterLock)();
    std::forward<ApplyFn>(apply)();
    std::forward<LeaveFn>(leaveLock)();
}

template <class IsDedicatedFn, class LogStartFailureFn, class EnterFn, class LeaveFn, class ApplyFn>
bool TryApplyNetTransportServerInitLocked(unsigned16 sessionPort, int port, IsDedicatedFn&& isDedicated,
                                          LogStartFailureFn&& logStartFailure, EnterFn&& enterLock, LeaveFn&& leaveLock,
                                          ApplyFn&& apply)
{
    if (!sessionPort)
    {
        if (std::forward<IsDedicatedFn>(isDedicated)())
        {
            std::forward<LogStartFailureFn>(logStartFailure)(port);
        }
        return false;
    }

    ApplyNetTransportServerInitLocked(std::forward<EnterFn>(enterLock), std::forward<LeaveFn>(leaveLock),
                                      std::forward<ApplyFn>(apply));
    return true;
}

template <class IsDedicatedFn, class LogStartFailureFn, class EnterFn, class LeaveFn, class GetLocalNameFn>
bool TryApplyNetTransportServerInitWithSession(unsigned16 sessionPort, SessionPacket& session, RString& storedPassword,
                                               int32& magicApp, RString& sessionName, int port, const RString& password,
                                               int magic, const MPVersionInfo& versionInfo, bool equalModRequired,
                                               int maxPlayers, const char* sessionNameInit,
                                               const char* sessionNameFormat, const char* playerName,
                                               const char* hostname, IsDedicatedFn&& isDedicated,
                                               LogStartFailureFn&& logStartFailure, EnterFn&& enterLock,
                                               LeaveFn&& leaveLock, GetLocalNameFn&& getLocalName
#ifdef DEDICATED_STAT_LOG
                                               ,
                                               unsigned64& nextConsoleLog
#endif
)
{
    return TryApplyNetTransportServerInitLocked(
        sessionPort, port, std::forward<IsDedicatedFn>(isDedicated), std::forward<LogStartFailureFn>(logStartFailure),
        std::forward<EnterFn>(enterLock), std::forward<LeaveFn>(leaveLock),
        [&]()
        {
            ApplyNetTransportServerInit(session, storedPassword, magicApp, sessionName, password, magic, versionInfo,
                                        equalModRequired, maxPlayers, sessionPort, sessionNameInit, sessionNameFormat,
                                        playerName, hostname, std::forward<GetLocalNameFn>(getLocalName)
#ifdef DEDICATED_STAT_LOG
                                                                  ,
                                        nextConsoleLog, []() { return getSystemTime(); }
#endif
            );
        });
}

} // namespace Poseidon
