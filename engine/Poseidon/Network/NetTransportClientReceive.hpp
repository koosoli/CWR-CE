#pragma once

#include <Poseidon/Network/NetTransportClientControlMessage.hpp>
#include <Poseidon/Network/NetTransportFragmentQueue.hpp>
#include <Poseidon/Network/NetTransportMessageQueue.hpp>

#include <utility>
using Poseidon::Foundation::nsNoMoreCallbacks;

namespace Poseidon
{

template <class EnterSndFn, class LeaveSndFn>
bool TryHandleNetTransportClientControlReceive(unsigned flags, unsigned length, const void* data, int& ackPlayer,
                                               int& playerNo, bool& sessionTerminated,
                                               NetTerminationReason& whySessionTerminated, EnterSndFn&& enterSnd,
                                               LeaveSndFn&& leaveSnd)
{
    if ((flags & MSG_MAGIC_FLAG) == 0)
    {
        return false;
    }

    if (length < sizeof(MagicPacket))
    {
        return true;
    }

    const unsigned32 magic = *static_cast<const unsigned32*>(data);
    switch (magic)
    {
        case MAGIC_ACK_PLAYER:
        case MAGIC_TERMINATE_SESSION:
            enterSnd();
            TryHandleNetTransportClientControlMessage(magic, data, length, ackPlayer, playerNo, sessionTerminated,
                                                      whySessionTerminated);
            leaveSnd();
            break;

        default:
            break;
    }

    return true;
}

template <class OwnerT, class GetOwnerFn, class RecordLastMsgFn, class HandleControlFn, class HandleUserFn>
NetStatus ProcessNetTransportClientReceiveCallback(NetMessage* msg, GetOwnerFn&& getOwner,
                                                   RecordLastMsgFn&& recordLastMessage, HandleControlFn&& handleControl,
                                                   HandleUserFn&& handleUser)
{
    auto* owner = std::forward<GetOwnerFn>(getOwner)();
    if (!owner || !msg)
    {
        return nsNoMoreCallbacks;
    }

    std::forward<RecordLastMsgFn>(recordLastMessage)(*owner, msg->getTime());
    const unsigned length = msg->getLength();
    if (!length)
    {
        return nsNoMoreCallbacks;
    }

    const unsigned flags = msg->getFlags();
    if (std::forward<HandleControlFn>(handleControl)(*owner, flags, length, msg->getData()))
    {
        return nsNoMoreCallbacks;
    }

    std::forward<HandleUserFn>(handleUser)(*owner, flags, msg);
    return nsNoMoreCallbacks;
}

struct NetTransportClientUserReceiveResult
{
    bool handled = false;
    bool inserted = false;
    bool mergedPart = false;
};

template <class MessageRef, class MergeFn, class InsertFn>
NetTransportClientUserReceiveResult
TryHandleNetTransportClientUserReceive(unsigned flags, NetMessage*& msg, MessageRef& split, NetMessage*& lastSplit,
                                       MessageRef& splitUrgent, NetMessage*& lastSplitUrgent,
                                       MergeFn&& mergeMessageListFn, InsertFn&& insertReceived)
{
    if ((flags & MSG_MAGIC_FLAG) != 0)
    {
        return {};
    }

    if (!TryAssembleNetTransportMessageFragments(flags, msg, split, lastSplit, splitUrgent, lastSplitUrgent,
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

template <class MessageRef, class EnterRcvFn, class LeaveRcvFn, class MergeFn, class InsertFn>
NetTransportClientUserReceiveResult
ProcessNetTransportClientUserReceive(unsigned flags, NetMessage*& msg, MessageRef& split, NetMessage*& lastSplit,
                                     MessageRef& splitUrgent, NetMessage*& lastSplitUrgent, EnterRcvFn&& enterReceive,
                                     LeaveRcvFn&& leaveReceive, MergeFn&& mergeMessageListFn, InsertFn&& insertReceived)
{
    enterReceive();
    NetTransportClientUserReceiveResult result = TryHandleNetTransportClientUserReceive(
        flags, msg, split, lastSplit, splitUrgent, lastSplitUrgent, std::forward<MergeFn>(mergeMessageListFn),
        std::forward<InsertFn>(insertReceived));
    leaveReceive();
    return result;
}

template <class CallbackT, class MessageRef, class EnterRcvFn, class LeaveRcvFn, class HandleVoiceFn,
          class UpdatePlaybackFn>
void ProcessNetTransportClientUserMessages(CallbackT* callback, void* context, MessageRef& received,
                                           EnterRcvFn&& enterReceive, LeaveRcvFn&& leaveReceive,
                                           HandleVoiceFn&& handleVoice, UpdatePlaybackFn&& updatePlayback)
{
    if (!callback)
    {
        return;
    }

    auto&& handle = handleVoice;
    ProcessNetTransportUserMessageQueue(
        received, std::forward<EnterRcvFn>(enterReceive), std::forward<LeaveRcvFn>(leaveReceive),
        [&](const Ref<NetMessage>& currentMessage)
        {
            if (handle(*currentMessage))
            {
                return;
            }

            (*callback)((char*)currentMessage->getData(), currentMessage->getLength(), context);
        });

    std::forward<UpdatePlaybackFn>(updatePlayback)();
}

} // namespace Poseidon
