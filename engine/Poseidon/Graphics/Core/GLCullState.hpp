#pragma once

#include <glad/gl.h>

// Atomic GL cull-face state bundles.
//
// `ApplyPipeline` owns cull state; the per-mode helpers here are the unique
// callsites of `glEnable(GL_CULL_FACE)` / `glDisable(GL_CULL_FACE)` /
// `glCullFace`.  Each sets both enable + face atomically, so partial state
// (e.g. enabled with a stale cull face) is unrepresentable.

namespace Poseidon
{
namespace render::cull
{

inline void Back()
{
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
}

inline void Front()
{
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
}

inline void None()
{
    glDisable(GL_CULL_FACE);
}

// Winding (`GL_CW` or `GL_CCW`).  Separated from cull mode because
// the descriptor carries them as independent fields — a draw can
// be double-sided (None) while still declaring its source winding
// for downstream consumers that need it.
inline void FrontFaceCW()
{
    glFrontFace(GL_CW);
}
inline void FrontFaceCCW()
{
    glFrontFace(GL_CCW);
}

} // namespace render::cull

} // namespace Poseidon
