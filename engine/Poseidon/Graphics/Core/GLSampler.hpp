#pragma once

#include <glad/gl.h>

// Texture sampler slot 0 bind.
//
// Only ONE sampler slot is per-draw-configurable from the backend code path.
// Slot 1's sampler is set once at init (`EngineGL33::CreateSamplerStates`) and
// never touched per-draw, so multi-pass results don't depend on inherited
// slot-1 state.  This helper exposes the slot-0 per-draw bind only.

namespace Poseidon
{
namespace render::sampler
{

inline void BindSlot0(GLuint sampler)
{
    glBindSampler(0, sampler);
}

} // namespace render::sampler

} // namespace Poseidon
