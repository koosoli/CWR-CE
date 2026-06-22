#pragma once

#include <Poseidon/Graphics/Rendering/Frame/SceneInputs.hpp>

// Forward decls to keep the header light.

namespace Poseidon
{
class Engine;
namespace render::frame
{

// Impure adapter: read live engine state into a value-typed
// `SceneInputs` snapshot.  The *only* place the frame layer reads
// non-value inputs.  Tests don't go through this — they construct
// `SceneInputs` directly so BuildFrame stays unit-testable without the
// engine.
SceneInputs ExtractSceneInputs(const Engine& engine, const Scene& scene);

} // namespace render::frame

} // namespace Poseidon
