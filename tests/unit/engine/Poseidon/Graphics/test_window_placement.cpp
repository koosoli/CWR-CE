// Unit tests for ResolveWindowPlacement — the pure decision function
// that maps DisplayConfig + desktop bounds to (mode, size, pos,
// refresh).  Lives without SDL so every config × desktop case is
// covered cheaply.
//
// The bug that motivated the resolver: borderless was previously just
// "drop the border" — it ignored sizing and produced a chromeless
// window at the configured (often sub-desktop) resolution, so DWM
// couldn't engage independent flip and users saw a tiny floating
// flat_quadangle.  Tests below pin "borderless = full desktop @ (0,0)".

#include <catch2/catch_test_macros.hpp>

#include <Poseidon/Graphics/Shared/WindowPlacement.hpp>
#include <catch2/catch_message.hpp>
#include <initializer_list>
#include <string>

using Poseidon::WindowMode;
using Poseidon::WindowPlacement;

using Poseidon::DisplayPlacementInput;
using Poseidon::kBorderlessRightEdgeOffset;

namespace
{
DisplayPlacementInput MakeCfg(const char* mode, int w, int h, int refresh = 0)
{
    DisplayPlacementInput cfg;
    cfg.displayMode = mode;
    cfg.width = w;
    cfg.height = h;
    cfg.refresh = refresh;
    return cfg;
}
} // namespace

TEST_CASE("Borderless ignores configured size and covers the desktop", "[Graphics][WindowPlacement]")
{
    // Desktop is 1920x1080@144; cfg asks for a tiny 800x600 borderless.
    // Resolver must override: width is desktopW + kBorderlessRightEdgeOffset
    // (1 on Windows for SDL #12791, 0 elsewhere — see the dedicated
    // regression case below), height is desktop, position (0,0),
    // refresh = desktop.
    auto cfg = MakeCfg("borderless", 800, 600);
    auto p = ResolveWindowPlacement(cfg, 1920, 1080, 144);

    REQUIRE(p.mode == WindowMode::Borderless);
    REQUIRE(p.width == 1920 + kBorderlessRightEdgeOffset);
    REQUIRE(p.height == 1080);
    REQUIRE(p.posX == 0);
    REQUIRE(p.posY == 0);
    REQUIRE(p.refreshHz == 144);
}

TEST_CASE("Borderless uses desktop+offset width even when cfg matches", "[Graphics][WindowPlacement]")
{
    auto cfg = MakeCfg("borderless", 1920, 1080);
    auto p = ResolveWindowPlacement(cfg, 1920, 1080, 60);

    REQUIRE(p.mode == WindowMode::Borderless);
    REQUIRE(p.width == 1920 + kBorderlessRightEdgeOffset);
    REQUIRE(p.height == 1080);
    REQUIRE(p.posX == 0);
    REQUIRE(p.posY == 0);
}

TEST_CASE("Borderless flat_quad != monitor flat_quad on Windows (SDL #12791 workaround)",
          "[Graphics][WindowPlacement][regression]")
{
    // Pinned regression for the SDL #12791 promotion: a borderless
    // OpenGL window whose bounds match the monitor's *exactly* gets
    // silently promoted to exclusive fullscreen by Win11+driver on
    // the first SDL_GL_SwapWindow, which bypasses DWM and suppresses
    // the shell OSD (volume / Alt-Tab / Snip / Discord overlay).
    //
    // The resolver MUST emit `desktopW + kBorderlessRightEdgeOffset`
    // — that's 1 on Windows (flat_quad != monitor flat_quad, DWM stays
    // composing), 0 everywhere else (compositors handle borderless
    // full-monitor cleanly on their own).  Verify across a few common
    // desktop sizes — any regression that reverts the offset lands
    // here as a hard failure on the affected platform.
    //   https://github.com/libsdl-org/SDL/issues/12791
    struct Case
    {
        int dw, dh;
    };
    for (auto c : {Case{1920, 1080}, Case{2560, 1440}, Case{3840, 2160}, Case{1366, 768}})
    {
        auto cfg = MakeCfg("borderless", 0, 0);
        auto p = ResolveWindowPlacement(cfg, c.dw, c.dh, 60);
        INFO("desktop " << c.dw << "x" << c.dh << " (kBorderlessRightEdgeOffset=" << kBorderlessRightEdgeOffset << ")");
        CHECK(p.mode == WindowMode::Borderless);
        CHECK(p.width == c.dw + kBorderlessRightEdgeOffset);
        CHECK(p.height == c.dh);
        CHECK(p.posX == 0);
        CHECK(p.posY == 0);
    }

#ifdef _WIN32
    // Windows-specific: the flat_quad MUST differ from the monitor flat_quad.
    auto cfg = MakeCfg("borderless", 0, 0);
    auto p = ResolveWindowPlacement(cfg, 1920, 1080, 60);
    CHECK(p.width != 1920);
    static_assert(kBorderlessRightEdgeOffset == 1, "SDL #12791 workaround requires +1 px on Windows");
#else
    static_assert(kBorderlessRightEdgeOffset == 0,
                  "Non-Windows platforms compose borderless cleanly; no offset expected");
#endif
}

TEST_CASE("Windowed honours cfg size and centers", "[Graphics][WindowPlacement]")
{
    auto cfg = MakeCfg("windowed", 1280, 720);
    auto p = ResolveWindowPlacement(cfg, 1920, 1080, 144);

    REQUIRE(p.mode == WindowMode::Windowed);
    REQUIRE(p.width == 1280);
    REQUIRE(p.height == 720);
    REQUIRE(p.posX == WindowPlacement::kCentered);
    REQUIRE(p.posY == WindowPlacement::kCentered);
    // Windowed never asks for a refresh rate — the desktop owns it.
    REQUIRE(p.refreshHz == 0);
}

TEST_CASE("Windowed clamps oversized configs to desktop", "[Graphics][WindowPlacement]")
{
    // Someone copied OFP.cfg from a 4K rig to a 1080p laptop.
    auto cfg = MakeCfg("windowed", 3840, 2160);
    auto p = ResolveWindowPlacement(cfg, 1920, 1080, 60);

    REQUIRE(p.mode == WindowMode::Windowed);
    REQUIRE(p.width == 1920);
    REQUIRE(p.height == 1080);
}

TEST_CASE("Windowed substitutes 800x600 when cfg size is zero", "[Graphics][WindowPlacement]")
{
    auto cfg = MakeCfg("windowed", 0, 0);
    auto p = ResolveWindowPlacement(cfg, 1920, 1080, 60);

    REQUIRE(p.mode == WindowMode::Windowed);
    REQUIRE(p.width == 800);
    REQUIRE(p.height == 600);
}

TEST_CASE("Exclusive honours cfg size and refresh", "[Graphics][WindowPlacement]")
{
    auto cfg = MakeCfg("exclusive", 1280, 720, 240);
    auto p = ResolveWindowPlacement(cfg, 1920, 1080, 144);

    REQUIRE(p.mode == WindowMode::Fullscreen);
    REQUIRE(p.width == 1280);
    REQUIRE(p.height == 720);
    REQUIRE(p.posX == 0);
    REQUIRE(p.posY == 0);
    // Exclusive is the only mode where cfg.refresh wins over desktop.
    REQUIRE(p.refreshHz == 240);
}

TEST_CASE("Exclusive falls back to desktop refresh when cfg has none", "[Graphics][WindowPlacement]")
{
    auto cfg = MakeCfg("exclusive", 1920, 1080, 0);
    auto p = ResolveWindowPlacement(cfg, 1920, 1080, 144);

    REQUIRE(p.mode == WindowMode::Fullscreen);
    REQUIRE(p.refreshHz == 144);
}

TEST_CASE("Exclusive with zero size falls back to desktop size", "[Graphics][WindowPlacement]")
{
    auto cfg = MakeCfg("exclusive", 0, 0, 0);
    auto p = ResolveWindowPlacement(cfg, 1920, 1080, 60);

    REQUIRE(p.mode == WindowMode::Fullscreen);
    REQUIRE(p.width == 1920);
    REQUIRE(p.height == 1080);
}

TEST_CASE("Unknown mode string falls back to borderless", "[Graphics][WindowPlacement]")
{
    auto cfg = MakeCfg("totally-bogus", 800, 600);
    auto p = ResolveWindowPlacement(cfg, 1920, 1080, 60);

    REQUIRE(p.mode == WindowMode::Borderless);
    // Includes the SDL #12791 +1-column offset on Windows (see the
    // dedicated case); 0 elsewhere.
    REQUIRE(p.width == 1920 + kBorderlessRightEdgeOffset);
    REQUIRE(p.height == 1080);
}

TEST_CASE("Defensively handles 0x0 desktop bounds", "[Graphics][WindowPlacement]")
{
    // SDL very rarely reports a 0x0 desktop on remote-display setups.
    // The resolver must not pass through a 0-sized window.
    auto cfg = MakeCfg("borderless", 800, 600);
    auto p = ResolveWindowPlacement(cfg, 0, 0, 0);

    REQUIRE(p.width >= 1);
    REQUIRE(p.height >= 1);
}
