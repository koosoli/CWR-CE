#pragma once

#include <Poseidon/UI/Controls/UIControls.hpp>
#include <Poseidon/UI/OptionsUI.hpp>
#include <Poseidon/UI/Map/UIMap.hpp>

#include <Poseidon/World/World.hpp>


namespace Poseidon
{
namespace UIActiveDisplay
{

inline ControlsContainer* WalkToDeepest(AbstractOptionsUI* ui)
{
    auto* cur = dynamic_cast<ControlsContainer*>(ui);
    if (!cur)
        return nullptr;

    while (cur->Child())
        cur = cur->Child();

    if (auto* display = dynamic_cast<Display*>(cur))
    {
        if (auto* msg = display->GetMsgBox())
            return msg;
    }

    return cur;
}

inline bool AcceptsCursor(AbstractOptionsUI* ui)
{
    auto* cur = dynamic_cast<ControlsContainer*>(ui);
    while (cur)
    {
        auto* display = dynamic_cast<Display*>(cur);
        if (!display || display->IsDisplayEnabled() || dynamic_cast<DisplayMap*>(display) ||
            dynamic_cast<DisplayNotebook*>(display))
            return true;

        cur = cur->Child();
    }

    return false;
}

inline ControlsContainer* FindTopmost(World* world)
{
    if (!world)
        return nullptr;

    AbstractOptionsUI* stack[] = {
        world->GetVoiceChat(),
        world->GetChat(),
        world->GetChannel(),
        world->GetUserDlg(),
        world->HasMap() ? world->Map() : nullptr,
        world->GetOptions(),
    };

    for (AbstractOptionsUI* ui : stack)
    {
        auto* cur = WalkToDeepest(ui);
        if (cur && AcceptsCursor(ui))
            return cur;
    }

    return nullptr;
}

// Like FindTopmost, but for choosing which container draws the SOFTWARE CURSOR.
// A transient overlay that called SetCursor(nullptr) (the CapsLock VoIP channel
// / chat HUDs) owns no cursor texture: it must not steal the cursor from the
// menu underneath, or the cursor vanishes while the overlay is up.  Skip any
// container without a cursor texture and fall through to the next, so the
// parent menu's cursor stays visible.  (FindTopmost is unchanged — it still
// resolves the topmost display for input routing / harness queries.)
inline ControlsContainer* FindTopmostWithCursor(World* world)
{
    if (!world)
        return nullptr;

    AbstractOptionsUI* stack[] = {
        world->GetVoiceChat(),
        world->GetChat(),
        world->GetChannel(),
        world->GetUserDlg(),
        world->HasMap() ? world->Map() : nullptr,
        world->GetOptions(),
    };

    for (AbstractOptionsUI* ui : stack)
    {
        auto* cur = WalkToDeepest(ui);
        if (cur && AcceptsCursor(ui) && cur->GetCursorTexture())
            return cur;
    }

    return nullptr;
}

// Resolve the container that actually owns a given control IDC, walking the
// same UI stack as FindTopmost.  FindTopmost returns a single "topmost"
// container, which during a briefing can be a transient channel/chat HUD that
// sits above a modal map child (e.g. the server get-ready dialog).  When the
// topmost container does not own the IDC, the harness still needs to reach the
// dialog that does — so scan every stack entry's deepest container and return
// the first that holds the IDC.
inline ControlsContainer* FindContainerWithIdc(World* world, int idc)
{
    if (!world)
        return nullptr;

    AbstractOptionsUI* stack[] = {
        world->GetVoiceChat(),
        world->GetChat(),
        world->GetChannel(),
        world->GetUserDlg(),
        world->HasMap() ? world->Map() : nullptr,
        world->GetOptions(),
    };

    for (AbstractOptionsUI* ui : stack)
    {
        auto* cur = WalkToDeepest(ui);
        if (cur && cur->GetCtrl(idc))
            return cur;
    }

    return nullptr;
}

} // namespace UIActiveDisplay

}  // namespace Poseidon
