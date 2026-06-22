#pragma once

#include <Poseidon/Network/NetTransportPeerSetup.hpp>
#include <Poseidon/Network/NetTransportUserIteration.hpp>

#include <Poseidon/Network/Legacy/netpch.hpp>

#include <cstdio>
#include <cstring>
#include <string>
#include <utility>
using Poseidon::Foundation::IteratorState;

namespace Poseidon
{

template <class UserMapT>
std::string FormatNetTransportServerUsers(UserMapT& users)
{
    char usersText[1024];

    std::snprintf(usersText, sizeof(usersText), "%u: ", users.card());
    ForEachNetTransportUserPlayerChannel(users, [&usersText](int player, NetChannel*)
                                         { std::sprintf(usersText + std::strlen(usersText), " %d", player); });
    return usersText;
}

template <class UserMapT, class LogFn>
void LogNetTransportServerUsers(UserMapT& users, LogFn&& logUsers)
{
    const std::string usersText = FormatNetTransportServerUsers(users);
    std::forward<LogFn>(logUsers)(usersText.c_str());
}

template <class GetPeerFn, class GetLocalAddressFn>
bool TryWriteNetTransportServerUrl(GetPeerFn&& getPeer, int sessionPort, GetLocalAddressFn&& getLocalAddress,
                                   char* address, DWORD addressLen)
{
    if (getPeer() == nullptr)
    {
        return false;
    }

    NET_ERROR(address);
    struct sockaddr_in local;
    getLocalAddress(local, sessionPort);
    std::snprintf(address, addressLen, "%u.%u.%u.%u:%u", (unsigned)IP4(local), (unsigned)IP3(local),
                  (unsigned)IP2(local), (unsigned)IP1(local), (unsigned)sessionPort);
    return true;
}

template <class EnterFn, class LeaveFn, class GetPeerFn, class GetLocalAddressFn>
bool TryWriteNetTransportServerUrlLocked(EnterFn&& enterPoolLock, LeaveFn&& leavePoolLock, GetPeerFn&& getPeer,
                                         int sessionPort, GetLocalAddressFn&& getLocalAddress, char* address,
                                         DWORD addressLen)
{
    return AccessNetTransportPeerLocked(std::forward<EnterFn>(enterPoolLock), std::forward<LeaveFn>(leavePoolLock),
                                        std::forward<GetPeerFn>(getPeer),
                                        [sessionPort, &getLocalAddress, address, addressLen](auto* peer)
                                        {
                                            return TryWriteNetTransportServerUrl([peer]() { return peer; }, sessionPort,
                                                                                 getLocalAddress, address, addressLen);
                                        });
}

} // namespace Poseidon
