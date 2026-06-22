#pragma once

#include <Poseidon/Network/NetTransportUserIteration.hpp>
#include <utility>
using Poseidon::Foundation::IteratorState;

namespace Poseidon
{

inline bool ShouldNetTransportDestroyDroppedPlayer(int player, int botId, bool dropped)
{
    return player != botId && dropped;
}

template <class UserMapT, class Callback>
void ForEachNetTransportNonBotPlayer(UserMapT& users, int botId, Callback&& callback)
{
    ForEachNetTransportUserPlayerChannel(users,
                                         [botId, &callback](int player, NetChannel* channel)
                                         {
                                             if (player != botId)
                                             {
                                                 callback(player, channel);
                                             }
                                         });
}

} // namespace Poseidon
