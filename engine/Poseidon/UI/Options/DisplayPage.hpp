#pragma once

// Display settings page for the production OptionsShell.
//
// Restart-required settings — changes don't take effect until the
// user explicitly hits Apply.  After Apply, push a ConfirmRevert
// modal with a 15 s countdown so a misconfigured display can roll
// back without leaving the user staring at a black screen.
//
// Row layout:
//   0 Monitor          Stepper   Adapter / output picker
//   1 Window Mode      Stepper   Borderless / Exclusive Fullscreen / Windowed
//   2 Resolution       Stepper   filtered by selected monitor;
//                                disabled in Borderless
//   3 Refresh Rate     Stepper   filtered by (monitor, resolution);
//                                enabled only in Exclusive Fullscreen
//   4 Aspect Handling  Stepper   Adaptive / Original stretched HUD
//   5 HUD Width Limit  Stepper   Off / 21:9 / 16:9, enabled only in Adaptive
//   6 Apply            Action    enabled only when m_pending != m_applied
// + Close              Action    appended by WithCloseRow
//
// State model:
//   m_pending — what the UI rows show.  Bound directly to row Get/Set.
//   m_applied — what the engine + display.cfg currently hold.
//
// Mount snapshots live engine state into both.  Each row change writes
// only m_pending.  Apply flushes m_pending → engine + ConfirmRevertPage;
// Keep saves to display.cfg, Revert / timeout / Esc restores from
// m_applied.  Esc on the page itself with un-Applied changes silently
// reverts UI (no engine touch — engine still holds m_applied).

#include <Poseidon/UI/Settings/DisplayConfig.hpp>
#include <Poseidon/UI/Settings/AspectRatio.hpp>
#include <Poseidon/UI/Options/ScrollListPage.hpp>
#include <Poseidon/Graphics/Core/Engine.hpp>   // MonitorInfo, ResolutionInfo, WindowMode

#include <array>
#include <string>
#include <utility>
#include <vector>


namespace Poseidon
{
class DisplayPage : public ScrollListPage
{
public:
	const char* TitleText() const override;

    static int WindowModeToUiIndex(DisplayConfig::WindowMode mode);
    static DisplayConfig::WindowMode UiIndexToWindowMode(int index);
    static int StyleToUiIndex(AspectRatio::DisplayStyle style);
    static AspectRatio::DisplayStyle UiIndexToStyle(int index);
    static int ClampToUiIndex(AspectRatio::UltrawideClamp clamp);
    static AspectRatio::UltrawideClamp UiIndexToClamp(int index);
    static bool HasPendingChanges(const DisplayConfig& pending, const DisplayConfig& applied);

	void Mount(OptionsShell& shell) override;
	void OnReshown(OptionsShell& shell) override;
	void Unmount(OptionsShell& shell) override;

protected:
	OptionsScrollList::Provider& ProviderRef() override { return m_provider; }

private:
	static const char* CloseLabel();
	static const char* CloseDescription();

	class DisplayProvider : public OptionsScrollList::Provider
	{
	public:
		enum : int {
			kRowMonitor    = 0,
			kRowWindowMode = 1,
			kRowResolution = 2,
			kRowRefresh    = 3,
			kRowStyle      = 4,
			kRowClamp      = 5,
			kRowApply      = 6,
			kRowCount      = 7,
		};

		// Owner sets these on Mount, after enumerating engine state.
		void SetPage(class DisplayPage* page) { m_page = page; }

		int  RowCount() const override         { return kRowCount; }
		const char* RowLabel(int row) const override;
		const char* RowDescription(int row) const override;
		OptionsScrollList::RowDef RowFor(int row) const override;
		int  RowValue(int row) const override;
		void SetRowValue(int row, int v) override;

		OptionsScrollList::Kind RowKind(int row) const override
		{
			return (row == kRowApply) ? OptionsScrollList::KindAction
			                          : OptionsScrollList::Provider::RowKind(row);
		}

		bool IsDisabled(int row) const override;
		bool IsPending(int row) const override;
		void OnRowAction(int row, Display& host) override;

	private:
		DisplayPage* m_page = nullptr;
	};

	// Snapshot the engine's current monitor / window mode / resolution /
	// refresh into both m_applied and m_pending, and rebuild the cached
	// option lists (monitors / resolutions / refresh rates / labels).
	void CaptureEngineState();

	// Refresh the cached option lists for whatever m_pending currently
	// references — call after SetRowValue on Monitor or Resolution
	// because the available resolution / refresh-rate lists narrow.
	void RebuildLists();

	// Push m_pending to the live engine in the correct order.
	void ApplyToEngine(const DisplayConfig& cfg);

	// Save m_applied to <userDir>/display.cfg.
	void SaveToCfg() const;

	void RefreshLocalizedChoices();

	DisplayConfig                   m_pending;
	DisplayConfig                   m_applied;
	DisplayProvider                 m_display;
	OptionsScrollList::WithCloseRow m_provider{m_display, CloseLabel(), CloseDescription()};

	// Cached lists, rebuilt by RebuildLists().  Two parallel arrays per
	// stepper row: the typed data + a c_str array the OptionsScrollList
	// row template consumes.
	std::vector<MonitorInfo>             m_monitors;
	std::vector<std::string>             m_monitorLabels;
	std::vector<const char*>             m_monitorCStrs;

	// Window mode is a fixed 3-entry list — declared here so RowFor can
	// hand back stable c_str pointers.
	std::array<std::string, 3>            m_windowModeLabels;
	std::array<const char*, 3>            m_windowModeCStrs{};
    std::array<std::string, 2>           m_styleLabels;
    std::array<const char*, 2>           m_styleCStrs{};
    std::array<std::string, 3>           m_clampLabels;
    std::array<const char*, 3>           m_clampCStrs{};

	std::vector<std::pair<int, int>>     m_resolutions;     // (W, H) without the "Native" sentinel
	std::vector<std::string>             m_resolutionLabels; // index 0 = "Native", then (W×H) per entry
	std::vector<const char*>             m_resolutionCStrs;

	std::vector<int>                     m_refreshRates;     // without the "System default" 0 entry
	std::vector<std::string>             m_refreshLabels;    // index 0 = "System default", then "<n> Hz"
	std::vector<const char*>             m_refreshCStrs;

	friend class DisplayProvider;   // Provider's row getters read these vectors
};

} // namespace Poseidon
