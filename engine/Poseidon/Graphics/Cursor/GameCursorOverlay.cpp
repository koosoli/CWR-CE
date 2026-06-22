#include <Poseidon/Graphics/Cursor/GameCursorOverlay.hpp>

#include <Poseidon/UI/UIActiveDisplay.hpp>
#include <Poseidon/UI/InGame/InGameUI.hpp>
#include <Poseidon/AI/VehicleAI.hpp>
#include <Poseidon/World/World.hpp>
#include <Poseidon/World/Scene/Scene.hpp>
#include <Poseidon/World/Scene/Camera/Camera.hpp>

namespace Poseidon
{

void GameCursorOverlay::Draw(Engine* /*engine*/)
{
    if (!_world)
        return;

    // Layer 1 — warning message sits above any dialog.
    if (auto* warn = _world->GetWarningMessage())
    {
        warn->DrawCursor();
        return;
    }

    // Layer 2 — topmost open dialog that actually owns a cursor.  A
    // cursor-less overlay (the CapsLock VoIP channel / chat HUDs call
    // SetCursor(nullptr)) is skipped so it doesn't blank the menu cursor
    // underneath it.
    if (auto* cur = UIActiveDisplay::FindTopmostWithCursor(_world))
    {
        cur->DrawCursor();
        return;
    }

    // A dialog is open but none of the stack owns a cursor (e.g. an
    // intro/cutscene map that called SetCursor(nullptr)).  That display
    // intentionally suppresses the cursor — don't fall through to the in-game
    // cursor below.
    if (UIActiveDisplay::FindTopmost(_world))
        return;

    // Layer 3 — no dialog open.  In-game UI gets to draw the seagull
    // cursor when the camera is on a non-AI vehicle (player free
    // cam, intro scene, replay viewer).  InGameUI::DrawHUDNonAI's
    // body is itself just the cursor — guarded by HasOptions() — so
    // dispatching here keeps the same behaviour.
    AbstractUI* gameUI = _world->GetUI();
    if (!gameUI)
        return;
    Object* camObj = _world->CameraOn();
    Entity* camV = dyn_cast<Entity, Object>(camObj);
    EntityAI* camAI = dyn_cast<EntityAI, Entity>(camV);
    if (camV && !camAI)
    {
        gameUI->DrawHUDNonAI(*GScene->GetCamera(), camV, _world->GetCameraType());
    }
    // camAI case: weapon/target cursor lives inside InGameUI::DrawHUD
    // (the AI HUD pass); we don't try to relocate it here because
    // DrawMouseCursor reads private state that DrawHUD prepares
    // earlier in the same call.  Documented limitation.
}

} // namespace Poseidon
