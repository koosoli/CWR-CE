#pragma once

#include "Frame.hpp"

namespace Poseidon
{

namespace render::frame
{

// Per-frame dispatch statistics — a pure fold over the Frame value.
// Feeds the --render-frame-log summary line; nothing here touches GL.
struct FrameStats
{
    unsigned int passCount = 0;
    unsigned int drawCount = 0;
    unsigned int maxDrawsInPass = 0;
};

inline FrameStats CountFrameStats(const Frame& f)
{
    FrameStats s;
    for (const auto& p : f.passes)
    {
        ++s.passCount;
        const auto draws = static_cast<unsigned int>(p.draws.size());
        s.drawCount += draws;
        if (draws > s.maxDrawsInPass)
            s.maxDrawsInPass = draws;
    }
    return s;
}

} // namespace render::frame

} // namespace Poseidon
