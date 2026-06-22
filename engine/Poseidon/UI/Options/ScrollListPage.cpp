#include <Poseidon/UI/Options/ScrollListPage.hpp>
#include <Poseidon/UI/Options/OptionsShell.hpp>

#include <Poseidon/Core/resincl.hpp>

#include <SDL3/SDL_keycode.h>

namespace Poseidon
{

int ScrollListPage::DefaultFocusIdc() const
{
    // First slot's value cell is the natural starting focus on a
    // scroll list — same as the old OptionsScrollList::FocusInitial.
    // Subclasses can override if their first row is a header.
    return 501;
}

void ScrollListPage::Mount(OptionsShell& shell)
{
    OptionsPage::Mount(shell);
    m_list = std::make_unique<OptionsScrollList>(shell, ProviderRef());
    m_list->RenderPage();
    m_list->FocusInitial();
}

void ScrollListPage::Unmount(OptionsShell& shell)
{
    m_list.reset();
    OptionsPage::Unmount(shell);
}

void ScrollListPage::OnReshown(OptionsShell& /*shell*/)
{
    // Shell un-hid every mounted IDC blindly; per-row visibility (e.g.
    // the left-aligned label is hidden on Action rows so the centred
    // stretched valueStep doesn't double up) needs re-applying.
    if (m_list)
    {
        m_list->RenderPage();
        m_list->RestoreRetainedFocus();
    }
}

bool ScrollListPage::OnButtonClicked(OptionsShell& shell, int idc)
{
    if (idc == IDC_CANCEL)
    {
        shell.PopPage();
        return true;
    }
    if (m_list && m_list->OnButtonClicked(idc))
        return true;
    return false;
}

bool ScrollListPage::OnKeyDown(OptionsShell& shell, unsigned nChar)
{
    if (nChar == SDLK_ESCAPE)
    {
        shell.PopPage();
        return true;
    }
    if (m_list && m_list->OnKeyDown(nChar))
        return true;
    return false;
}

void ScrollListPage::OnSimulate(OptionsShell& shell)
{
    if (m_list)
        m_list->OnSimulate();
}

} // namespace Poseidon
