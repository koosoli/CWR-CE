// ViewerController unit tests — exercise Poll() with a mock provider so
// the controller's input-mapping logic is verified without an SDL
// window or InputSubsystem singleton.
//
// What's covered:
//   - LMB drag → translateX/Y; RMB drag → rotateX/Y; MMB drag → zoom
//   - Wheel and MMB drag both feed the zoom axis
//   - `[`/`]` produce animScrub ±1 (held, not edge)
//   - Edge actions (Esc/F5/Space/R/O) consume their scancode exactly
//     once and the matching ViewerControls bool fires for that frame
//   - `?` (Shift+/) edge-detects via the controller's instance latch
//     so a held combo only toggles once

#include <catch2/catch_test_macros.hpp>

#include <Poseidon/Input/ViewerController.hpp>
#include <SDL3/SDL_scancode.h>

using namespace Poseidon;
namespace
{
// Mock provider — public state setters drive Poll().  Tracks consumed
// keys so tests can assert Esc/F5/etc. ConsumeKeyPress was called.
struct MockProvider : public IViewerInputProvider
{
    bool lmb = false, rmb = false, mmb = false;
    float dx = 0.0f, dy = 0.0f;
    float wheel = 0.0f;
    bool keysDown[SDL_SCANCODE_COUNT] = {};
    bool keysPressed[SDL_SCANCODE_COUNT] = {};
    bool consumed[SDL_SCANCODE_COUNT] = {};

    bool MouseLeft() const override { return lmb; }
    bool MouseRight() const override { return rmb; }
    bool MouseMiddle() const override { return mmb; }
    float MouseDeltaX() const override { return dx; }
    float MouseDeltaY() const override { return dy; }
    float ConsumeWheel() override
    {
        float w = wheel;
        wheel = 0.0f;
        return w;
    }
    bool KeyDown(SDL_Scancode k) const override { return keysDown[k]; }
    bool KeyPressed(SDL_Scancode k) const override { return keysPressed[k]; }
    void ConsumeKeyPress(SDL_Scancode k) override
    {
        consumed[k] = true;
        keysPressed[k] = false;
    }
};
} // namespace

TEST_CASE("ViewerController: idle input produces zero controls", "[Input][ViewerController]")
{
    MockProvider p;
    ViewerController c;
    auto out = c.Poll(p);
    CHECK(out.translateX == 0.0f);
    CHECK(out.translateY == 0.0f);
    CHECK(out.rotateX == 0.0f);
    CHECK(out.rotateY == 0.0f);
    CHECK(out.zoom == 0.0f);
    CHECK(out.animScrub == 0.0f);
    CHECK(out.playPauseAnim == false);
    CHECK(out.exitViewer == false);
}

TEST_CASE("ViewerController: LMB drag produces translateX/Y", "[Input][ViewerController]")
{
    MockProvider p;
    p.lmb = true;
    p.dx = 12.0f;
    p.dy = -3.5f;
    ViewerController c;
    auto out = c.Poll(p);
    CHECK(out.translateX == 12.0f);
    CHECK(out.translateY == -3.5f);
    // Only LMB axis lit — RMB / MMB / scroll all zero.
    CHECK(out.rotateX == 0.0f);
    CHECK(out.rotateY == 0.0f);
    CHECK(out.zoom == 0.0f);
}

TEST_CASE("ViewerController: RMB drag produces rotateX/Y", "[Input][ViewerController]")
{
    MockProvider p;
    p.rmb = true;
    p.dx = 7.0f;
    p.dy = 4.0f;
    ViewerController c;
    auto out = c.Poll(p);
    CHECK(out.rotateX == 7.0f);
    CHECK(out.rotateY == 4.0f);
    CHECK(out.translateX == 0.0f);
    CHECK(out.zoom == 0.0f);
}

TEST_CASE("ViewerController: wheel feeds zoom and is consumed", "[Input][ViewerController]")
{
    MockProvider p;
    p.wheel = 2.5f;
    ViewerController c;
    auto out = c.Poll(p);
    CHECK(out.zoom == 2.5f);
    // Second poll same frame would see zero — provider drained on consume.
    auto out2 = c.Poll(p);
    CHECK(out2.zoom == 0.0f);
}

TEST_CASE("ViewerController: MMB drag feeds pan, not zoom", "[Input][ViewerController]")
{
    MockProvider p;
    p.mmb = true;
    p.dx = 8.0f;
    p.dy = -3.0f;
    ViewerController c;
    auto out = c.Poll(p);
    // MMB now drives pan (camera moves in screen plane) — wheel
    // owns zoom exclusively.
    CHECK(out.panX == 8.0f);
    CHECK(out.panY == -3.0f);
    CHECK(out.zoom == 0.0f);
    CHECK(out.translateX == 0.0f);
    CHECK(out.rotateX == 0.0f);
}

TEST_CASE("ViewerController: wheel and MMB are independent axes", "[Input][ViewerController]")
{
    MockProvider p;
    p.mmb = true;
    p.dy = -10.0f;
    p.wheel = 1.0f;
    ViewerController c;
    auto out = c.Poll(p);
    // Wheel feeds zoom only; MMB drag fills panX/panY.  No
    // cross-talk between the two.
    CHECK(out.zoom == 1.0f);
    CHECK(out.panY == -10.0f);
}

TEST_CASE("ViewerController: bracket keys produce animScrub +/-1", "[Input][ViewerController]")
{
    MockProvider p;
    ViewerController c;

    p.keysDown[SDL_SCANCODE_RIGHTBRACKET] = true;
    CHECK(c.Poll(p).animScrub == 1.0f);

    p.keysDown[SDL_SCANCODE_RIGHTBRACKET] = false;
    p.keysDown[SDL_SCANCODE_LEFTBRACKET] = true;
    CHECK(c.Poll(p).animScrub == -1.0f);

    // Both held → cancel.
    p.keysDown[SDL_SCANCODE_RIGHTBRACKET] = true;
    CHECK(c.Poll(p).animScrub == 0.0f);
}

TEST_CASE("ViewerController: edge actions consume their scancode", "[Input][ViewerController]")
{
    MockProvider p;
    ViewerController c;

    auto check = [&](SDL_Scancode k, auto getter)
    {
        // Reset consumed flag, set the press, poll, expect the action
        // bool to fire and the key to be consumed.
        p.consumed[k] = false;
        p.keysPressed[k] = true;
        auto out = c.Poll(p);
        CHECK(getter(out) == true);
        CHECK(p.consumed[k] == true);
        // Provider's IsKeyPressed is now false (ConsumeKeyPress cleared it),
        // so the next poll wouldn't re-fire.
        auto out2 = c.Poll(p);
        CHECK(getter(out2) == false);
    };

    check(SDL_SCANCODE_SPACE, [](auto& o) { return o.playPauseAnim; });
    check(SDL_SCANCODE_R, [](auto& o) { return o.resetAnim; });
    check(SDL_SCANCODE_O, [](auto& o) { return o.openAnim; });
    check(SDL_SCANCODE_F5, [](auto& o) { return o.reloadTextures; });
    check(SDL_SCANCODE_ESCAPE, [](auto& o) { return o.exitViewer; });
}

TEST_CASE("ViewerController: Shift+/ edge-detects toggleHelp", "[Input][ViewerController]")
{
    MockProvider p;
    ViewerController c;

    // Initial press of Shift+/ → toggleHelp once.
    p.keysDown[SDL_SCANCODE_SLASH] = true;
    p.keysDown[SDL_SCANCODE_LSHIFT] = true;
    CHECK(c.Poll(p).toggleHelp == true);
    // Held → no further toggle this frame chain.
    CHECK(c.Poll(p).toggleHelp == false);
    CHECK(c.Poll(p).toggleHelp == false);

    // Release shift, repress → fires again.
    p.keysDown[SDL_SCANCODE_LSHIFT] = false;
    CHECK(c.Poll(p).toggleHelp == false);
    p.keysDown[SDL_SCANCODE_LSHIFT] = true;
    CHECK(c.Poll(p).toggleHelp == true);
}

TEST_CASE("ViewerController: F1 edge-detects toggleHelp regardless of shift", "[Input][ViewerController]")
{
    // F1 is the universal binding so non-US-layout users (Czech / DE /
    // FR — where SDL_SCANCODE_SLASH is not the `?` key) can still
    // toggle the help overlay.
    MockProvider p;
    ViewerController c;

    p.keysDown[SDL_SCANCODE_F1] = true;
    CHECK(c.Poll(p).toggleHelp == true);
    // Held → no further toggle.
    CHECK(c.Poll(p).toggleHelp == false);

    // Release, repress → fires again.
    p.keysDown[SDL_SCANCODE_F1] = false;
    CHECK(c.Poll(p).toggleHelp == false);
    p.keysDown[SDL_SCANCODE_F1] = true;
    CHECK(c.Poll(p).toggleHelp == true);
}

TEST_CASE("ViewerController: Esc never leaks past consume", "[Input][ViewerController]")
{
    // Regression guard for the "Esc takes down the game" failure mode:
    // the very first frame Esc is pressed, ConsumeKeyPress must fire so
    // any game-side handler running later in the same frame's input
    // pump doesn't see the key.
    MockProvider p;
    ViewerController c;
    p.keysPressed[SDL_SCANCODE_ESCAPE] = true;
    auto out = c.Poll(p);
    CHECK(out.exitViewer == true);
    CHECK(p.consumed[SDL_SCANCODE_ESCAPE] == true);
}
