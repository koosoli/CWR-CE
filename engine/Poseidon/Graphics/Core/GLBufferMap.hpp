#pragma once

#include <glad/gl.h>

// Buffer-mapping helpers with the usage hint baked into the API name.
//
// `glMapBufferRange` takes a runtime `GLbitfield` of access flags, of which
// `GL_MAP_INVALIDATE_BUFFER_BIT` is one.  Passing INVALIDATE on a
// `GL_STATIC_DRAW` buffer contradicts the static promise; NVIDIA reacts by
// demoting the buffer to host memory (KHR_debug perf warning 131186, frame-time
// regression).
//
// The wrong combination is made unrepresentable: there is no
// public API in this header that maps a static buffer with INVALIDATE,
// or maps a dynamic buffer without it.  Callers pick one of two named
// functions based on the buffer's actual usage hint, and the GL flag
// bundle each one passes is locked at the callsite.
//
// `MapDynamicWriteInvalidate` — for buffers created with
//   `GL_DYNAMIC_DRAW`.  Bakes in `GL_MAP_WRITE_BIT |
//   GL_MAP_INVALIDATE_BUFFER_BIT`.  The INVALIDATE flag is what makes
//   the dynamic path stall-free (driver hands back fresh storage
//   while the GPU finishes with the old contents).
//
// `MapStaticWriteOnce` — for buffers created with `GL_STATIC_DRAW`.
//   Uses plain `glMapBuffer(... GL_WRITE_ONLY)`.  No INVALIDATE flag;
//   the driver-side optimisation that flag enables doesn't apply to
//   a buffer the driver placed in VRAM and intends to leave there.
//
// There is intentionally *no* third helper.  Any combination of "STATIC buffer
// + INVALIDATE flag" or "DYNAMIC buffer + no INVALIDATE" is the bug class this
// header prevents; making it representable re-opens the regression path.

namespace Poseidon
{
namespace render::buf
{

inline void* MapDynamicWriteInvalidate(GLenum target, GLintptr offset, GLsizeiptr length)
{
    constexpr GLbitfield kFlags = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;
    return glMapBufferRange(target, offset, length, kFlags);
}

inline void* MapStaticWriteOnce(GLenum target)
{
    return glMapBuffer(target, GL_WRITE_ONLY);
}

} // namespace render::buf

} // namespace Poseidon
