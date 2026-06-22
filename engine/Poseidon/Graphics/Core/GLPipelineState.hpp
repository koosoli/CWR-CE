#pragma once

#include <glad/gl.h>

// Per-draw pipeline GL state helpers that don't naturally fit into
// a per-mode bundle (blend / depth / cull have their own headers).
//
// These cover the "miscellaneous" parts of `ApplyPipeline` and
// `BeginPass`'s FULL-INIT path: color-mask, polygon-offset (decal
// path), depth-test / depth-clamp toggles, and the clear-color
// uniform.  Each helper is named so its precondition and effect
// are obvious from the callsite; raw `glEnable` / `glDisable` /
// `glColorMask` / `glPolygonOffset` / `glClearColor` are removed
// from the backend.

namespace Poseidon
{
namespace render::pipeline
{

inline void SetColorMask(bool write)
{
    const GLboolean v = write ? GL_TRUE : GL_FALSE;
    glColorMask(v, v, v, v);
}

// Polygon-offset for OnSurface decals (coplanar disambiguation).
// When enabled, factor/units default to (-1, -1)
// which matches the legacy "push fragment forward by 1 unit" decal
// bias.  Callers that need a different offset should add a new
// named helper rather than parameterise this one.
inline void SetPolygonOffsetForDecals(bool enabled)
{
    if (enabled)
    {
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(-1.0f, -1.0f);
    }
    else
    {
        glDisable(GL_POLYGON_OFFSET_FILL);
    }
}

// Polygon-offset for ground-projected shadows.  Shadows are coplanar
// with the receiver and rely on a depth bias to win the LessEqual depth
// test.  The decal offset's slope (factor) term shrinks toward 0 as the
// view approaches top-down, so at steep / 3rd-person angles the bias
// vanishes and the shadow loses the depth test and disappears.  A large
// constant (units) term keeps the bias angle-independent so the shadow
// survives at all view angles.
inline void SetPolygonOffsetForShadows(bool enabled)
{
    if (enabled)
    {
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(-1.0f, -64.0f);
    }
    else
    {
        glDisable(GL_POLYGON_OFFSET_FILL);
    }
}

// Depth-test / depth-clamp toggles for BeginPass's FULL-INIT and
// queue-flush transitions.  Pair them because both are reset at
// pass boundaries.
inline void EnableDepthTest()
{
    glEnable(GL_DEPTH_TEST);
}
inline void DisableDepthClamp()
{
    glDisable(GL_DEPTH_CLAMP);
}
inline void EnableDepthClamp()
{
    glEnable(GL_DEPTH_CLAMP);
}

// glClearColor wrapper — packs the four channels for callers that
// have them as floats already.  No semantic logic; this exists so
// the audit can assert `glClearColor` lives only in the helper
// header.
inline void SetClearColor(float r, float g, float b, float a)
{
    glClearColor(r, g, b, a);
}

} // namespace render::pipeline

} // namespace Poseidon
