#pragma once

// Confirm-revert modal — pushed by destructive Apply paths (today
// the Display screen) to give the user a 15 s window to confirm a
// change actually works.  If the modal renders at all, the new mode
// is functional.  Keep persists; Revert / countdown / Esc rolls back.
//
// Standard pattern across modern games (CS2, ARMA 3, Cyberpunk).
//
// Caller wiring: Apply pushes the modal with two callbacks —
// onKeep gets called when the user clicks Keep; onRevert when they
// click Revert, hit Esc, or the countdown expires.  The caller is
// responsible for both the engine-state snapshot/restore and any
// cfg-file persistence; this page is purely the UI shell.

#include <Poseidon/UI/Options/ButtonModalPage.hpp>

#include <functional>


namespace Poseidon
{
class ConfirmRevertPage : public ButtonModalPage
{
public:
	ConfirmRevertPage(std::function<void()> onKeep,
	                  std::function<void()> onRevert,
	                  float timeoutSeconds = 15.0f)
		: ButtonModalPage({kIdcKeep, kIdcRevert}, kIdcRevert)
		, m_onKeep(std::move(onKeep))
		, m_onRevert(std::move(onRevert))
		, m_timeRemaining(timeoutSeconds)
	{}

	int         DefaultFocusIdc()   const override { return kIdcRevert; }
	const char* ResourceClassName() const override { return "RscOptionsPageConfirmRevert"; }

	void Mount(OptionsShell& shell) override;
	void OnSimulate(OptionsShell& shell) override;

protected:
	void OnResolved(OptionsShell& shell, int activatedIdc) override;

private:
	void UpdateCountdownText(OptionsShell& shell);

	static constexpr int kIdcKeep   = 9201;
	static constexpr int kIdcRevert = 9202;
	static constexpr int kIdcCountdown = 9281;

	std::function<void()> m_onKeep;
	std::function<void()> m_onRevert;
	float                 m_timeRemaining;
	uint32_t              m_lastTickMs = 0;
};

} // namespace Poseidon
