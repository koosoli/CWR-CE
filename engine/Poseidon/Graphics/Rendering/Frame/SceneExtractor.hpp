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

// Extract a specific value-owned view without changing Scene::_camera.  Primary
// reads the live scene camera; Auxiliary consumes the supplied RenderView.  An
// auxiliary extraction intentionally does not replay the recorded primary draw
// list: a later visibility-source milestone must collect it for that view.
SceneInputs ExtractSceneInputs(const Engine& engine, const Scene& scene, const RenderView& view);

} // namespace render::frame

} // namespace Poseidon
