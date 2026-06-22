#pragma once

// PoseidonOptionsTest — base for any modal page with a fixed cycle of
// focusable button IDCs.  One-button (info, "OK got it"), two-button
// (Yes/No, Keep/Revert), three-button (Save/Retry/Cancel modulo the
// caveat that CapturePage's Retry isn't terminal so it doesn't fit
// here) — all share the same ceremony: m_resolved guard, Esc → cancel,
// click / Enter on a button → resolve, arrow / Tab focus wrap.
//
// Subclass plugs in:
//   - the cycle: ordered IDCs of the buttons in tab/arrow order
//   - the cancel IDC: where Esc and IDC_CANCEL route
//   - OnResolved(shell, activatedIdc): fired once before the page pops
//
// The activated IDC is the one the user clicked / Entered on, OR the
// cancel IDC if dismissed via Esc / IDC_CANCEL.  Subclass decides what
// each IDC means semantically.

#include <Poseidon/UI/Options/OptionsPage.hpp>

#include <vector>


namespace Poseidon
{
class ButtonModalPage : public OptionsPage
{
public:
	const char* TitleText() const override { return ""; }
	bool        IsModal()   const override { return true; }

	bool OnButtonClicked(OptionsShell& shell, int idc) override;
	bool OnKeyDown(OptionsShell& shell, unsigned nChar) override;
	// Default per-frame step: mirror pointer hover into button focus so the
	// hovered button highlights like the scroll-list rows do.  Subclasses that
	// override OnSimulate (e.g. a countdown) should call UpdateHoverFocus too.
	void OnSimulate(OptionsShell& shell) override;

protected:
	// Focus whichever button IDC the cursor is over (fresh GetCtrl hit-test).
	void UpdateHoverFocus(OptionsShell& shell);

	// `buttons`: ordered IDCs in arrow / Tab cycle order (left→right or
	// top→bottom is the convention).  `cancelIdc` must be one of them —
	// it's where Esc and IDC_CANCEL route.  For single-button modals,
	// the cycle has one entry and cancelIdc equals it.
	ButtonModalPage(std::vector<int> buttons, int cancelIdc)
		: m_buttons(std::move(buttons)), m_cancelIdc(cancelIdc) {}

	// Fired exactly once before the page pops.  `activatedIdc` is the
	// button the user activated, or m_cancelIdc on Esc / IDC_CANCEL.
	virtual void OnResolved(OptionsShell& shell, int activatedIdc) = 0;

	// Resolve from the subclass — used by countdown timeouts etc.
	void Resolve(OptionsShell& shell, int activatedIdc);

	bool IsResolved() const { return m_resolved; }
	int  CancelIdc()  const { return m_cancelIdc; }

private:
	std::vector<int> m_buttons;
	int              m_cancelIdc;
	bool             m_resolved = false;
	float            m_lastCursorX = -2.0f;
	float            m_lastCursorY = -2.0f;
};

} // namespace Poseidon
