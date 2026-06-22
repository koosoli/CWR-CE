#pragma once

#include "Frame.hpp"

#include <vector>

namespace Poseidon
{
class Engine;
namespace render::frame
{

void ObserveRenderedFrame(Engine& engine, Scene& scene);

// Pass shape of the most recently observed frame — kind + draw count
// per emitted pass, in emission order.  Read by the tri harness
// (`triGetFrameShape` / `triAssertFrameShape`) to pin the pipeline's
// pass structure in integration tests.
struct ObservedPass
{
    FramePassKind kind = FramePassKind::WorldOpaque;
    int draws = 0;
};
const std::vector<ObservedPass>& LastObservedFrameShape();

} // namespace render::frame

} // namespace Poseidon
