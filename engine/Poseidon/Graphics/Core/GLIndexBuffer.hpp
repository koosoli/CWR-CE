#pragma once

#include <glad/gl.h>

#ifndef NDEBUG
#include <Poseidon/Dev/Debug/DebugTrap.hpp>
#endif

// IBO bind that enforces the active-VAO precondition.
//
// `glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ...)` in core profile only
// has effect when a non-default VAO is currently bound — the IBO
// binding is recorded as part of the active VAO's state.  Binding
// the IBO with VAO 0 either silently no-ops or attaches to the
// global default VAO that no draw uses; either way the engine sees
// a flood of `GL_INVALID_OPERATION` errors from subsequent draws.
//
// This helper is the unique callsite of `glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ...)`
// in the engine.  Callers pass the IBO handle and the helper does
// the bind; in debug builds, the helper asserts that the current
// VAO is non-zero before issuing the bind, so a missing `glBindVertexArray`
// upstream surfaces immediately rather than as a delayed
// GL_INVALID_OPERATION from the next draw.
//
// The function name documents the precondition.  A new IBO bind site in the
// backend must either route through this helper (and inherit the assertion) or
// be added to the audit allow-list explicitly.  See render-invariants.md.

namespace Poseidon
{
namespace render::ibo
{

inline void BindOnActiveVao(GLuint ibo)
{
#ifndef NDEBUG
    GLint currentVao = 0;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &currentVao);
    if (currentVao == 0)
    {
        // VAO 0 — default VAO; binding an IBO here is the misuse this guard
        // catches.  Trap so it halts at the offending callsite rather than at
        // the next glDrawElements.
        BREAK();
    }
#endif
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
}

} // namespace render::ibo

} // namespace Poseidon
