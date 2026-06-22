#ifndef POSEIDON_TEST_HARNESS_PLAYER_TRACKER_HPP
#define POSEIDON_TEST_HARNESS_PLAYER_TRACKER_HPP

// Poll-based player list diffing — pushes player_joined/left events each frame.
// Include this after HarnessServer.hpp and network.hpp.

#include <Poseidon/Dev/Harness/HarnessServer.hpp>
#include <Poseidon/Dev/Harness/HarnessProtocol.hpp>
#include <Poseidon/Network/Network.hpp>
#include <string>
#include <vector>

namespace Poseidon::Dev
{

struct HarnessPlayerTracker
{
    std::vector<int> prevDpids;
    std::vector<std::string> prevNames;

    void Poll(HarnessServer& server)
    {
        std::vector<int> curDpids;
        std::vector<std::string> curNames;
        try
        {
            INetworkManager& netMgr = GetNetworkManager();
            AutoArray<NetPlayerInfo, Foundation::MemAllocSA> players;
            netMgr.GetPlayers(players);
            for (int i = 0; i < players.Size(); i++)
            {
                curDpids.push_back(players[i].dpid);
                curNames.push_back(std::string(players[i].name));
            }
        }
        catch (...)
        {
            // Network manager not available — treat as empty list
        }

        // Detect joins: dpids in current but not in previous
        for (size_t i = 0; i < curDpids.size(); i++)
        {
            bool found = false;
            for (size_t j = 0; j < prevDpids.size(); j++)
            {
                if (curDpids[i] == prevDpids[j]) { found = true; break; }
            }
            if (!found)
                server.PushEvent(HarnessProtocol::PlayerJoinedEvent(curDpids[i], curNames[i].c_str()));
        }

        // Detect leaves: dpids in previous but not in current
        for (size_t i = 0; i < prevDpids.size(); i++)
        {
            bool found = false;
            for (size_t j = 0; j < curDpids.size(); j++)
            {
                if (prevDpids[i] == curDpids[j]) { found = true; break; }
            }
            if (!found)
                server.PushEvent(HarnessProtocol::PlayerLeftEvent(prevDpids[i], prevNames[i].c_str()));
        }

        prevDpids = std::move(curDpids);
        prevNames = std::move(curNames);
    }
};

} // namespace Poseidon::Dev

#endif // POSEIDON_TEST_HARNESS_PLAYER_TRACKER_HPP
