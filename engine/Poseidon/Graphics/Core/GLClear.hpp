#pragma once

#include <glad/gl.h>

// glClear with the depth-mask precondition baked in.
//
// `glClear(GL_DEPTH_BUFFER_BIT)` is gated by the current `glDepthMask` state —
// if the mask is GL_FALSE the depth buffer is NOT cleared even though the clear
// "succeeded", and subsequent draws see stale depth from the previous frame.
// The engine's render path frequently leaves the mask at GL_FALSE after a
// NoZWrite draw, so each clear needs `glDepthMask(GL_TRUE)` first.  This helper
// bundles the precondition so the order can't be reversed and the mask write
// can't be forgotten.  It is the unique callsite of `glClear` in the engine.
//
// Two helpers — one that touches depth and one that doesn't — so a pure colour
// clear doesn't pay the redundant glDepthMask call.

namespace Poseidon
{
namespace render::clear
{

// Clear color + depth + stencil (the engine's standard frame-start
// pattern).  The depthMask is set to GL_TRUE before the clear because
// `glClear` honours `glDepthMask` and the engine's render path
// frequently leaves the mask at GL_FALSE after a `NoZWrite` draw.
inline void ColorDepthStencil(GLbitfield extraMask = 0)
{
    glDepthMask(GL_TRUE);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | extraMask);
}

// Clear an arbitrary mask.  If the mask includes GL_DEPTH_BUFFER_BIT
// the helper always issues `glDepthMask(GL_TRUE)` first.
inline void WithMask(GLbitfield mask)
{
    if (mask & GL_DEPTH_BUFFER_BIT)
        glDepthMask(GL_TRUE);
    if (mask)
        glClear(mask);
}

} // namespace render::clear

} // namespace Poseidon
