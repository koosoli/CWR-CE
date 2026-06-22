#include <Poseidon/UI/Options/FlatMenuPage.hpp>
#include <Poseidon/UI/Options/OptionsShell.hpp>

#include <Poseidon/Core/resincl.hpp>
#include <Poseidon/Input/InputSubsystem.hpp>

#include <SDL3/SDL_keycode.h>

namespace Poseidon
{

bool FlatMenuPage::OnButtonClicked(OptionsShell& shell, int idc)
{
    if (idc == CloseIdc() || idc == IDC_CANCEL)
    {
        shell.PopPage();
        return true;
    }
    bool handled = OnNav(shell, idc);
    if (handled && ContainsCycleIdc(idc, m_cycle, m_cycleSize))
        m_lastNavIdc = idc;
    return handled;
}

void FlatMenuPage::OnReshown(OptionsShell& shell)
{
    // Only restore when the user has actually navigated into a child
    // before — first-mount focus stays at PushPage's DefaultFocusIdc
    // (typically the top row).
    if (m_lastNavIdc >= 0)
        FocusCycleIdc(shell, m_lastNavIdc, m_cycle, m_cycleSize);
}

bool FlatMenuPage::OnKeyDown(OptionsShell& shell, unsigned nChar)
{
    if (nChar == SDLK_ESCAPE)
    {
        shell.PopPage();
        return true;
    }
    return WrapFocus(shell, nChar, m_cycle, m_cycleSize);
}

void FlatMenuPage::OnSimulate(OptionsShell& shell)
{
    auto& in = InputSubsystem::Instance();
    float cx = in.GetCursorX();
    float cy = in.GetCursorY();
    float mouseX = 0.5f + cx * 0.5f;
    float mouseY = 0.5f + cy * 0.5f;
    bool firstSample = (m_lastCursorX < -1.0f);
    bool moved = !firstSample && ((cx != m_lastCursorX) || (cy != m_lastCursorY));
    m_lastCursorX = cx;
    m_lastCursorY = cy;
    if (!moved)
        return;

    auto* notebook = shell.GetNotebook();
    if (!notebook)
        return;

    int hoveredIdc = -1;
    if (IControl* hovered = notebook->GetCtrl(mouseX, mouseY); hovered && hovered != notebook)
        hoveredIdc = hovered->IDC();
    if (hoveredIdc < 0)
        return;

    FocusCycleIdc(shell, hoveredIdc, m_cycle, m_cycleSize);
}

} // namespace Poseidon
