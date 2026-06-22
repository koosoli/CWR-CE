#pragma once

#include <Poseidon/Network/NetTransportAddress.hpp>
#include <Poseidon/Network/NetTransportClientHandshake.hpp>
#include <Poseidon/Network/Legacy/netpch.hpp>

#include <utility>

namespace Poseidon
{

struct NetTransportClientHandshakeTransactionResult
{
    ConnectResult result = CRError;
    unsigned64 now = 0;
    unsigned64 timeout = 0;
    bool acknowledged = false;
};

template <class PacketT, class MessageFactory, class TimeFn, class SleepFn, class EnterFn, class LeaveFn>
NetTransportClientHandshakeTransactionResult
RunNetTransportClientHandshakeTransaction(int& ackPlayer, NetChannel* channel, const PacketT& packet,
                                          MessageFactory&& newMessage, TimeFn&& getNow, SleepFn&& sleep,
                                          EnterFn&& enterLock, LeaveFn&& leaveLock)
{
    NetTransportClientHandshakeTransactionResult transaction{};
    ackPlayer = CRNone;

    transaction.now = getNow();
    unsigned64 next = transaction.now;
    transaction.timeout = ComputeNetTransportHandshakeTimeout(transaction.now);

    do
    {
        if (transaction.now >= next)
        {
            Ref<NetMessage> msg = newMessage(sizeof(packet), channel);
            if (!msg)
            {
                return transaction;
            }

            msg->setFlags(MSG_ALL_FLAGS, MSG_TO_BCAST_FLAG | MSG_MAGIC_FLAG);
            msg->setData((unsigned8*)&packet, sizeof(packet));
            msg->send(true);
            next = ComputeNetTransportHandshakeNextRetry(transaction.now);
        }

        leaveLock();
        sleep(NetTransportHandshakePollWaitMs);
        enterLock();
        transaction.now = getNow();
    } while (ShouldContinueNetTransportHandshakeWait(ackPlayer, transaction.now, transaction.timeout));

    transaction.acknowledged = ackPlayer != CRNone;
    transaction.result = ResolveNetTransportHandshakeResult(ackPlayer);
    return transaction;
}

template <class PacketT, class MessageFactory, class TimeFn, class SleepFn, class EnterFn, class LeaveFn>
ConnectResult RunNetTransportClientHandshakeAttempt(int& ackPlayer, NetChannel* channel, const PacketT& packet,
                                                    int* port, MessageFactory&& newMessage, TimeFn&& getNow,
                                                    SleepFn&& sleep, EnterFn&& enterLock, LeaveFn&& leaveLock)
{
    const NetTransportClientHandshakeTransactionResult transaction = RunNetTransportClientHandshakeTransaction(
        ackPlayer, channel, packet, std::forward<MessageFactory>(newMessage), std::forward<TimeFn>(getNow),
        std::forward<SleepFn>(sleep), std::forward<EnterFn>(enterLock), std::forward<LeaveFn>(leaveLock));

    ApplyNetTransportClientHandshakeSuccess(channel, transaction.acknowledged, port);
    return transaction.result;
}

} // namespace Poseidon
