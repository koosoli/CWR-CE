#pragma once

#include <Poseidon/Network/Legacy/netpch.hpp>
#include <Poseidon/Network/NetTransport.hpp>

#include <utility>

namespace Poseidon
{

template <class MessageRef>
void InsertNetTransportReceivedMessage(MessageRef& received, NetMessage* msg)
{
    if (!received)
    {
        msg->next = nullptr;
        received = msg;
        return;
    }

    const auto serial = msg->getSerial();
    if (serial < received->getSerial())
    {
        msg->next = received;
        received = msg;
        return;
    }

    NetMessage* current = received;
    while (current->next && current->next->getSerial() < serial)
    {
        current = current->next;
    }

    msg->next = current->next;
    current->next = msg;
}

template <class MessageRef>
void ClearNetTransportMessageList(MessageRef& head)
{
    MessageRef next;
    while (head)
    {
        next = head->next;
        head->next = nullptr;
        head = next;
    }
}

template <class MessageRef, class EnterFn, class LeaveFn>
void ClearNetTransportMessageListLocked(MessageRef& head, EnterFn&& enterLock, LeaveFn&& leaveLock)
{
    std::forward<EnterFn>(enterLock)();
    ClearNetTransportMessageList(head);
    std::forward<LeaveFn>(leaveLock)();
}

template <class MessageRef, class EnterFn, class LeaveFn>
void RemoveNetTransportReceivedMessages(MessageRef& received, EnterFn&& enterReceive, LeaveFn&& leaveReceive)
{
    ClearNetTransportMessageListLocked(received, std::forward<EnterFn>(enterReceive),
                                       std::forward<LeaveFn>(leaveReceive));
}

template <class MessageRef>
void DrainNetTransportSendComplete(MessageRef& sent, SendCompleteCallback* callback, void* context)
{
    NetMessage* msg = sent;
    while (msg)
    {
        (*callback)(static_cast<DWORD>(msg->id), msg->wasSent(), context);
        msg = msg->next;
    }

    ClearNetTransportMessageList(sent);
}

} // namespace Poseidon
