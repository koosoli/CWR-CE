#pragma once

#include <Poseidon/Network/NetTransportMessageQueue.hpp>

#include <utility>
using Poseidon::Foundation::nsNoMoreCallbacks;
using Poseidon::Foundation::nsOK;
using Poseidon::Foundation::nsOutputSent;

namespace Poseidon
{

template <class EnterFn>
bool BeginNetTransportSendCompleteProcessing(SendCompleteCallback* callback, EnterFn&& enterSend)
{
    if (!callback)
    {
        return false;
    }

    enterSend();
    return true;
}

template <class MessageRef, class LeaveFn>
void FinishNetTransportSendCompleteProcessing(MessageRef& sent, SendCompleteCallback* callback, void* context,
                                              LeaveFn&& leaveSend)
{
    DrainNetTransportSendComplete(sent, callback, context);
    leaveSend();
}

template <class MessageRef, class EnterFn, class LeaveFn>
bool ProcessNetTransportSendCompleteQueue(MessageRef& sent, SendCompleteCallback* callback, void* context,
                                          EnterFn&& enterSend, LeaveFn&& leaveSend)
{
    if (!BeginNetTransportSendCompleteProcessing(callback, std::forward<EnterFn>(enterSend)))
    {
        return false;
    }

    FinishNetTransportSendCompleteProcessing(sent, callback, context, std::forward<LeaveFn>(leaveSend));
    return true;
}

template <class MessageRef, class EnterFn, class LeaveFn>
NetStatus QueueNetTransportCompletedMessage(NetMessage* msg, bool canQueue, MessageRef& sent, EnterFn&& enterSend,
                                            LeaveFn&& leaveSend)
{
    if (!msg || !canQueue)
    {
        return nsNoMoreCallbacks;
    }

    enterSend();
    msg->next = sent;
    sent = msg;
    leaveSend();
    return nsNoMoreCallbacks;
}

template <class OwnerT, class CanQueueFn, class SentRefFn, class EnterFn, class LeaveFn>
NetStatus ProcessNetTransportSendCompleteCallback(NetMessage* msg, void* data, CanQueueFn&& canQueue,
                                                  SentRefFn&& sentRef, EnterFn&& enterSend, LeaveFn&& leaveSend)
{
    auto* owner = static_cast<OwnerT*>(data);
    if (!owner)
    {
        return nsNoMoreCallbacks;
    }

    auto&& sent = std::forward<SentRefFn>(sentRef)(*owner);
    return QueueNetTransportCompletedMessage(
        msg, std::forward<CanQueueFn>(canQueue)(*owner), sent, [owner, &enterSend]() { enterSend(*owner); },
        [owner, &leaveSend]() { leaveSend(*owner); });
}

template <class MessageT>
void ConfigureNetTransportQueuedSend(MessageT& message, NetCallBack* callback, void* callbackData,
                                     unsigned64 sendTimeout)
{
    message.setCallback(callback, nsOutputSent, callbackData);
    message.setSendTimeout(sendTimeout);
}

template <class MessageRef, class EnterFn, class LeaveFn>
void RemoveNetTransportSendCompleteQueue(MessageRef& sent, EnterFn&& enterSend, LeaveFn&& leaveSend)
{
    enterSend();
    ClearNetTransportMessageList(sent);
    leaveSend();
}

template <class MessageRef, class EnterFn, class LeaveFn>
void RemoveNetTransportCompletedMessages(MessageRef& sent, EnterFn&& enterSend, LeaveFn&& leaveSend)
{
    RemoveNetTransportSendCompleteQueue(sent, std::forward<EnterFn>(enterSend), std::forward<LeaveFn>(leaveSend));
}

} // namespace Poseidon
