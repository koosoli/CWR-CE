#pragma once

// Pure window-placement resolver.  Decides the final (mode, size,
// position, refresh) tuple from the engine's display config plus the
// target display's desktop mode.  Splitting this out from the SDL
// call site lets us unit-test every config × desktop combination
// without booting a window — and keeps the three modes on one rule
// table instead of three scattered branches.

#include <Poseidon/Graphics/Shared/WindowMode.hpp>

#include <climits>
#include <string>

namespace Poseidon
{
struct WindowPlacement
{
    static constexpr int kCentered = INT_MIN;

    WindowMode mode = WindowMode::Borderless;
    int width = 0;
    int height = 0;
    int posX = kCentered;
    int posY = kCentered;
    int refreshHz = 0; // 0 = let driver / SDL pick
};

// Extra column added to a Borderless window's *bounding rect* past the
// monitor's right edge so the rect doesn't match the monitor's rect
// exactly.  Without this, WGL drivers on Win10/11 silently promote a
// matching borderless OpenGL window to exclusive-fullscreen scanout on
// the next `SDL_GL_SwapWindow`, bypassing DWM and suppressing the
// shell volume OSD / Alt-Tab / Snip & Sketch / Discord overlay / OBS
// Game Capture, etc.  See libsdl-org/SDL #12791:
//   https://github.com/libsdl-org/SDL/issues/12791
//
// X11 / Wayland / Cocoa have no equivalent driver-side promotion (the
// compositor stays in the present chain on its own), so the offset is
// 0 there — the resolver still treats the value cleanly without needing
// a per-call platform branch, and the tests use the same constant so
// they stay green on every build target.
inline constexpr int kBorderlessRightEdgeOffset =
#ifdef _WIN32
    1
#else
    0
#endif
    ;

// Inputs from the engine's display config — passed as a POD so the
// resolver stays decoupled from any specific config struct.
struct DisplayPlacementInput
{
    int width = 0;
    int height = 0;
    int refresh = 0;
    std::string displayMode; // "windowed" / "borderless" / "exclusive"
};

// Rules:
//   "windowed"   -> Windowed,   w/h clamped to desktop, centered, refresh = 0
//   "borderless" -> Borderless, width = desktop + kBorderlessRightEdgeOffset
//                                (1 on Windows for SDL #12791, 0 elsewhere —
//                                see the constant declaration above),
//                                height = desktop, pos (0,0),
//                                refresh = desktop
//   "exclusive"  -> Fullscreen, w/h from cfg (>=1), pos (0,0),
//                                refresh = cfg.refresh ? cfg.refresh : desktop
//   anything else -> treated as "borderless" (matches default).
//
// desktopRefresh of 0 is allowed (some display backends don't report it);
// in that case Borderless leaves refreshHz = 0.
WindowPlacement ResolveWindowPlacement(const DisplayPlacementInput& cfg, int desktopW, int desktopH,
                                       int desktopRefresh);

} // namespace Poseidon
