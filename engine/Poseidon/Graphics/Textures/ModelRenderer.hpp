#pragma once

#include <vector>
#include <cstdint>
#include <string>

namespace Poseidon
{
class Shape;
}
using Poseidon::Shape;

namespace Poseidon
{

// Optional log callback for debugging (set before calling RenderP3DFile)
using ModelRendererLogFunc = void (*)(const char* msg);
void SetModelRendererLog(ModelRendererLogFunc func);

struct RenderedModel
{
    std::vector<uint8_t> rgb; // 3 bytes per pixel
    int width = 0;
    int height = 0;
    bool valid() const { return width > 0 && height > 0 && !rgb.empty(); }
};

// Render wireframe of a Shape LOD to RGB pixel buffer
// view: "front", "back", "top", "bottom", "right", "left", "3d", "quad"
// bgR/bgG/bgB: background color (default white)
RenderedModel RenderModelWireframe(Shape* lod, int imgW, int imgH, const std::string& view = "quad", uint8_t bgR = 255,
                                   uint8_t bgG = 255, uint8_t bgB = 255);

// Load P3D file and render LOD 0 wireframe (convenience)
RenderedModel RenderP3DFile(const std::string& path, int imgW, int imgH, const std::string& view = "quad",
                            int lodIndex = 0, uint8_t bgR = 255, uint8_t bgG = 255, uint8_t bgB = 255);

} // namespace Poseidon
