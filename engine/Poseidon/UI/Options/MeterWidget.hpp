#pragma once

// PoseidonOptionsTest — VU bar renderer.
//
// Two callers (so far):
//   - OptionsScrollList::UpdateMeter (Audio screen's input level row)
//   - MicTestPage::OnSimulate (mic-test modal)
//
// Both push a fill rectangle and a thin peak indicator across the same
// horizontal track.  The math is identical and easy to get subtly
// wrong (peak X clamping in particular), so it lives in one place.
//
// Stateless: callers own their level / peak smoothing and call
// RenderMeterBar() each frame with the current values.

#include <Poseidon/UI/Controls/UIControls.hpp>

#include <algorithm>


namespace Poseidon
{
struct MeterBarLayout
{
	float trackX;
	float trackY;
	float trackW;
	float trackH;
	float peakW;
};

inline void AdvanceMeterBar(float target01, float& level, float& peak)
{
	float target = std::clamp(target01, 0.0f, 1.0f) * 100.0f;

	level += (target - level) * 0.25f;
	level = std::clamp(level, 0.0f, 100.0f);

	if (level > peak)
		peak = level;
	else
		peak = std::max(0.0f, peak - 0.6f);
}

inline void RenderMeterBar(ControlObjectContainer& notebook,
                            int idcFill, int idcPeak,
                            const MeterBarLayout& layout,
                            float level01, float peak01)
{
	float level = std::clamp(level01, 0.0f, 1.0f);
	float peak  = std::clamp(peak01,  0.0f, 1.0f);

	notebook.SetSubControlPos(idcFill, layout.trackX, layout.trackY,
	                          layout.trackW * level, layout.trackH);

	float px = std::clamp(
		layout.trackX + peak * layout.trackW - layout.peakW * 0.5f,
		layout.trackX, layout.trackX + layout.trackW - layout.peakW);
	notebook.SetSubControlPos(idcPeak, px, layout.trackY, layout.peakW, layout.trackH);
}

} // namespace Poseidon
