#pragma once

#include <Poseidon/Network/Legacy/netpch.hpp>

#include <utility>

namespace Poseidon
{

template <class LeaveReceiveLock, class EnterReceiveLock, class ProcessMessage>
void DrainNetTransportUserMessages(Ref<NetMessage>& received, LeaveReceiveLock&& leaveReceiveLock,
                                   EnterReceiveLock&& enterReceiveLock, ProcessMessage&& processMessage)
{
    Ref<NetMessage> message;
    while (received)
    {
        message = received;
        received = message->next;
        message->next = nullptr;
        leaveReceiveLock();
        processMessage(message);
        enterReceiveLock();
    }
}

template <class EnterReceiveLock, class LeaveReceiveLock, class ProcessMessage>
void ProcessNetTransportUserMessageQueue(Ref<NetMessage>& received, EnterReceiveLock&& enterReceiveLock,
                                         LeaveReceiveLock&& leaveReceiveLock, ProcessMessage&& processMessage)
{
    enterReceiveLock();
    DrainNetTransportUserMessages(received, leaveReceiveLock, enterReceiveLock,
                                  std::forward<ProcessMessage>(processMessage));
    leaveReceiveLock();
}

} // namespace Poseidon
