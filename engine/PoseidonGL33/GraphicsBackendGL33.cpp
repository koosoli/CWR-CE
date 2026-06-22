#include <Poseidon/Graphics/GraphicsEngineFactory.hpp>
#include <Poseidon/Graphics/Core/EngineFactory.hpp>
#include <Poseidon/Graphics/Shared/WindowMetrics.hpp>
using Poseidon::Engine;

using Poseidon::GraphicsBackendDescriptor;

using Poseidon::GraphicsEngineParams;
using Poseidon::GraphicsEngineFactory;

using Poseidon::CreateEngineGL33;
using Poseidon::WindowMetrics;

namespace
{
Engine* CreateGL33Backend(const GraphicsEngineParams& params)
{
    return CreateEngineGL33(params.width, params.height, params.useWindow, params.bitsPerPixel);
}

bool IsGL33Available()
{
    return true;
}
} // namespace

namespace Poseidon {
void RegisterGL33GraphicsBackend()
{
    GraphicsEngineFactory::Register(GraphicsBackendDescriptor{
        "gl33",
        "OpenGL 3.3 Core (SDL3)",
        100,
        &CreateGL33Backend,
        &IsGL33Available,
    });
}
} // namespace Poseidon
