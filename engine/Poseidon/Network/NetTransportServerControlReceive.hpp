#pragma once

#include <Poseidon/Network/NetTransportEnumResponse.hpp>
#include <Poseidon/Network/NetTransportServerControlMessage.hpp>

#include <utility>
using Poseidon::Foundation::IteratorState;
using Poseidon::Foundation::nsNoMoreCallbacks;
using Poseidon::Foundation::nsOK;
using Poseidon::Foundation::nsOutputSent;

namespace Poseidon
{

template <class OwnerT, class GetOwnerFn, class HandleControlFn>
NetStatus ProcessNetTransportServerControlReceiveCallback(NetMessage* msg, GetOwnerFn&& getOwner,
                                                          HandleControlFn&& handleControl)
{
    auto* owner = std::forward<GetOwnerFn>(getOwner)();
    if (!owner || !msg || msg->getLength() < sizeof(unsigned32) || !(msg->getFlags() & MSG_MAGIC_FLAG))
    {
        return nsNoMoreCallbacks;
    }

    struct sockaddr_in distant;
    msg->getDistant(distant);
    std::forward<HandleControlFn>(handleControl)(*owner, *msg, distant, *static_cast<unsigned32*>(msg->getData()));
    return nsNoMoreCallbacks;
}

template <class EnterUsrFn, class LeaveUsrFn, class SendEnumResponseFn>
bool TryHandleNetTransportServerEnumRequest(unsigned length, const void* data, int magicApplication,
                                            const SessionPacket& session, unsigned playerCount, unsigned32 serial,
                                            const struct sockaddr_in& distant, EnterUsrFn&& enterUsr,
                                            LeaveUsrFn&& leaveUsr, SendEnumResponseFn&& sendEnumResponse)
{
    if (length != sizeof(EnumPacket))
    {
        return false;
    }

    const auto& request = *static_cast<const EnumPacket*>(data);
    enterUsr();
    if (request.magicApplication == magicApplication)
    {
        const SessionPacket response = BuildNetTransportEnumResponse(session, MAGIC_ENUM_RESPONSE, serial, playerCount);
        sendEnumResponse(response, distant, SESSION_PACKET_SIZE);
#ifdef ENUM_LEGACY
        const SessionPacket legacyResponse =
            BuildNetTransportEnumResponse(session, MAGIC_ENUM_RESPONSE_LEGACY, response.request, response.numPlayers);
        sendEnumResponse(legacyResponse, distant, SESSION_PACKET_1);
#endif
    }
    leaveUsr();
    return true;
}

template <class MessageT, class UserCountFn, class EnterUsrFn, class LeaveUsrFn, class MessageFactory>
bool ProcessNetTransportServerEnumRequestMessage(const MessageT& message, int magicApplication,
                                                 const SessionPacket& session, UserCountFn&& getPlayerCount,
                                                 const struct sockaddr_in& distant, EnterUsrFn&& enterUsr,
                                                 LeaveUsrFn&& leaveUsr, MessageFactory&& newMessage)
{
    auto&& messageFactory = newMessage;
    return TryHandleNetTransportServerEnumRequest(
        message.getLength(), message.getData(), magicApplication, session,
        std::forward<UserCountFn>(getPlayerCount)() - 1, message.getSerial(), distant,
        std::forward<EnterUsrFn>(enterUsr), std::forward<LeaveUsrFn>(leaveUsr),
        [&](const SessionPacket& response, const struct sockaddr_in& distantAddress, unsigned responseLength) {
            DispatchNetTransportEnumResponse(response, distantAddress, responseLength, message.getChannel(),
                                             messageFactory);
        });
}

template <class UserMapT, class SupportMapT, class CreateAllocator, class PeerT, class SupportFactory>
NetTransportCreatePlayerAttemptResult ProcessNetTransportCreatePlayerAttemptWithPeer(
    UserMapT& users, SupportMapT& support, AutoArray<CreatePlayerInfo, CreateAllocator>& createPlayers, int& botId,
    const SessionPacket& session, const char* serverPassword, int maxPlayers, int uniqueToTry,
    const struct sockaddr_in& distant, const CreatePlayerPacket& request, NetCallBack* processRoutine, PeerT& peer,
    SupportFactory&& makeSupport)
{
    return ProcessNetTransportCreatePlayerAttempt(
        users, support, createPlayers, botId, session, serverPassword, maxPlayers, uniqueToTry, distant, request,
        processRoutine,
        [&peer, distant]() mutable
        {
            struct sockaddr_in address = distant;
            return peer.findChannel(address);
        },
        [&peer, distant]() mutable
        {
            struct sockaddr_in address = distant;
            return peer.getPool()->createChannel(address, &peer);
        },
        std::forward<SupportFactory>(makeSupport));
}

template <class UserMapT, class SupportMapT, class CreateAllocator, class EnterPeerLockFn, class LeavePeerLockFn,
          class GetPeerFn, class SupportFactory>
NetTransportCreatePlayerAttemptResult ProcessNetTransportCreatePlayerAttemptLocked(
    UserMapT& users, SupportMapT& support, AutoArray<CreatePlayerInfo, CreateAllocator>& createPlayers, int& botId,
    const SessionPacket& session, const char* serverPassword, int maxPlayers, int uniqueToTry,
    const struct sockaddr_in& distant, const CreatePlayerPacket& request, NetCallBack* processRoutine,
    EnterPeerLockFn&& enterPeerLock, LeavePeerLockFn&& leavePeerLock, GetPeerFn&& getPeer, SupportFactory&& makeSupport)
{
    enterPeerLock();
    auto* peer = getPeer();
    NET_ERROR(peer);
    const NetTransportCreatePlayerAttemptResult attempt = ProcessNetTransportCreatePlayerAttemptWithPeer(
        users, support, createPlayers, botId, session, serverPassword, maxPlayers, uniqueToTry, distant, request,
        processRoutine, *peer, std::forward<SupportFactory>(makeSupport));
    leavePeerLock();
    return attempt;
}

template <class LogTruncatedFn, class LogAcceptedFn>
void LogNetTransportAcceptedPlayer(const NetTransportCreatePlayerAttemptResult& attempt, int sessionPlayerCount,
                                   unsigned userCount, LogTruncatedFn&& logTruncated, LogAcceptedFn&& logAccepted)
{
    NET_ERROR(attempt.accepted.info);
    const CreatePlayerInfo& info = *attempt.accepted.info;
    if (attempt.accepted.nameTruncated)
    {
        std::forward<LogTruncatedFn>(logTruncated)(info);
    }

    std::forward<LogAcceptedFn>(logAccepted)(sessionPlayerCount, info, userCount);
}

template <class EnterUsrFn, class LeaveUsrFn, class AttemptFn, class FinishDestroyFn, class SleepFn,
          class CreateErrorFn, class AcceptedFn, class DispatchAckFn>
bool TryHandleNetTransportCreatePlayerControlMessage(unsigned length, const void* data, int magicApplication,
                                                     const struct sockaddr_in& distant, NetChannel* fallbackChannel,
                                                     EnterUsrFn&& enterUsr, LeaveUsrFn&& leaveUsr,
                                                     AttemptFn&& attemptCreatePlayer,
                                                     FinishDestroyFn&& finishDestroyPlayer, SleepFn&& sleepAfterDestroy,
                                                     CreateErrorFn&& onCreateError, AcceptedFn&& onAccepted,
                                                     DispatchAckFn&& dispatchAck)
{
    if (length != sizeof(CreatePlayerPacket))
    {
        return false;
    }

    const auto& request = *static_cast<const CreatePlayerPacket*>(data);
    enterUsr();
    if (request.magicApplication != magicApplication)
    {
        leaveUsr();
        return true;
    }

    NetTransportCreatePlayerAttemptResult attempt{};
    for (;;)
    {
        attempt = attemptCreatePlayer(request);
        if (attempt.duplicate)
        {
            leaveUsr();
            return true;
        }
        if (!attempt.retryAfterDestroy)
        {
            break;
        }

        finishDestroyPlayer(attempt.previousPlayer);
        leaveUsr();
        sleepAfterDestroy();
        enterUsr();
    }

    if (attempt.channel == nullptr && attempt.result == CRError)
    {
        onCreateError();
    }
    else if (attempt.channel != nullptr)
    {
        onAccepted(attempt, request);
    }
    leaveUsr();

    dispatchAck(BuildNetTransportServerControlAckPlan(attempt, distant, fallbackChannel));
    return true;
}

template <class EnterUsrFn, class LeaveUsrFn, class AttemptFn, class FinishDestroyFn, class SleepFn,
          class CreateErrorFn, class AcceptedFn, class MessageFactory>
bool ProcessNetTransportCreatePlayerControlMessage(unsigned length, const void* data, int magicApplication,
                                                   const struct sockaddr_in& distant, NetChannel* fallbackChannel,
                                                   EnterUsrFn&& enterUsr, LeaveUsrFn&& leaveUsr,
                                                   AttemptFn&& attemptCreatePlayer,
                                                   FinishDestroyFn&& finishDestroyPlayer, SleepFn&& sleepAfterDestroy,
                                                   CreateErrorFn&& onCreateError, AcceptedFn&& onAccepted,
                                                   MessageFactory&& newMessage)
{
    auto&& messageFactory = newMessage;
    return TryHandleNetTransportCreatePlayerControlMessage(
        length, data, magicApplication, distant, fallbackChannel, std::forward<EnterUsrFn>(enterUsr),
        std::forward<LeaveUsrFn>(leaveUsr), std::forward<AttemptFn>(attemptCreatePlayer),
        std::forward<FinishDestroyFn>(finishDestroyPlayer), std::forward<SleepFn>(sleepAfterDestroy),
        std::forward<CreateErrorFn>(onCreateError), std::forward<AcceptedFn>(onAccepted),
        [&](const NetTransportServerControlAckPlan& plan)
        { DispatchNetTransportServerControlAck(plan, messageFactory); });
}

template <class UserMapT, class SupportMapT, class CreateAllocator, class EnterUsrFn, class LeaveUsrFn,
          class FinishDestroyFn, class SleepFn, class CreateErrorFn, class AcceptedFn, class EnterPeerLockFn,
          class LeavePeerLockFn, class GetPeerFn, class SupportFactory, class MessageFactory>
bool ProcessNetTransportServerCreatePlayerControlMessage(
    unsigned length, const void* data, int magicApplication, UserMapT& users, SupportMapT& support,
    AutoArray<CreatePlayerInfo, CreateAllocator>& createPlayers, int& botId, const SessionPacket& session,
    const char* serverPassword, int uniqueToTry, const struct sockaddr_in& distant, NetChannel* fallbackChannel,
    NetCallBack* processRoutine, EnterUsrFn&& enterUsr, LeaveUsrFn&& leaveUsr, FinishDestroyFn&& finishDestroyPlayer,
    SleepFn&& sleepAfterDestroy, CreateErrorFn&& onCreateError, AcceptedFn&& onAccepted,
    EnterPeerLockFn&& enterPeerLock, LeavePeerLockFn&& leavePeerLock, GetPeerFn&& getPeer, SupportFactory&& makeSupport,
    MessageFactory&& newMessage)
{
    auto&& enterPeer = enterPeerLock;
    auto&& leavePeer = leavePeerLock;
    auto&& peerGetter = getPeer;
    auto&& supportFactory = makeSupport;
    return ProcessNetTransportCreatePlayerControlMessage(
        length, data, magicApplication, distant, fallbackChannel, std::forward<EnterUsrFn>(enterUsr),
        std::forward<LeaveUsrFn>(leaveUsr),
        [&](const CreatePlayerPacket& request)
        {
            return ProcessNetTransportCreatePlayerAttemptLocked(
                users, support, createPlayers, botId, session, serverPassword, session.maxPlayers, uniqueToTry, distant,
                request, processRoutine, enterPeer, leavePeer, peerGetter, supportFactory);
        },
        std::forward<FinishDestroyFn>(finishDestroyPlayer), std::forward<SleepFn>(sleepAfterDestroy),
        std::forward<CreateErrorFn>(onCreateError), std::forward<AcceptedFn>(onAccepted),
        std::forward<MessageFactory>(newMessage));
}

template <class MessageT, class UserMapT, class SupportMapT, class CreateAllocator, class EnterUsrFn, class LeaveUsrFn,
          class FinishDestroyFn, class SleepFn, class CreateErrorFn, class AcceptedFn, class EnterPeerLockFn,
          class LeavePeerLockFn, class GetPeerFn, class SupportFactory, class MessageFactory>
bool ProcessNetTransportServerCreatePlayerMessage(
    const MessageT& message, int magicApplication, UserMapT& users, SupportMapT& support,
    AutoArray<CreatePlayerInfo, CreateAllocator>& createPlayers, int& botId, const SessionPacket& session,
    const char* serverPassword, int uniqueToTry, const struct sockaddr_in& distant, NetCallBack* processRoutine,
    EnterUsrFn&& enterUsr, LeaveUsrFn&& leaveUsr, FinishDestroyFn&& finishDestroyPlayer, SleepFn&& sleepAfterDestroy,
    CreateErrorFn&& onCreateError, AcceptedFn&& onAccepted, EnterPeerLockFn&& enterPeerLock,
    LeavePeerLockFn&& leavePeerLock, GetPeerFn&& getPeer, SupportFactory&& makeSupport, MessageFactory&& newMessage)
{
    return ProcessNetTransportServerCreatePlayerControlMessage(
        message.getLength(), message.getData(), magicApplication, users, support, createPlayers, botId, session,
        serverPassword, uniqueToTry, distant, message.getChannel(), processRoutine, std::forward<EnterUsrFn>(enterUsr),
        std::forward<LeaveUsrFn>(leaveUsr), std::forward<FinishDestroyFn>(finishDestroyPlayer),
        std::forward<SleepFn>(sleepAfterDestroy), std::forward<CreateErrorFn>(onCreateError),
        std::forward<AcceptedFn>(onAccepted), std::forward<EnterPeerLockFn>(enterPeerLock),
        std::forward<LeavePeerLockFn>(leavePeerLock), std::forward<GetPeerFn>(getPeer),
        std::forward<SupportFactory>(makeSupport), std::forward<MessageFactory>(newMessage));
}

template <class MessageT, class UserMapT, class SupportMapT, class CreateAllocator, class EnterUsrFn, class LeaveUsrFn,
          class FinishDestroyFn, class SleepFn, class CreateErrorFn, class BeforeLogAcceptedFn, class LogTruncatedFn,
          class LogAcceptedFn, class EnterPeerLockFn, class LeavePeerLockFn, class GetPeerFn, class SupportFactory,
          class MessageFactory>
bool ProcessNetTransportServerCreatePlayerMessageWithAcceptedLog(
    const MessageT& message, int magicApplication, UserMapT& users, SupportMapT& support,
    AutoArray<CreatePlayerInfo, CreateAllocator>& createPlayers, int& botId, const SessionPacket& session,
    const char* serverPassword, int uniqueToTry, const struct sockaddr_in& distant, NetCallBack* processRoutine,
    EnterUsrFn&& enterUsr, LeaveUsrFn&& leaveUsr, FinishDestroyFn&& finishDestroyPlayer, SleepFn&& sleepAfterDestroy,
    CreateErrorFn&& onCreateError, BeforeLogAcceptedFn&& beforeLogAccepted, LogTruncatedFn&& logTruncated,
    LogAcceptedFn&& logAccepted, EnterPeerLockFn&& enterPeerLock, LeavePeerLockFn&& leavePeerLock, GetPeerFn&& getPeer,
    SupportFactory&& makeSupport, MessageFactory&& newMessage)
{
    auto&& beforeAccepted = beforeLogAccepted;
    auto&& truncatedLogger = logTruncated;
    auto&& acceptedLogger = logAccepted;
    return ProcessNetTransportServerCreatePlayerMessage(
        message, magicApplication, users, support, createPlayers, botId, session, serverPassword, uniqueToTry, distant,
        processRoutine, std::forward<EnterUsrFn>(enterUsr), std::forward<LeaveUsrFn>(leaveUsr),
        std::forward<FinishDestroyFn>(finishDestroyPlayer), std::forward<SleepFn>(sleepAfterDestroy),
        std::forward<CreateErrorFn>(onCreateError),
        [&](const NetTransportCreatePlayerAttemptResult& attempt, const CreatePlayerPacket&)
        {
            beforeAccepted();
            LogNetTransportAcceptedPlayer(attempt, session.numPlayers, users.card(), truncatedLogger, acceptedLogger);
        },
        std::forward<EnterPeerLockFn>(enterPeerLock), std::forward<LeavePeerLockFn>(leavePeerLock),
        std::forward<GetPeerFn>(getPeer), std::forward<SupportFactory>(makeSupport),
        std::forward<MessageFactory>(newMessage));
}

template <class EnterUsrFn, class LeaveUsrFn, class ReconnectFn, class DispatchAckFn>
bool TryHandleNetTransportReconnectPlayerControlMessage(unsigned length, const void* data,
                                                        const struct sockaddr_in& distant, NetChannel* fallbackChannel,
                                                        EnterUsrFn&& enterUsr, LeaveUsrFn&& leaveUsr,
                                                        ReconnectFn&& reconnectPlayer, DispatchAckFn&& dispatchAck)
{
    if (length != sizeof(ReconnectPlayerPacket))
    {
        return false;
    }

    const auto& request = *static_cast<const ReconnectPlayerPacket*>(data);
    enterUsr();
    const NetTransportPlayerReconnectResult reconnect = reconnectPlayer(request);
    leaveUsr();

    dispatchAck(BuildNetTransportServerControlAckPlan(reconnect, request.playerNo, distant, fallbackChannel));
    return true;
}

template <class EnterUsrFn, class LeaveUsrFn, class ReconnectFn, class MessageFactory>
bool ProcessNetTransportReconnectPlayerControlMessage(unsigned length, const void* data,
                                                      const struct sockaddr_in& distant, NetChannel* fallbackChannel,
                                                      EnterUsrFn&& enterUsr, LeaveUsrFn&& leaveUsr,
                                                      ReconnectFn&& reconnectPlayer, MessageFactory&& newMessage)
{
    auto&& messageFactory = newMessage;
    return TryHandleNetTransportReconnectPlayerControlMessage(
        length, data, distant, fallbackChannel, std::forward<EnterUsrFn>(enterUsr), std::forward<LeaveUsrFn>(leaveUsr),
        std::forward<ReconnectFn>(reconnectPlayer), [&](const NetTransportServerControlAckPlan& plan)
        { DispatchNetTransportServerControlAck(plan, messageFactory); });
}

template <class UserMapT, class EnterUsrFn, class LeaveUsrFn, class MessageFactory>
bool ProcessNetTransportServerReconnectPlayerControlMessage(unsigned length, const void* data, UserMapT& users,
                                                            const struct sockaddr_in& distant,
                                                            NetChannel* fallbackChannel, EnterUsrFn&& enterUsr,
                                                            LeaveUsrFn&& leaveUsr, MessageFactory&& newMessage)
{
    return ProcessNetTransportReconnectPlayerControlMessage(
        length, data, distant, fallbackChannel, std::forward<EnterUsrFn>(enterUsr), std::forward<LeaveUsrFn>(leaveUsr),
        [&](const ReconnectPlayerPacket& request)
        {
            struct sockaddr_in reconnectDistant = distant;
            return TryReconnectNetTransportPlayer(users, request.playerNo, reconnectDistant);
        },
        std::forward<MessageFactory>(newMessage));
}

template <class MessageT, class UserMapT, class EnterUsrFn, class LeaveUsrFn, class MessageFactory>
bool ProcessNetTransportServerReconnectPlayerMessage(const MessageT& message, UserMapT& users,
                                                     const struct sockaddr_in& distant, EnterUsrFn&& enterUsr,
                                                     LeaveUsrFn&& leaveUsr, MessageFactory&& newMessage)
{
    return ProcessNetTransportServerReconnectPlayerControlMessage(
        message.getLength(), message.getData(), users, distant, message.getChannel(),
        std::forward<EnterUsrFn>(enterUsr), std::forward<LeaveUsrFn>(leaveUsr),
        std::forward<MessageFactory>(newMessage));
}

template <class HandleEnumFn, class HandleCreateFn, class HandleReconnectFn>
bool TryHandleNetTransportServerControlReceive(unsigned32 magic, HandleEnumFn&& handleEnum,
                                               HandleCreateFn&& handleCreate, HandleReconnectFn&& handleReconnect)
{
    switch (magic)
    {
        case MAGIC_ENUM_REQUEST:
            return handleEnum();

        case MAGIC_CREATE_PLAYER:
            return handleCreate();

        case MAGIC_RECONNECT_PLAYER:
            return handleReconnect();

        default:
            return false;
    }
}

template <class OwnerT, class GetOwnerFn, class HandleEnumFn, class HandleCreateFn, class HandleReconnectFn>
NetStatus ProcessNetTransportServerControlReceiveCallbackWithMagic(NetMessage* msg, GetOwnerFn&& getOwner,
                                                                   HandleEnumFn&& handleEnum,
                                                                   HandleCreateFn&& handleCreate,
                                                                   HandleReconnectFn&& handleReconnect)
{
    auto&& enumHandler = handleEnum;
    auto&& createHandler = handleCreate;
    auto&& reconnectHandler = handleReconnect;
    return ProcessNetTransportServerControlReceiveCallback<OwnerT>(
        msg, std::forward<GetOwnerFn>(getOwner),
        [&](OwnerT& owner, NetMessage& message, const struct sockaddr_in& distant, unsigned32 magic)
        {
            TryHandleNetTransportServerControlReceive(
                magic, [&]() { return enumHandler(owner, message, distant); },
                [&]() { return createHandler(owner, message, distant); },
                [&]() { return reconnectHandler(owner, message, distant); });
        });
}

} // namespace Poseidon
