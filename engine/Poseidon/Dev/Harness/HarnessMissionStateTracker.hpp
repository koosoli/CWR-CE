#ifndef POSEIDON_TEST_HARNESS_MISSION_STATE_TRACKER_HPP
#define POSEIDON_TEST_HARNESS_MISSION_STATE_TRACKER_HPP

#include <Poseidon/Dev/Harness/HarnessServer.hpp>
#include <Poseidon/Dev/Harness/HarnessProtocol.hpp>
#include <Poseidon/Network/Network.hpp>

namespace Poseidon::Dev
{

struct HarnessMissionStateTracker
{
    bool hasPrevState = false;
    NetworkGameState prevState = NGSNone;

    static const char* StateName(NetworkGameState state)
    {
        switch (state)
        {
            case NGSNone:
                return "NGSNone";
            case NGSCreating:
                return "NGSCreating";
            case NGSCreate:
                return "NGSCreate";
            case NGSLogin:
                return "NGSLogin";
            case NGSEdit:
                return "NGSEdit";
            case NGSMissionVoted:
                return "NGSMissionVoted";
            case NGSPrepareSide:
                return "NGSPrepareSide";
            case NGSPrepareRole:
                return "NGSPrepareRole";
            case NGSPrepareOK:
                return "NGSPrepareOK";
            case NGSDebriefing:
                return "NGSDebriefing";
            case NGSDebriefingOK:
                return "NGSDebriefingOK";
            case NGSTransferMission:
                return "NGSTransferMission";
            case NGSLoadIsland:
                return "NGSLoadIsland";
            case NGSBriefing:
                return "NGSBriefing";
            case NGSPlay:
                return "NGSPlay";
        }
        return "NGSUnknown";
    }

    void Poll(Poseidon::Dev::HarnessServer& server)
    {
        NetworkGameState state = NGSNone;
        try
        {
            state = GetNetworkManager().GetServerState();
        }
        catch (...)
        {
            state = NGSNone;
        }

        if (!hasPrevState)
        {
            if (state != NGSNone)
                server.PushEvent(HarnessProtocol::MissionStateEvent(StateName(NGSNone), StateName(state)));
            prevState = state;
            hasPrevState = true;
            return;
        }

        if (state != prevState)
        {
            server.PushEvent(HarnessProtocol::MissionStateEvent(StateName(prevState), StateName(state)));
            prevState = state;
        }
    }
};

#endif // POSEIDON_TEST_HARNESS_MISSION_STATE_TRACKER_HPP

} // namespace Poseidon
