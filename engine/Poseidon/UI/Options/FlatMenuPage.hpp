#pragma once

// PoseidonOptionsTest — base for any flat-menu page (Index, Graphics,
// Controls, future Credits / Jimbo destinations).
//
// Subclass passes its nav cycle to the base ctor.  The last entry is
// the close row.  OnNav(idc) is called for every nav-row click except
// the close row.  The base handles:
//   - Esc / IDC_CANCEL / close-row click → shell.PopPage()
//   - Up / Down / Left / Right wraparound across the cycle
//
// Focus memory: the base remembers the last nav row the user activated
// and restores focus to it on OnReshown — so popping back from a child
// page lands the cursor on the row that opened the child rather than
// resetting to the page's default.  First mount (no prior nav) keeps
// the initial PushPage / DefaultFocusIdc focus intact.  Subclasses that
// override OnReshown must chain to the base.

#include <Poseidon/UI/Options/OptionsPage.hpp>


namespace Poseidon
{
class FlatMenuPage : public OptionsPage
{
public:
	bool OnButtonClicked(OptionsShell& shell, int idc) final;
	bool OnKeyDown(OptionsShell& shell, unsigned nChar) final;
	void OnSimulate(OptionsShell& shell) final;
	void OnReshown(OptionsShell& shell) override;

protected:
	FlatMenuPage(const int* cycle, int cycleSize)
		: m_cycle(cycle), m_cycleSize(cycleSize) {}

	int CloseIdc() const { return m_cycleSize > 0 ? m_cycle[m_cycleSize - 1] : -1; }

	// Called for every nav-row click *except* the close row.  Subclass
	// pushes the corresponding child page or no-ops.
	virtual bool OnNav(OptionsShell& shell, int idc) = 0;

private:
	const int* m_cycle;
	int        m_cycleSize;
	float      m_lastCursorX = -2.0f;
	float      m_lastCursorY = -2.0f;
	int        m_lastNavIdc  = -1;   // -1 = no prior nav, leaves DefaultFocusIdc untouched
};

} // namespace Poseidon
