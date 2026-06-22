#pragma once

#include <Poseidon/Network/NetTransportServerDestroyReceive.hpp>
#include <Poseidon/Network/NetTransportFragmentQueue.hpp>
#include <Poseidon/Network/NetTransportMessageQueue.hpp>
#include <Poseidon/Network/NetTransportServerPlayerLookup.hpp>

#include <Poseidon/Audio/Voice/VonNet.hpp>

#include <utility>
using Poseidon::Foundation::IteratorState;
using Poseidon::Foundation::nsNoMoreCallbacks;
using Poseidon::Foundation::nsOK;
using Poseidon::Foundation::nsOutputSent;

namespace Poseidon
{

template <class MessageT, class FindPlayerFn, class OnMissingPlayerFn, class OnVoicePacketFn, class OnUserMessageFn>
void DispatchNetTransportServerUserMessage(const MessageT& message, FindPlayerFn&& findPlayer,
                                           OnMissingPlayerFn&& onMissingPlayer, OnVoicePacketFn&& onVoicePacket,
                                           OnUserMessageFn&& onUserMessage)
{
    const int player = findPlayer(message.getChannel());
    if (player == -1)
    {
        onMissingPlayer(message.getChannel());
        return;
    }

    if (vonIsDataPacket(message.getData(), message.getLength()))
    {
        onVoicePacket(player, message.getData(), message.getLength());
        return;
    }

    onUserMessage(player, (char*)message.getData(), message.getLength());
}

template <class MessageRef, class EnterRcvFn, class LeaveRcvFn, class FindPlayerFn, class OnMissingPlayerFn,
          class OnVoicePacketFn, class OnUserMessageFn>
void ProcessNetTransportServerUserMessageQueue(MessageRef& received, EnterRcvFn&& enterReceive,
                                               LeaveRcvFn&& leaveReceive, FindPlayerFn&& findPlayer,
                                               OnMissingPlayerFn&& onMissingPlayer, OnVoicePacketFn&& onVoicePacket,
                                               OnUserMessageFn&& onUserMessage)
{
    auto&& find = findPlayer;
    auto&& onMissing = onMissingPlayer;
    auto&& onVoice = onVoicePacket;
    auto&& onUser = onUserMessage;
    ProcessNetTransportUserMessageQueue(
        received, std::forward<EnterRcvFn>(enterReceive), std::forward<LeaveRcvFn>(leaveReceive),
        [&](const Ref<NetMessage>& currentMessage)
        { DispatchNetTransportServerUserMessage(*currentMessage, find, onMissing, onVoice, onUser); });
}

template <class CallbackT, class MessageRef, class EnterRcvFn, class LeaveRcvFn, class FindPlayerFn,
          class OnMissingPlayerFn, class OnVoicePacketFn>
void ProcessNetTransportServerUserMessages(CallbackT* callback, void* context, MessageRef& received,
                                           EnterRcvFn&& enterReceive, LeaveRcvFn&& leaveReceive,
                                           FindPlayerFn&& findPlayer, OnMissingPlayerFn&& onMissingPlayer,
                                           OnVoicePacketFn&& onVoicePacket)
{
    if (!callback)
    {
        return;
    }

    ProcessNetTransportServerUserMessageQueue(
        received, std::forward<EnterRcvFn>(enterReceive), std::forward<LeaveRcvFn>(leaveReceive),
        std::forward<FindPlayerFn>(findPlayer), std::forward<OnMissingPlayerFn>(onMissingPlayer),
        std::forward<OnVoicePacketFn>(onVoicePacket),
        [callback, context](int player, char* data, int length) { (*callback)(player, data, length, context); });
}

template <class CallbackT, class MessageRef, class UserMapT, class EnterRcvFn, class LeaveRcvFn, class EnterUsrFn,
          class LeaveUsrFn, class GetVoiceServerFn>
void ProcessNetTransportServerUserMessagesWithLookup(CallbackT* callback, void* context, MessageRef& received,
                                                     UserMapT& users, EnterRcvFn&& enterReceive,
                                                     LeaveRcvFn&& leaveReceive, EnterUsrFn&& enterUsers,
                                                     LeaveUsrFn&& leaveUsers, GetVoiceServerFn&& getVoiceServer)
{
    ProcessNetTransportServerUserMessages(
        callback, context, received, std::forward<EnterRcvFn>(enterReceive), std::forward<LeaveRcvFn>(leaveReceive),
        [&users, &enterUsers, &leaveUsers](NetChannel* channel)
        { return ReadNetTransportServerPlayerByChannelLocked(users, channel, enterUsers, leaveUsers); },
        [](NetChannel* channel)
        { RptF("No player found for channel %u - message ignored", (unsigned)(uintptr_t)channel); },
        [&getVoiceServer](int player, const void* data, int length)
        { RouteNetTransportServerVoicePacket(getVoiceServer, player, data, length); });
}

template <class OwnerT, class GetOwnerFn, class HandleMagicFn, class HandleUserFn>
NetStatus ProcessNetTransportServerReceiveCallback(NetMessage* msg, GetOwnerFn&& getOwner, HandleMagicFn&& handleMagic,
                                                   HandleUserFn&& handleUser)
{
    auto* owner = std::forward<GetOwnerFn>(getOwner)();
    if (!owner || !msg)
    {
        return nsNoMoreCallbacks;
    }

    const unsigned length = msg->getLength();
    if (!length)
    {
        return nsNoMoreCallbacks;
    }

    const unsigned flags = msg->getFlags();
    if ((flags & MSG_MAGIC_FLAG) != 0)
    {
        if (length < sizeof(unsigned32))
        {
            return nsNoMoreCallbacks;
        }

        std::forward<HandleMagicFn>(handleMagic)(*owner, *msg, length, *static_cast<unsigned32*>(msg->getData()));
        return nsNoMoreCallbacks;
    }

    std::forward<HandleUserFn>(handleUser)(*owner, flags, msg);
    return nsNoMoreCallbacks;
}

struct NetTransportServerUserReceiveResult
{
    bool handled = false;
    bool inserted = false;
    bool mergedPart = false;
};

template <class SupportT, class MergeFn, class InsertFn>
NetTransportServerUserReceiveResult
TryHandleNetTransportServerUserReceive(unsigned flags, NetMessage*& msg, SupportT& support,
                                       MergeFn&& mergeMessageListFn, InsertFn&& insertReceived)
{
    if ((flags & MSG_MAGIC_FLAG) != 0)
    {
        return {};
    }

    if (!TryAssembleNetTransportMessageFragments(flags, msg, support.m_split, support.m_lastSplit,
                                                 support.m_splitUrgent, support.m_lastSplitUrgent,
                                                 std::forward<MergeFn>(mergeMessageListFn)))
    {
        return {
            .handled = true,
            .inserted = false,
            .mergedPart = false,
        };
    }

    insertReceived(msg);
    return {
        .handled = true,
        .inserted = true,
        .mergedPart = (flags & MSG_PART_FLAG) != 0,
    };
}

template <class SupportT, class EnterRcvFn, class LeaveRcvFn, class MergeFn, class InsertFn>
NetTransportServerUserReceiveResult
ProcessNetTransportServerUserReceive(unsigned flags, NetMessage*& msg, SupportT& support, EnterRcvFn&& enterReceive,
                                     LeaveRcvFn&& leaveReceive, MergeFn&& mergeMessageListFn, InsertFn&& insertReceived)
{
    enterReceive();
    NetTransportServerUserReceiveResult result = TryHandleNetTransportServerUserReceive(
        flags, msg, support, std::forward<MergeFn>(mergeMessageListFn), std::forward<InsertFn>(insertReceived));
    leaveReceive();
    return result;
}

template <class OwnerT, class GetOwnerFn, class HandleDestroyFn, class HandleUserFn>
NetStatus ProcessNetTransportServerReceiveCallbackWithDestroy(NetMessage* msg, GetOwnerFn&& getOwner,
                                                              HandleDestroyFn&& handleDestroy,
                                                              HandleUserFn&& handleUser)
{
    auto&& destroy = handleDestroy;
    auto&& user = handleUser;
    return ProcessNetTransportServerReceiveCallback<OwnerT>(
        msg, std::forward<GetOwnerFn>(getOwner),
        [&destroy](OwnerT& owner, NetMessage& message, unsigned length, unsigned32 magic)
        {
            if (magic == MAGIC_DESTROY_PLAYER)
            {
                destroy(owner, message, length);
            }
        },
        [&user](OwnerT& owner, unsigned flags, NetMessage* currentMessage) { user(owner, flags, currentMessage); });
}

template <class MessageT, class SupportMapT, class EnterRcvFn, class LeaveRcvFn, class AssertSupportFn, class MergeFn,
          class InsertFn>
NetTransportServerUserReceiveResult ProcessNetTransportServerChannelReceive(
    unsigned flags, NetMessage*& msg, MessageT& message, SupportMapT& supportMap, EnterRcvFn&& enterReceive,
    LeaveRcvFn&& leaveReceive, AssertSupportFn&& assertSupport, MergeFn&& mergeMessageListFn, InsertFn&& insertReceived)
{
    struct sockaddr_in distant;
    message.getDistant(distant);

    enterReceive();
    auto* support = supportMap.get(sockaddrKey(distant));
    std::forward<AssertSupportFn>(assertSupport)(support);
    NetTransportServerUserReceiveResult result = TryHandleNetTransportServerUserReceive(
        flags, msg, *support, std::forward<MergeFn>(mergeMessageListFn), std::forward<InsertFn>(insertReceived));
    leaveReceive();
    return result;
}

} // namespace Poseidon
