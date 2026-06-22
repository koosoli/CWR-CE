#pragma once

#include <Poseidon/Graphics/Core/Engine.hpp>

namespace Poseidon
{
namespace Presentation
{

enum class RectKind
{
    PhysicalViewport,
    Logical2DViewport,
    Safe4x3,
    Safe16x9,
    Safe21x9,
    HudSafe16x9,
    HudSafe21x9,
    TitleSafe16x9,
};

struct Rect
{
    float x = 0.0f;
    float y = 0.0f;
    float w = 1.0f;
    float h = 1.0f;
};

Rect PhysicalViewportRect();
Rect Logical2DRect(const AspectSettings& aspect);
Rect CenterRatioRect(float targetRatio, int viewportWidth, int viewportHeight);
Rect ResolveRect(RectKind kind, int viewportWidth, int viewportHeight, const AspectSettings& aspect);

const char* RectKindName(RectKind kind);
bool ParseRectKind(const char* name, RectKind& out);

} // namespace Presentation
} // namespace Poseidon
