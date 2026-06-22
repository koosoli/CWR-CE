#include <Poseidon/UI/Options/ConfirmRevertPage.hpp>
#include <Poseidon/UI/Options/OptionsShell.hpp>

#include <Poseidon/Core/Global.hpp>
#include <Poseidon/UI/Locale/Stringtable/Stringtable.hpp>

#include <cstdio>
#include <Poseidon/Foundation/Framework/AppFrame.hpp>
#include <Poseidon/Foundation/Strings/RString.hpp>

namespace Poseidon
{

void ConfirmRevertPage::Mount(OptionsShell& shell)
{
    OptionsPage::Mount(shell);
    m_lastTickMs = Poseidon::Foundation::GlobalTickCount();
    UpdateCountdownText(shell);
}

void ConfirmRevertPage::UpdateCountdownText(OptionsShell& shell)
{
    int seconds = int(m_timeRemaining + 0.5f);
    if (seconds < 0)
        seconds = 0;
    char buf[64];
    snprintf(buf, sizeof(buf), LocalizeString("STR_DISP_MAIN_OPT_CONFIRM_REVERT_COUNTDOWN"), seconds);
    SetCtrlText(shell, kIdcCountdown, buf);
}

void ConfirmRevertPage::OnSimulate(OptionsShell& shell)
{
    if (IsResolved())
        return;

    // Keep / Revert highlight on mouse hover (this class overrides OnSimulate
    // for the countdown, so the base hover step has to be invoked explicitly).
    UpdateHoverFocus(shell);

    uint32_t now = Poseidon::Foundation::GlobalTickCount();
    float dt = float(now - m_lastTickMs) * 0.001f;
    m_lastTickMs = now;

    m_timeRemaining -= dt;
    if (m_timeRemaining <= 0.0f)
    {
        Resolve(shell, kIdcRevert);
        return;
    }
    UpdateCountdownText(shell);
}

void ConfirmRevertPage::OnResolved(OptionsShell& /*shell*/, int activatedIdc)
{
    if (activatedIdc == kIdcKeep)
        m_onKeep();
    else
        m_onRevert();
}

} // namespace Poseidon
