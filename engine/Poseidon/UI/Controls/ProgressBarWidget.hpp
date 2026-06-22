#pragma once

// Agnostic determinate progress bar, drawn as a track + fill on a notebook —
// the same track+fill mechanism the Options sliders use (see
// OptionsScrollList::SetSliderBar).  Any screen that needs a progress bar
// mounts a fill control over a static track and calls RenderProgressBar()
// each frame with a fraction in [0,1].  The MODS addon download (two bars:
// current addon + all addons) and the multiplayer single-mission transfer
// (one bar) both reuse this.
//
// Stateless: the caller owns the fraction (e.g. from DownloadProgress) and
// calls RenderProgressBar() each frame.  The pure geometry lives in
// ProgressFillRect() and the readout in FormatProgressPercent() so both can
// be unit-tested without a live notebook.

#include <Poseidon/UI/Controls/UIControls.hpp>

#include <algorithm>
#include <cstdio>
#include <string>

namespace Poseidon
{
struct ProgressBarLayout
{
    float trackX = 0.0f;
    float trackY = 0.0f;
    float trackW = 0.0f;
    float trackH = 0.0f;
};

struct ProgressFillGeom
{
    float x;
    float y;
    float w;
    float h;
};

// Fill rectangle for a fraction in [0,1].  Clamps out-of-range fractions so a
// stale or garbage value can never paint outside the track.
inline ProgressFillGeom ProgressFillRect(const ProgressBarLayout& layout, float fraction01)
{
    const float f = std::clamp(fraction01, 0.0f, 1.0f);
    return {layout.trackX, layout.trackY, layout.trackW * f, layout.trackH};
}

inline void RenderProgressBar(ControlObjectContainer& notebook, int idcFill, const ProgressBarLayout& layout,
                              float fraction01)
{
    const ProgressFillGeom g = ProgressFillRect(layout, fraction01);
    notebook.SetSubControlPos(idcFill, g.x, g.y, g.w, g.h);
}

// "0%".."100%" — single source of truth for the percent readout next to a bar.
inline std::string FormatProgressPercent(float fraction01)
{
    const int pct = static_cast<int>(std::clamp(fraction01, 0.0f, 1.0f) * 100.0f + 0.5f);
    char buf[8];
    snprintf(buf, sizeof(buf), "%d%%", pct);
    return buf;
}

} // namespace Poseidon
