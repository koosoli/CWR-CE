#pragma once

#include <Poseidon/Network/NetTransport.hpp>
#include <Poseidon/Network/NetTransportProtocol.hpp>
#include <Poseidon/Network/NetTransportTermination.hpp>

#include <utility>

class NetChannel;

namespace Poseidon
{

DestroyPlayerPacket BuildNetTransportDestroyPlayerPacket();
void ResetNetTransportClientTerminationState(bool& sessionTerminated, NetTerminationReason& whySessionTerminated);
float ReadNetTransportClientLastMsgAge(unsigned64 now, const NetChannel* channel);
float ReadNetTransportClientLastMsgAgeReported(unsigned64 now, unsigned64 lastMsgReported);
void RecordNetTransportClientLastMsgAgeReported(unsigned64 now, unsigned64& lastMsgReported);
bool UpdateNetTransportClientDroppedState(bool amIBot, bool channelDropped, bool& sessionTerminated,
                                          NetTerminationReason& whySessionTerminated);
bool ReadNetTransportClientSessionTerminated(bool hasChannel, bool amIBot, bool channelDropped, bool& sessionTerminated,
                                             NetTerminationReason& whySessionTerminated);

template <class ChannelT, class EnterFn, class LeaveFn, class NowFn>
float ReadNetTransportClientLastMsgAgeLocked(ChannelT* channel, EnterFn&& enterLock, LeaveFn&& leaveLock,
                                             NowFn&& getNow)
{
    std::forward<EnterFn>(enterLock)();
    const float result = ReadNetTransportClientLastMsgAge(std::forward<NowFn>(getNow)(), channel);
    std::forward<LeaveFn>(leaveLock)();
    return result;
}

template <class ChannelT, class EnterFn, class LeaveFn>
bool ReadNetTransportClientSessionTerminatedLocked(ChannelT* channel, bool amIBot, bool& sessionTerminated,
                                                   NetTerminationReason& whySessionTerminated, EnterFn&& enterLock,
                                                   LeaveFn&& leaveLock)
{
    std::forward<EnterFn>(enterLock)();
    const bool hasChannel = channel != nullptr;
    const bool channelDropped = hasChannel && channel->dropped();
    const bool result = ReadNetTransportClientSessionTerminated(hasChannel, amIBot, channelDropped, sessionTerminated,
                                                                whySessionTerminated);
    std::forward<LeaveFn>(leaveLock)();
    return result;
}

template <class EnterFn, class LeaveFn>
NetTerminationReason ReadNetTransportClientTerminationReasonLocked(NetTerminationReason& whySessionTerminated,
                                                                   EnterFn&& enterLock, LeaveFn&& leaveLock)
{
    std::forward<EnterFn>(enterLock)();
    const NetTerminationReason result = whySessionTerminated;
    std::forward<LeaveFn>(leaveLock)();
    return result;
}

template <class NowFn>
float ReadNetTransportClientLastMsgAgeReportedNow(unsigned64 lastMsgReported, NowFn&& getNow)
{
    return ReadNetTransportClientLastMsgAgeReported(std::forward<NowFn>(getNow)(), lastMsgReported);
}

template <class NowFn>
void RecordNetTransportClientLastMsgAgeReportedNow(unsigned64& lastMsgReported, NowFn&& getNow)
{
    RecordNetTransportClientLastMsgAgeReported(std::forward<NowFn>(getNow)(), lastMsgReported);
}

template <class ChannelHolder, class ReceivedQueueT, class SplitQueueT, class SplitTailT, class SplitUrgentQueueT,
          class SplitUrgentTailT, class SentQueueT>
void InitializeNetTransportClientState(unsigned64& lastMsgReported, ChannelHolder& channel, ReceivedQueueT& received,
                                       SplitQueueT& split, SplitTailT& lastSplit, SplitUrgentQueueT& splitUrgent,
                                       SplitUrgentTailT& lastSplitUrgent, SentQueueT& sent, bool& sessionTerminated,
                                       NetTerminationReason& whySessionTerminated, int32& magicApp, unsigned64 now)
{
    lastMsgReported = now;
    channel = nullptr;
    received = nullptr;
    split = nullptr;
    lastSplit = nullptr;
    splitUrgent = nullptr;
    lastSplitUrgent = nullptr;
    sent = nullptr;
    ResetNetTransportClientTerminationState(sessionTerminated, whySessionTerminated);
    magicApp = 0;
}

template <class MessageFactory, class SleepFn>
void NotifyNetTransportClientDisconnect(bool& sessionTerminated, NetTerminationReason& whySessionTerminated,
                                        NetChannel* channel, unsigned32 destructWaitMs, MessageFactory&& createMessage,
                                        SleepFn&& sleepMs)
{
    if (sessionTerminated)
    {
        return;
    }

    ApplyNetTransportTermination(sessionTerminated, whySessionTerminated, NTRDisconnected);
    if (!channel)
    {
        return;
    }

    const DestroyPlayerPacket packet = BuildNetTransportDestroyPlayerPacket();
    auto out = createMessage(sizeof(packet), channel);
    if (!out)
    {
        return;
    }

    out->setFlags(MSG_ALL_FLAGS, MSG_MAGIC_FLAG);
    out->setData((unsigned8*)&packet, sizeof(packet));
    out->send(true);
    sleepMs(destructWaitMs);
}

template <class ChannelHolder, class LeaveFn, class DeleteFn>
void ReleaseNetTransportClientChannel(ChannelHolder& channel, LeaveFn&& leaveSend, DeleteFn&& deleteChannel)
{
    auto old = channel;
    channel = nullptr;
    leaveSend();
    if (old)
    {
        deleteChannel(old);
    }
}

template <class ChannelHolder, class UnsetClientFn, class RemoveUserMessagesFn, class RemoveSendCompleteFn,
          class LeaveFn, class DeleteFn, class CheckPoolFn, class FreeMemoryFn>
void FinalizeNetTransportClientShutdown(ChannelHolder& channel, UnsetClientFn&& unsetClient,
                                        RemoveUserMessagesFn&& removeUserMessages,
                                        RemoveSendCompleteFn&& removeSendComplete, LeaveFn&& leaveSend,
                                        DeleteFn&& deleteChannel, CheckPoolFn&& checkPool, FreeMemoryFn&& freeMemory)
{
    std::forward<UnsetClientFn>(unsetClient)();
    std::forward<RemoveUserMessagesFn>(removeUserMessages)();
    std::forward<RemoveSendCompleteFn>(removeSendComplete)();
    ReleaseNetTransportClientChannel(channel, std::forward<LeaveFn>(leaveSend), std::forward<DeleteFn>(deleteChannel));
    std::forward<CheckPoolFn>(checkPool)();
    std::forward<FreeMemoryFn>(freeMemory)();
}

template <class EnterFn, class NotifyFn, class FinalizeFn>
void ProcessNetTransportClientShutdownLocked(EnterFn&& enterSend, NotifyFn&& notifyDisconnect, FinalizeFn&& finalize)
{
    std::forward<EnterFn>(enterSend)();
    std::forward<NotifyFn>(notifyDisconnect)();
    std::forward<FinalizeFn>(finalize)();
}

template <class ChannelHolder, class MessageFactory, class SleepFn, class EnterFn, class ClearVoiceFn,
          class ShutdownVoiceFn, class UnsetClientFn, class RemoveUserMessagesFn, class RemoveSendCompleteFn,
          class LeaveFn, class DeleteFn, class CheckPoolFn, class FreeMemoryFn>
void ProcessNetTransportClientShutdownWithVoice(ChannelHolder& channel, bool& sessionTerminated,
                                                NetTerminationReason& whySessionTerminated, unsigned32 destructWaitMs,
                                                MessageFactory&& createMessage, SleepFn&& sleepMs, EnterFn&& enterSend,
                                                ClearVoiceFn&& clearVoice, ShutdownVoiceFn&& shutdownVoice,
                                                UnsetClientFn&& unsetClient, RemoveUserMessagesFn&& removeUserMessages,
                                                RemoveSendCompleteFn&& removeSendComplete, LeaveFn&& leaveSend,
                                                DeleteFn&& deleteChannel, CheckPoolFn&& checkPool,
                                                FreeMemoryFn&& freeMemory)
{
    ProcessNetTransportClientShutdownLocked(
        std::forward<EnterFn>(enterSend),
        [&]()
        {
            NotifyNetTransportClientDisconnect(sessionTerminated, whySessionTerminated, channel, destructWaitMs,
                                               std::forward<MessageFactory>(createMessage),
                                               std::forward<SleepFn>(sleepMs));
            std::forward<ClearVoiceFn>(clearVoice)();
            std::forward<ShutdownVoiceFn>(shutdownVoice)();
        },
        [&]()
        {
            FinalizeNetTransportClientShutdown(channel, std::forward<UnsetClientFn>(unsetClient),
                                               std::forward<RemoveUserMessagesFn>(removeUserMessages),
                                               std::forward<RemoveSendCompleteFn>(removeSendComplete),
                                               std::forward<LeaveFn>(leaveSend), std::forward<DeleteFn>(deleteChannel),
                                               std::forward<CheckPoolFn>(checkPool),
                                               std::forward<FreeMemoryFn>(freeMemory));
        });
}

} // namespace Poseidon
