#include <Poseidon/UI/Options/ButtonModalPage.hpp>
#include <Poseidon/UI/Options/OptionsShell.hpp>

#include <Poseidon/Core/resincl.hpp>
#include <Poseidon/Input/InputSubsystem.hpp>
#include <Poseidon/UI/Controls/UIControls.hpp>

#include <SDL3/SDL_keycode.h>

namespace Poseidon
{

void ButtonModalPage::OnSimulate(OptionsShell& shell)
{
    UpdateHoverFocus(shell);
}

void ButtonModalPage::UpdateHoverFocus(OptionsShell& shell)
{
    if (m_resolved || m_buttons.empty())
        return;
    auto& in = InputSubsystem::Instance();
    float cx = in.GetCursorX();
    float cy = in.GetCursorY();
    bool firstSample = (m_lastCursorX < -1.0f);
    bool moved = !firstSample && ((cx != m_lastCursorX) || (cy != m_lastCursorY));
    m_lastCursorX = cx;
    m_lastCursorY = cy;
    if (!moved)
        return;
    auto* notebook = shell.GetNotebook();
    if (!notebook)
        return;
    // Hit-test the cursor fresh this frame (GetHoveredIdc lags a frame; see
    // OptionsScrollList::UpdateHoverFocus).
    float mouseX = 0.5f + cx * 0.5f;
    float mouseY = 0.5f + cy * 0.5f;
    int hoveredIdc = -1;
    if (IControl* hovered = notebook->GetCtrl(mouseX, mouseY); hovered && hovered != notebook)
        hoveredIdc = hovered->IDC();
    if (hoveredIdc < 0 || hoveredIdc == shell.GetFocusedNotebookIdc())
        return;
    for (int btn : m_buttons)
    {
        if (btn == hoveredIdc)
        {
            shell.FocusNotebookCtrl(btn);
            return;
        }
    }
}

bool ButtonModalPage::OnButtonClicked(OptionsShell& shell, int idc)
{
    if (m_resolved)
        return true;
    if (idc == IDC_CANCEL)
    {
        Resolve(shell, m_cancelIdc);
        return true;
    }
    for (int btn : m_buttons)
    {
        if (idc == btn)
        {
            Resolve(shell, btn);
            return true;
        }
    }
    return false;
}

bool ButtonModalPage::OnKeyDown(OptionsShell& shell, unsigned nChar)
{
    if (m_resolved)
        return false;
    if (nChar == SDLK_ESCAPE)
    {
        Resolve(shell, m_cancelIdc);
        return true;
    }
    if (m_buttons.empty())
        return false;
    return WrapFocus(shell, nChar, m_buttons.data(), int(m_buttons.size()), AnyAxis);
}

void ButtonModalPage::Resolve(OptionsShell& shell, int activatedIdc)
{
    if (m_resolved)
        return;
    m_resolved = true;
    OnResolved(shell, activatedIdc);
    shell.PopPage();
}

} // namespace Poseidon
