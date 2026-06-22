#pragma once

// Display user-settings persisted to <user-dir>/display.cfg.
//
// Sibling to AudioConfig — same Defaults / Normalize(env) / Load / Save
// pattern.  Restart-required settings (those that need to recreate the
// swapchain or move the window between monitors): monitor index,
// window mode, resolution, refresh rate.  Live-apply quality knobs
// land in graphics.cfg later, separately.
//
// Persistence semantics differ from AudioConfig in one place: Display
// changes only flow to disk when the user explicitly hits Apply on
// the Display screen and confirms via the ConfirmRevert modal.  Boot
// still eager-writes defaults if the file is missing, and Normalize
// at boot still doesn't persist (a temporarily disconnected monitor
// keeps its remembered name pending reconnection).
//
// Field conventions:
//   monitor = 0          → primary
//   windowMode = Borderless → safe default; works on every system
//                            without enumerating modes first
//   resolutionWidth = 0  → "native / system default" sentinel; same
//                            idea as AudioConfig's outputDevice = ""
//   resolutionHeight = 0 → see above; W and H are stored independently
//                            but always normalize together (one zero
//                            means both treated as native)
//   refreshRate = 0      → system default; only meaningful in Fullscreen
//                            (in Borderless / Windowed the OS owns the rate)

#include <Poseidon/UI/Settings/AspectRatio.hpp>

#include <string>
#include <utility>
#include <vector>


namespace Poseidon
{
class DisplayConfig
{
public:
	enum WindowMode
	{
		Fullscreen = 0,
		Borderless = 1,
		Windowed   = 2,
	};

	// Persistable fields.  Public for direct access from tests + the
	// DisplayPage's pending/applied bookkeeping; invariants enforced by
	// Normalize, not by setters.
	int        monitor          = 0;
	WindowMode windowMode       = Borderless;
	int                         resolutionWidth  = 0;                // 0 → native
	int                         resolutionHeight = 0;                // 0 → native
	int                         refreshRate      = 0;                // 0 → system default
	AspectRatio::DisplayStyle   displayStyle     = AspectRatio::Modern;
	AspectRatio::UltrawideClamp ultrawideClamp   = AspectRatio::Clamp21x9;

	// Reset every field to factory defaults.
	void LoadDefaults();

	// Validation against the runtime environment.  Out-of-range monitor
	// indices reset to 0 (primary); unknown window modes reset to
	// Borderless; resolutions not present in the selected monitor's
	// supported list reset to (0, 0); refresh rates not supported at
	// the selected resolution reset to 0.  Each field validates
	// independently — an invalid resolution must not also wipe the
	// refresh rate.  Returns true if any field was changed.
	struct Environment
	{
		virtual ~Environment() = default;
		virtual int GetMonitorCount() const = 0;

		// List of (width, height) pairs supported by the given monitor.
		// Empty list means "any resolution is valid" (Dummy backend);
		// (0, 0) is always valid regardless of list contents (native).
		virtual std::vector<std::pair<int, int>> ListResolutions(int monitorIdx) const = 0;

		// List of refresh rates supported at (monitor, w, h).  Empty
		// means "any rate is valid".  0 is always valid (system default).
		virtual std::vector<int> ListRefreshRates(int monitorIdx, int w, int h) const = 0;
	};
	bool Normalize(const Environment& env);

	// True on successful parse, false if the file doesn't exist or is
	// unparseable.  Same posture as AudioConfig::Load — leaves the
	// instance untouched on failure so callers can chain
	// Load → LoadDefaults → Save.
	bool Load(const std::string& path);

	// Writes via ParamFile.  Returns false on I/O error.
	bool Save(const std::string& path) const;

	// Curated window sizes offered in Windowed mode.  Borderless / Fullscreen
	// extents are owned by the OS / the monitor's mode list; Windowed is the one
	// mode where a small fixed size (800x600 and friends) is a sensible, portable
	// choice independent of the desktop's exclusive-fullscreen modes.  A persisted
	// Windowed resolution validates against this list even when it is not a
	// fullscreen mode the monitor enumerates.
	static const std::vector<std::pair<int, int>>& WindowedSizes();
	static bool IsWindowedSize(int w, int h);

};

} // namespace Poseidon
