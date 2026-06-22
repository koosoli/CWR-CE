#pragma once

// Redundant-bind elimination for the GL33 backend (perf effort 03).
// Every glBindVertexArray / glBindTexture(GL_TEXTURE_2D) in the backend
// routes through these helpers so the cache always reflects GL reality;
// anything that touches bind state outside them (ImGui overlay, context
// reset) must call Invalidate(). Deletion of a cached object must call
// the matching On*Deleted — GL implicitly unbinds deleted objects.
//
// B-007 invariant (test_gl_state_cache_audit): a state cache is only
// admissible with explicit invalidation at every side-effecting site.

namespace Poseidon
{
namespace GL33Bind
{

void Vao(unsigned int vao);
void Tex2D(int unit, unsigned int tex); // leaves the active unit = `unit`
void ActiveUnit(int unit);
void OnVaoDeleted(unsigned int vao);
void OnTexDeleted(unsigned int tex);
void Invalidate();
// True iff the GL-level cache agrees that `unit` is bound to `tex`.
// OnTexDeleted keeps this accurate across texture deletion and handle recycling.
bool IsTexBound(int unit, unsigned int tex);

// Test/diagnostic counters: total requests vs actual GL calls.
unsigned long long VaoRequests();
unsigned long long VaoBinds();
unsigned long long TexRequests();
unsigned long long TexBinds();

} // namespace GL33Bind
} // namespace Poseidon
