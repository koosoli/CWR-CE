#pragma once

// PoseidonOptionsTest — microphone-test modal page.
//
// Pushed by the Audio screen's "Test Microphone" action row.  Confirms
// the input chain end-to-end: mic device → samples reach the engine →
// playback path → speakers.  In the production build this drives a
// capture-to-playback loopback via VoNCapture + a streaming OpenAL
// source; here in the mock the bar is fed by the same synthetic
// envelope the AudioPage's Input Level row uses.
//
// Single-button modal — Close (or Esc / IDC_CANCEL) pops the page.

#include <Poseidon/UI/Options/ButtonModalPage.hpp>
#include <Poseidon/Audio/Voice/VonCapture.hpp>
#include <Poseidon/Audio/Voice/MicLoopback.hpp>


namespace Poseidon
{
class MicTestPage : public ButtonModalPage
{
public:
	MicTestPage() : ButtonModalPage({kIdcClose}, kIdcClose) {}

	int         DefaultFocusIdc()   const override { return kIdcClose; }
	const char* ResourceClassName() const override { return "RscOptionsPageMicTest"; }

	void Mount(OptionsShell& shell) override;
	void Unmount(OptionsShell& shell) override;
	void OnSimulate(OptionsShell& shell) override;

protected:
	void OnResolved(OptionsShell& /*shell*/, int /*activatedIdc*/) override {}

private:
	static constexpr int kIdcClose = 9501;

	float m_level = 0.0f;   // 0..100, smoothed
	float m_peak  = 0.0f;   // 0..100, decays

	VoNCapture  m_capture;
	MicLoopback m_loopback;
};

} // namespace Poseidon
