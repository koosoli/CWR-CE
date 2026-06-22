#include <Poseidon/Graphics/Shared/WindowPlacement.hpp>

#include <algorithm>

namespace Poseidon
{

namespace
{
int ClampPositive(int v, int lo, int hi)
{
    if (v < lo)
        return lo;
    if (v > hi)
        return hi;
    return v;
}
} // namespace

WindowPlacement ResolveWindowPlacement(const DisplayPlacementInput& cfg, int desktopW, int desktopH, int desktopRefresh)
{
    // Clamp desktop bounds defensively — a backend that reports 0×0
    // would otherwise produce a 0-sized window in Borderless.
    if (desktopW <= 0)
        desktopW = 1;
    if (desktopH <= 0)
        desktopH = 1;
    if (desktopRefresh < 0)
        desktopRefresh = 0;

    WindowPlacement p;

    if (cfg.displayMode == "windowed")
    {
        p.mode = WindowMode::Windowed;
        // Clamp so an over-sized config doesn't spawn a window that runs
        // off-screen.  Anything < 1 falls back to a usable default.
        const int w = (cfg.width > 0) ? cfg.width : 800;
        const int h = (cfg.height > 0) ? cfg.height : 600;
        p.width = ClampPositive(w, 1, desktopW);
        p.height = ClampPositive(h, 1, desktopH);
        p.posX = WindowPlacement::kCentered;
        p.posY = WindowPlacement::kCentered;
        p.refreshHz = 0;
        return p;
    }

    if (cfg.displayMode == "exclusive")
    {
        p.mode = WindowMode::Fullscreen;
        // Don't clamp — the driver may legitimately pick a non-desktop mode
        // (the whole point of exclusive).  Just guarantee >= 1.
        p.width = (cfg.width > 0) ? cfg.width : desktopW;
        p.height = (cfg.height > 0) ? cfg.height : desktopH;
        p.posX = 0;
        p.posY = 0;
        p.refreshHz = (cfg.refresh > 0) ? cfg.refresh : desktopRefresh;
        return p;
    }

    // Default / unknown -> Borderless desktop fullscreen.  Cover the
    // monitor at native resolution.  On Windows the bounding rect is
    // extended one column past the monitor's right edge so the rect
    // doesn't match the monitor's rect exactly — this prevents the
    // WGL driver's "borderless full-monitor OpenGL window" promotion
    // to exclusive-fullscreen scanout (libsdl-org/SDL #12791) which
    // bypasses DWM and suppresses shell overlays.  See the
    // `kBorderlessRightEdgeOffset` declaration in WindowPlacement.hpp
    // for the per-platform value and the SDL issue link.
    //
    // Caveat (Windows-only because the offset is 0 elsewhere): with a
    // second monitor immediately to the right of the primary at
    // (desktopW, 0), the extra column lands on the adjacent monitor
    // and a 1-pixel sliver of the game is visible there.  Multi-
    // monitor topology resolution is out of scope for this pure
    // resolver — single-monitor desktops are the dominant case and the
    // caller can post-process the placement if it needs more
    // sophisticated layout-aware behaviour.
    p.mode = WindowMode::Borderless;
    p.width = desktopW + kBorderlessRightEdgeOffset;
    p.height = desktopH;
    p.posX = 0;
    p.posY = 0;
    p.refreshHz = desktopRefresh;
    return p;
}

} // namespace Poseidon
