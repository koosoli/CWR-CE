#include <Poseidon/Graphics/Cursor/ViewerCursorOverlay.hpp>

#include <Poseidon/Graphics/Core/Engine.hpp>
#include <Poseidon/Graphics/IGraphicsEngine.hpp>
#include <Poseidon/Graphics/Textures/TextureBank.hpp>
#include <Poseidon/Input/InputSubsystem.hpp>

namespace Poseidon
{

void ViewerCursorOverlay::Draw(Engine* engine)
{
    if (!engine || !engine->TextBank())
        return;

    MipInfo white = engine->TextBank()->UseMipmap(nullptr, 0, 0);

    auto& input = InputSubsystem::Instance();
    float ndcX = input.GetCursorX();
    float ndcY = input.GetCursorY();
    int cx = (int)((ndcX * 0.5f + 0.5f) * engine->Width2D());
    int cy = (int)((ndcY * 0.5f + 0.5f) * engine->Height2D());

    // Compact rectangle cursor — 9×9 white square with 1-px black halo
    // and a single-pixel hot point.  Reads cleanly against any
    // background (the halo gives contrast on light terrain, the
    // hot-point dot gives a precise focus).  Square shape was the
    // explicit ask — rings looked busy, the rect is closer to a
    // standard pointer.
    const PackedColor fg(Color(1.0f, 1.0f, 1.0f, 1.0f));
    const PackedColor halo(Color(0.0f, 0.0f, 0.0f, 0.85f));

    // Halo (11×11) underneath
    engine->Draw2D(white, halo, Rect2DPixel(cx - 5, cy - 5, 11, 11));
    // White rect on top with a 1-px halo gap on each side (so the
    // halo reads as an outline).
    engine->Draw2D(white, fg, Rect2DPixel(cx - 4, cy - 4, 9, 9));
    // Central hot-point pixel — black against the white square so
    // the user can see exactly where the click would land.
    engine->Draw2D(white, halo, Rect2DPixel(cx, cy, 1, 1));
}

void ViewerCursorOverlay::ToggleLock(Engine* engine)
{
    _locked = !_locked;
    if (engine)
        engine->SetMouseGrab(_locked);
}

} // namespace Poseidon
