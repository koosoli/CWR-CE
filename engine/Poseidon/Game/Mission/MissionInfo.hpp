#pragma once

// Mission introspection — free functions that wrap engine globals (GWorld,
// sensorsMap) behind a stable API, so callers ask "is a mission running?" or
// "what endings does it define?" instead of poking at globals directly.

#include <string>
#include <vector>

namespace Poseidon
{
} // namespace Poseidon


namespace MissionInfo
{

// True iff a playable mission is loaded — GWorld is non-null AND a
// real player has been assigned.  The main menu's rotating background
// scene sets GWorld but no real player, so this correctly returns
// false there.
bool IsActive();

// Scripted endings the active mission declares via ASTEnd1..ASTEnd6
// sensors.  Returns the subset of {"end1".."end6"} for which a
// matching detector exists in the global sensorsMap.  Returns empty
// when !IsActive() OR the mission defines no End sensors.  Does NOT
// include "win", "lose", or "killed" — those are universally
// applicable cheat outcomes, not script-defined endings; callers that
// expose cheats should add them on top.
std::vector<std::string> AvailableEndings();

// Pure helper — given which sensor categories are present, return
// the canonical ending name list.  No engine state access, fully
// unit-testable.  `hasEnd[i]` corresponds to ASTEnd1+i.
std::vector<std::string> EndingsFromSensorFlags(bool hasLoose, const bool hasEnd[6]);

} // namespace MissionInfo
