#include <Poseidon/UI/Options/ConfirmPage.hpp>

#include <Poseidon/UI/Options/OptionsShell.hpp>

namespace Poseidon
{

void ConfirmPage::Mount(OptionsShell& shell)
{
    OptionsPage::Mount(shell);
    SetCtrlText(shell, kIdcTitle, m_title.c_str());
    SetCtrlText(shell, kIdcBody, m_body.c_str());
    SetCtrlText(shell, kIdcYes, m_yesLabel.c_str());
    SetCtrlText(shell, kIdcNo, m_noLabel.c_str());
}

void ConfirmPage::OnResolved(OptionsShell& /*shell*/, int activatedIdc)
{
    if (activatedIdc == kIdcYes && m_onYes)
        m_onYes();
}

} // namespace Poseidon
