#include <Poseidon/Game/Mission/MissionInfo.hpp>
#include <Poseidon/World/World.hpp>
#include <Poseidon/AI/AI.hpp>
#include <Poseidon/AI/Path/ArcadeWaypoint.hpp>
#include <Poseidon/World/Detection/Detector.hpp>
#include <stdio.h>
#include <Poseidon/Foundation/Types/LLinks.hpp>

using namespace Poseidon;
namespace Poseidon
{
} // namespace Poseidon

namespace MissionInfo
{

bool IsActive()
{
    // GWorld is non-null while the menu's background scene is loaded too,
    // so check for the real player.  The engine clears it on mission end
    // and menu return.
    return GWorld != nullptr && GWorld->GetRealPlayer() != nullptr;
}

std::vector<std::string> EndingsFromSensorFlags(bool /*hasLoose*/, const bool hasEnd[6])
{
    // ASTLoose is reserved for the universal "lose" outcome wired by
    // the cheat layer — it does not represent a script-defined ending
    // in the same sense as ASTEnd1..6 (every mission can lose by
    // running out of objectives or by dying).  Reported here only as
    // an argument to keep the signature stable for future expansion.
    std::vector<std::string> out;
    for (int i = 0; i < 6; i++)
    {
        if (hasEnd[i])
        {
            char name[8];
            snprintf(name, sizeof(name), "end%d", i + 1);
            out.emplace_back(name);
        }
    }
    return out;
}

std::vector<std::string> AvailableEndings()
{
    if (!IsActive())
        return {};

    bool hasLoose = false;
    bool hasEnd[6] = {false, false, false, false, false, false};
    for (int i = 0; i < sensorsMap.Size(); i++)
    {
        Vehicle* veh = sensorsMap[i];
        if (!veh)
            continue;
        Detector* sensor = dyn_cast<Detector>(veh);
        if (!sensor)
            continue;
        switch (sensor->GetAction())
        {
            case ASTLoose:
                hasLoose = true;
                break;
            case ASTEnd1:
                hasEnd[0] = true;
                break;
            case ASTEnd2:
                hasEnd[1] = true;
                break;
            case ASTEnd3:
                hasEnd[2] = true;
                break;
            case ASTEnd4:
                hasEnd[3] = true;
                break;
            case ASTEnd5:
                hasEnd[4] = true;
                break;
            case ASTEnd6:
                hasEnd[5] = true;
                break;
            default:
                break;
        }
    }
    return EndingsFromSensorFlags(hasLoose, hasEnd);
}

} // namespace MissionInfo
