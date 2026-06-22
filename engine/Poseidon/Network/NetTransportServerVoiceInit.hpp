#pragma once

#include <Poseidon/Network/NetTransport.hpp>

#include <cstdint>
#include <utility>

namespace Poseidon
{

template <class InitServerFn, class GetServerFn, class SendVoiceFn>
bool InitializeNetTransportServerVoiceForwarder(InitServerFn&& initServer, GetServerFn&& getServer,
                                                SendVoiceFn&& sendVoice)
{
    initServer();
    auto* server = getServer();
    if (!server)
    {
        return false;
    }

    server->setForwarder(
        [sendVoice = std::forward<SendVoiceFn>(sendVoice)](uint32_t target, const void* data, int size) {
            sendVoice(static_cast<int>(target), reinterpret_cast<uint8_t*>(const_cast<void*>(data)), size,
                      NMFHighPriority);
        });
    return true;
}

template <class InitServerFn, class GetServerFn, class SendVoiceFn, class OnInitializedFn>
bool InitializeNetTransportServerVoice(InitServerFn&& initServer, GetServerFn&& getServer, SendVoiceFn&& sendVoice,
                                       OnInitializedFn&& onInitialized)
{
    if (!InitializeNetTransportServerVoiceForwarder(std::forward<InitServerFn>(initServer),
                                                    std::forward<GetServerFn>(getServer),
                                                    std::forward<SendVoiceFn>(sendVoice)))
    {
        return false;
    }

    std::forward<OnInitializedFn>(onInitialized)();
    return true;
}

template <class InitServerFn, class GetServerFn, class SendVoiceFn, class OnInitializedFn>
bool ProcessNetTransportServerVoiceInit(InitServerFn&& initServer, GetServerFn&& getServer, SendVoiceFn&& sendVoice,
                                        OnInitializedFn&& onInitialized)
{
    return InitializeNetTransportServerVoice(std::forward<InitServerFn>(initServer),
                                             std::forward<GetServerFn>(getServer), std::forward<SendVoiceFn>(sendVoice),
                                             std::forward<OnInitializedFn>(onInitialized));
}

template <class InitServerFn, class GetServerFn, class SendMessageFn, class OnInitializedFn>
bool ProcessNetTransportServerVoiceInitWithSend(InitServerFn&& initServer, GetServerFn&& getServer,
                                                SendMessageFn&& sendMessage, OnInitializedFn&& onInitialized)
{
    auto send = std::forward<SendMessageFn>(sendMessage);
    return ProcessNetTransportServerVoiceInit(
        std::forward<InitServerFn>(initServer), std::forward<GetServerFn>(getServer),
        [send = std::move(send)](int target, uint8_t* data, int size, NetMsgFlags flags)
        { send(target, data, size, flags); }, std::forward<OnInitializedFn>(onInitialized));
}

template <class InitServerFn, class GetServerFn, class QueueSendFn, class OnInitializedFn>
bool ProcessNetTransportServerVoiceInitWithQueuedSend(InitServerFn&& initServer, GetServerFn&& getServer,
                                                      QueueSendFn&& queueSend, OnInitializedFn&& onInitialized)
{
    auto send = std::forward<QueueSendFn>(queueSend);
    return ProcessNetTransportServerVoiceInit(
        std::forward<InitServerFn>(initServer), std::forward<GetServerFn>(getServer),
        [send = std::move(send)](int target, uint8_t* data, int size, NetMsgFlags flags)
        {
            DWORD msgID;
            send(target, reinterpret_cast<BYTE*>(data), size, msgID, flags);
        },
        std::forward<OnInitializedFn>(onInitialized));
}

} // namespace Poseidon
