#pragma once

#include <Poseidon/AI/AI.hpp>
#include <Poseidon/World/World.hpp>


namespace Poseidon
{
inline AICenterMode TranslateMode(GameMode gameMode)
{
    switch (gameMode)
    {
        case GModeNetware:
            return AICMNetwork;
        case GModeIntro:
            return AICMIntro;
        case GModeArcade:
        default:
            return AICMArcade;
    }
}

}  // namespace Poseidon
