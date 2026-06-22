#pragma once

// Engine-wide initialization front door for the Poseidon layer.
//
// Each app (Game, Server, Viewer, Tools, …) calls Poseidon::InitDefaults()
// from main() after the engine foundation is up.  It registers every
// "default callback" impl that lives in this layer — script engine
// archive/info functions, ParamFile CRC, ProgressSystem alive callback,
// SDL clipboard — through the corresponding free Init*() functions
// owned by each TU.
//
// Explicit Init*() calls are used instead of namespace-scope constructor
// side effects (the `} GFoo;` pattern), which are vulnerable to:
//   (1) the linker dropping a TU when no other symbol references it, and
//   (2) cross-TU static initialization order races that can clobber a
//       registered impl back to the no-op base.
//
// Calls here are idempotent and safe to invoke multiple times.

namespace Poseidon
{
void InitDefaults();
}

