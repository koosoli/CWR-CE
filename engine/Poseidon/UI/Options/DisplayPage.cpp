#include <Poseidon/UI/Options/DisplayPage.hpp>
#include <Poseidon/UI/Options/ConfirmRevertPage.hpp>
#include <Poseidon/UI/Options/OptionsShell.hpp>
#include <Poseidon/Foundation/Platform/AppConfig.hpp>

#include <Poseidon/Core/Global.hpp>
#include <Poseidon/Graphics/IGraphicsEngine.hpp>
#include <Poseidon/UI/Settings/AspectRatio.hpp>
#include <Poseidon/UI/Settings/Presentation.hpp>
#include <Poseidon/UI/Locale/Stringtable/Stringtable.hpp>
#include <Poseidon/Foundation/Common/GamePaths.hpp>

#include <SDL3/SDL_video.h>
#include <cstdio>
#include <memory>
#include <utility>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_stdinc.h>
#include <Poseidon/Foundation/Containers/Array.hpp>
#include <Poseidon/Foundation/Framework/Log.hpp>
#include <Poseidon/Foundation/Strings/RString.hpp>

namespace Poseidon
{

namespace
{
struct DisplayRowText
{
    const char* label;
    const char* desc;
};
const DisplayRowText kRows[] = {
    {"STR_DISP_MAIN_OPT_DISPLAY_MONITOR", "STR_DISP_MAIN_OPT_DISPLAY_MONITOR_DESC"},
    {"STR_DISP_MAIN_OPT_DISPLAY_WINDOW_MODE", "STR_DISP_MAIN_OPT_DISPLAY_WINDOW_MODE_DESC"},
    {"STR_DISP_MAIN_OPT_DISPLAY_RESOLUTION", "STR_DISP_MAIN_OPT_DISPLAY_RESOLUTION_DESC"},
    {"STR_DISP_MAIN_OPT_DISPLAY_REFRESH_RATE", "STR_DISP_MAIN_OPT_DISPLAY_REFRESH_RATE_DESC"},
    {"STR_DISP_MAIN_OPT_DISPLAY_STYLE", "STR_DISP_MAIN_OPT_DISPLAY_STYLE_DESC"},
    {"STR_DISP_MAIN_OPT_DISPLAY_ULTRAWIDE_CLAMP", "STR_DISP_MAIN_OPT_DISPLAY_ULTRAWIDE_CLAMP_DESC"},
    {"STR_DISP_MAIN_OPT_DISPLAY_APPLY", "STR_DISP_MAIN_OPT_DISPLAY_APPLY_DESC"},
};

std::string DisplayCfgPath()
{
    return GamePaths::Instance().UserDir() + "display.cfg";
}

int WindowModeToUiIndexImpl(DisplayConfig::WindowMode mode)
{
    switch (mode)
    {
        case DisplayConfig::Borderless:
            return 0; // "Fullscreen" in the UI
        case DisplayConfig::Fullscreen:
            return 1; // "Exclusive Fullscreen"
        case DisplayConfig::Windowed:
            return 2; // "Window"
    }
    return 0;
}

DisplayConfig::WindowMode UiIndexToWindowModeImpl(int index)
{
    switch (index)
    {
        case 0:
            return DisplayConfig::Borderless;
        case 1:
            return DisplayConfig::Fullscreen;
        case 2:
        default:
            return DisplayConfig::Windowed;
    }
}

SDL_DisplayID DisplayIdForMonitorIndex(int monitorIndex)
{
    int count = 0;
    SDL_DisplayID* displays = SDL_GetDisplays(&count);
    if (!displays)
        return SDL_GetPrimaryDisplay();

    SDL_DisplayID display = SDL_GetPrimaryDisplay();
    if (monitorIndex >= 0 && monitorIndex < count)
        display = displays[monitorIndex];
    SDL_free(displays);
    return display;
}

bool HasPendingChangesImpl(const DisplayConfig& pending, const DisplayConfig& applied)
{
    return pending.monitor != applied.monitor || pending.windowMode != applied.windowMode ||
           pending.resolutionWidth != applied.resolutionWidth || pending.resolutionHeight != applied.resolutionHeight ||
           pending.refreshRate != applied.refreshRate || pending.displayStyle != applied.displayStyle ||
           pending.ultrawideClamp != applied.ultrawideClamp;
}

void ApplyDisplayPolicy(int viewportWidth, int viewportHeight)
{
    // One aspect decision point for boot, resize and the Options apply
    // path alike — honors a live dev override and carries the world
    // crop rect, that a page-local mapper would otherwise drop.
    Poseidon::Presentation::Apply(viewportWidth, viewportHeight);
}
} // namespace

const char* DisplayPage::TitleText() const
{
    return LocalizeString("STR_DISP_MAIN_OPT_DISPLAY");
}

int DisplayPage::WindowModeToUiIndex(DisplayConfig::WindowMode mode)
{
    return WindowModeToUiIndexImpl(mode);
}

DisplayConfig::WindowMode DisplayPage::UiIndexToWindowMode(int index)
{
    return UiIndexToWindowModeImpl(index);
}

int DisplayPage::StyleToUiIndex(AspectRatio::DisplayStyle style)
{
    return style == AspectRatio::Legacy ? 1 : 0;
}

AspectRatio::DisplayStyle DisplayPage::UiIndexToStyle(int index)
{
    return index == 1 ? AspectRatio::Legacy : AspectRatio::Modern;
}

int DisplayPage::ClampToUiIndex(AspectRatio::UltrawideClamp clamp)
{
    switch (clamp)
    {
        case AspectRatio::Clamp21x9:
            return 1;
        case AspectRatio::Clamp16x9:
            return 2;
        case AspectRatio::ClampOff:
        default:
            return 0;
    }
}

AspectRatio::UltrawideClamp DisplayPage::UiIndexToClamp(int index)
{
    switch (index)
    {
        case 1:
            return AspectRatio::Clamp21x9;
        case 2:
            return AspectRatio::Clamp16x9;
        case 0:
        default:
            return AspectRatio::ClampOff;
    }
}

bool DisplayPage::HasPendingChanges(const DisplayConfig& pending, const DisplayConfig& applied)
{
    return HasPendingChangesImpl(pending, applied);
}

const char* DisplayPage::CloseLabel()
{
    return LocalizeString("STR_DISP_CLOSE");
}

const char* DisplayPage::CloseDescription()
{
    return LocalizeString("STR_DISP_MAIN_OPT_CLOSE_DESC");
}

// DisplayProvider — row getters / setters / actions.

const char* DisplayPage::DisplayProvider::RowLabel(int row) const
{
    static_assert(sizeof(kRows) / sizeof(kRows[0]) == kRowCount, "DisplayPage row table out of sync with kRowCount");
    if (row >= 0 && row < kRowCount)
        return LocalizeString(kRows[row].label);
    return "";
}

const char* DisplayPage::DisplayProvider::RowDescription(int row) const
{
    if (row >= 0 && row < kRowCount)
        return LocalizeString(kRows[row].desc);
    return "";
}

OptionsScrollList::RowDef DisplayPage::DisplayProvider::RowFor(int row) const
{
    if (!m_page)
        return {-1, nullptr, 0};
    switch (row)
    {
        case kRowMonitor:
            return {502, m_page->m_monitorCStrs.empty() ? nullptr : m_page->m_monitorCStrs.data(),
                    (int)m_page->m_monitorCStrs.size()};
        case kRowWindowMode:
            return {512, m_page->m_windowModeCStrs.data(), 3};
        case kRowResolution:
            return {522, m_page->m_resolutionCStrs.empty() ? nullptr : m_page->m_resolutionCStrs.data(),
                    (int)m_page->m_resolutionCStrs.size()};
        case kRowRefresh:
            return {532, m_page->m_refreshCStrs.empty() ? nullptr : m_page->m_refreshCStrs.data(),
                    (int)m_page->m_refreshCStrs.size()};
        case kRowStyle:
            return {542, m_page->m_styleCStrs.data(), (int)m_page->m_styleCStrs.size()};
        case kRowClamp:
            return {552, m_page->m_clampCStrs.data(), (int)m_page->m_clampCStrs.size()};
        case kRowApply:
            // Action row — no value cell.
            return {-1, nullptr, 0};
    }
    return {-1, nullptr, 0};
}

int DisplayPage::DisplayProvider::RowValue(int row) const
{
    if (!m_page)
        return 0;
    const DisplayConfig& p = m_page->m_pending;
    switch (row)
    {
        case kRowMonitor:
        {
            int n = (int)m_page->m_monitors.size();
            if (n <= 0)
                return 0;
            return (p.monitor >= 0 && p.monitor < n) ? p.monitor : 0;
        }
        case kRowWindowMode:
            return DisplayPage::WindowModeToUiIndex(p.windowMode);
        case kRowStyle:
            return DisplayPage::StyleToUiIndex(p.displayStyle);
        case kRowClamp:
            return DisplayPage::ClampToUiIndex(p.ultrawideClamp);
        case kRowResolution:
        {
            // Index 0 = "Native" (W=H=0 in the config).
            if (p.resolutionWidth <= 0 || p.resolutionHeight <= 0)
                return 0;
            for (size_t i = 0; i < m_page->m_resolutions.size(); ++i)
            {
                if (m_page->m_resolutions[i].first == p.resolutionWidth &&
                    m_page->m_resolutions[i].second == p.resolutionHeight)
                    return (int)i + 1;
            }
            return 0; // pending value not in current monitor's list → fall back to Native
        }
        case kRowRefresh:
        {
            if (p.refreshRate <= 0)
                return 0;
            for (size_t i = 0; i < m_page->m_refreshRates.size(); ++i)
                if (m_page->m_refreshRates[i] == p.refreshRate)
                    return (int)i + 1;
            return 0;
        }
        case kRowApply:
            return 0;
    }
    return 0;
}

void DisplayPage::DisplayProvider::SetRowValue(int row, int v)
{
    if (!m_page)
        return;
    DisplayConfig& p = m_page->m_pending;
    switch (row)
    {
        case kRowMonitor:
            if (v >= 0 && v < (int)m_page->m_monitors.size())
            {
                p.monitor = v;
                // Resolution / refresh lists depend on monitor — rebuild
                // and re-validate the pending values against the new lists.
                m_page->RebuildLists();
            }
            break;
        case kRowWindowMode:
        {
            switch (DisplayPage::UiIndexToWindowMode(v))
            {
                case DisplayConfig::Borderless:
                    p.windowMode = DisplayConfig::Borderless;
                    p.resolutionWidth = 0;
                    p.resolutionHeight = 0;
                    p.refreshRate = 0;
                    break;
                case DisplayConfig::Fullscreen:
                    p.windowMode = DisplayConfig::Fullscreen;
                    break;
                case DisplayConfig::Windowed:
                    p.windowMode = DisplayConfig::Windowed;
                    p.resolutionWidth = 0;
                    p.resolutionHeight = 0;
                    p.refreshRate = 0;
                    break;
            }
            m_page->RebuildLists();
            break;
        }
        case kRowStyle:
            p.displayStyle = DisplayPage::UiIndexToStyle(v);
            break;
        case kRowClamp:
            p.ultrawideClamp = DisplayPage::UiIndexToClamp(v);
            break;
        case kRowResolution:
            if (v == 0)
            {
                p.resolutionWidth = 0;
                p.resolutionHeight = 0;
            }
            else if (v - 1 < (int)m_page->m_resolutions.size())
            {
                p.resolutionWidth = m_page->m_resolutions[v - 1].first;
                p.resolutionHeight = m_page->m_resolutions[v - 1].second;
            }
            m_page->RebuildLists();
            break;
        case kRowRefresh:
            if (v == 0)
                p.refreshRate = 0;
            else if (v - 1 < (int)m_page->m_refreshRates.size())
                p.refreshRate = m_page->m_refreshRates[v - 1];
            break;
        case kRowApply:
            // Action — handled by OnRowAction, not SetRowValue.
            break;
    }
}

bool DisplayPage::DisplayProvider::IsDisabled(int row) const
{
    if (!m_page)
        return false;
    switch (row)
    {
        case kRowStyle:
            return false;
        case kRowClamp:
            return m_page->m_pending.displayStyle != AspectRatio::Modern;
        case kRowResolution:
            // Resolution = exclusive mode (Fullscreen) or window size (Windowed). Borderless
            // is always desktop-sized, so the row is inert there.
            return m_page->m_pending.windowMode == DisplayConfig::Borderless;
        case kRowRefresh:
            return m_page->m_pending.windowMode != DisplayConfig::Fullscreen;
        case kRowApply:
            return !DisplayPage::HasPendingChanges(m_page->m_pending, m_page->m_applied);
    }
    return false;
}

bool DisplayPage::DisplayProvider::IsPending(int row) const
{
    if (!m_page)
        return false;
    const DisplayConfig& p = m_page->m_pending;
    const DisplayConfig& a = m_page->m_applied;
    switch (row)
    {
        case kRowMonitor:
            return p.monitor != a.monitor;
        case kRowWindowMode:
            return p.windowMode != a.windowMode;
        case kRowStyle:
            return p.displayStyle != a.displayStyle;
        case kRowClamp:
            return p.ultrawideClamp != a.ultrawideClamp;
        case kRowResolution:
            return p.resolutionWidth != a.resolutionWidth || p.resolutionHeight != a.resolutionHeight;
        case kRowRefresh:
            return p.refreshRate != a.refreshRate;
    }
    return false;
}

void DisplayPage::DisplayProvider::OnRowAction(int row, Display& host)
{
    if (!m_page || row != kRowApply)
        return;
    if (IsDisabled(kRowApply))
        return; // no pending changes

    auto* shell = dynamic_cast<OptionsShell*>(&host);
    if (!shell)
        return;

    // Snapshot the current applied state so Revert can rewind cleanly
    // even if the user makes more changes between Apply and Keep/Revert
    // (which they can't today — the modal blocks UI input — but the
    // snapshot makes the revert path independent of any future change).
    DisplayConfig snapshot = m_page->m_applied;
    DisplayConfig pending = m_page->m_pending;

    // Push pending to the live engine.
    m_page->ApplyToEngine(pending);

    auto onKeep = [page = m_page, pending]()
    {
        page->m_applied = pending;
        page->SaveToCfg();
    };
    auto onRevert = [page = m_page, snapshot]()
    {
        page->ApplyToEngine(snapshot);
        page->m_pending = snapshot;
        page->m_applied = snapshot;
    };

    // Tests can shorten the 15s default via --confirm-revert-timeout so the
    // auto-revert assertion doesn't pay the full real-time wait.
    const float timeoutOverride = AppConfig::Instance().ConfirmRevertTimeoutSeconds();
    if (timeoutOverride > 0.f)
        shell->PushPage(std::make_unique<ConfirmRevertPage>(std::move(onKeep), std::move(onRevert), timeoutOverride));
    else
        shell->PushPage(std::make_unique<ConfirmRevertPage>(std::move(onKeep), std::move(onRevert)));
}

// DisplayPage — Mount / Unmount + cache rebuilds.

void DisplayPage::Mount(OptionsShell& shell)
{
    m_display.SetPage(this);
    CaptureEngineState();
    ScrollListPage::Mount(shell);
}

void DisplayPage::OnReshown(OptionsShell& shell)
{
    RefreshLocalizedChoices();
    m_provider.SetCloseTexts(CloseLabel(), CloseDescription());
    ScrollListPage::OnReshown(shell);
}

void DisplayPage::Unmount(OptionsShell& shell)
{
    // Pending changes that were never Applied: silently revert the UI
    // state (engine never moved because Apply was never pressed).  No
    // modal — the user opted out by leaving without Apply.
    m_pending = m_applied;
    ScrollListPage::Unmount(shell);
}

void DisplayPage::CaptureEngineState()
{
    if (!GEngine)
    {
        m_applied.LoadDefaults();
        m_pending = m_applied;
        RebuildLists();
        return;
    }

    // Monitors first — index drives the resolution-list scope.
    FindArray<MonitorInfo> mons;
    GEngine->ListMonitors(mons);
    m_monitors.clear();
    for (int i = 0; i < mons.Size(); ++i)
        m_monitors.push_back(mons[i]);

    m_applied.monitor = GEngine->GetCurrentMonitor();
    m_applied.windowMode = static_cast<DisplayConfig::WindowMode>(GEngine->GetCurrentWindowMode());

    // Engine's "current resolution" is exposed via the engine width /
    // height members; we capture them via the same ListResolutions —
    // but since there's no GetCurrentResolution accessor on the
    // abstract interface, leave the captured pending at the live engine
    // width by reading what's in display.cfg first.  In practice the
    // boot-time LoadAndApplyDisplayConfig already aligned engine state
    // with display.cfg, so reading from display.cfg here is the
    // least-surprising source of truth.
    {
        DisplayConfig fromFile;
        if (fromFile.Load(DisplayCfgPath()))
        {
            m_applied.resolutionWidth = fromFile.resolutionWidth;
            m_applied.resolutionHeight = fromFile.resolutionHeight;
            m_applied.refreshRate = fromFile.refreshRate;
            m_applied.displayStyle = fromFile.displayStyle;
            m_applied.ultrawideClamp = fromFile.ultrawideClamp;
        }
    }

    if (m_applied.windowMode == DisplayConfig::Borderless)
    {
        m_applied.resolutionWidth = 0;
        m_applied.resolutionHeight = 0;
        m_applied.refreshRate = 0;
    }
    else if (m_applied.windowMode == DisplayConfig::Windowed)
    {
        // Windowed keeps a user-chosen window size (resolutionWidth/Height, 0 = auto);
        // only the refresh rate is meaningless — the OS owns it in windowed.
        m_applied.refreshRate = 0;
    }
    m_pending = m_applied;
    ApplyDisplayPolicy(GEngine->Width(), GEngine->Height());
    RebuildLists();
}

void DisplayPage::RebuildLists()
{
    RefreshLocalizedChoices();

    // Monitors → label "Friendly name (W×H)".
    m_monitorLabels.clear();
    m_monitorCStrs.clear();
    for (size_t i = 0; i < m_monitors.size(); ++i)
    {
        const auto& m = m_monitors[i];
        char buf[160];
        const char* monitorName = LocalizeString("STR_DISP_MAIN_OPT_DISPLAY_MONITOR_FALLBACK");
        if (m.name.GetLength())
            monitorName = m.name.Data();
        snprintf(buf, sizeof(buf), LocalizeString("STR_DISP_MAIN_OPT_DISPLAY_MONITOR_ENTRY_FORMAT"), monitorName, m.w,
                 m.h);
        m_monitorLabels.emplace_back(buf);
    }
    for (auto& s : m_monitorLabels)
        m_monitorCStrs.push_back(s.c_str());

    // Resolutions for the pending-selected monitor.  Index 0 is the
    // "Native" sentinel; subsequent entries are the engine-reported list.
    m_resolutions.clear();
    m_resolutionLabels.clear();
    m_resolutionCStrs.clear();
    {
        FindArray<ResolutionInfo> res;
        if (m_pending.windowMode == DisplayConfig::Windowed)
        {
            // Windowed offers a curated set of basic window sizes (800x600 up). A window
            // must be *smaller* than the desktop (so it is a real window with the desktop
            // around it), so drop any size that matches or exceeds the monitor on either axis.
            int deskW = 0, deskH = 0;
            if (m_pending.monitor >= 0 && m_pending.monitor < (int)m_monitors.size())
            {
                deskW = m_monitors[m_pending.monitor].w;
                deskH = m_monitors[m_pending.monitor].h;
            }
            for (const auto& wh : DisplayConfig::WindowedSizes())
            {
                if (deskW > 0 && deskH > 0 && (wh.first >= deskW || wh.second >= deskH))
                    continue;
                ResolutionInfo info;
                info.w = wh.first;
                info.h = wh.second;
                info.bpp = 32;
                res.AddUnique(info);
            }
        }
        else if (GEngine)
        {
            GEngine->ListResolutions(res);
        }
        if (res.Size() == 0 && m_pending.windowMode == DisplayConfig::Fullscreen)
        {
            SDL_DisplayID display = DisplayIdForMonitorIndex(m_pending.monitor);
            int count = 0;
            if (SDL_DisplayMode** modes = SDL_GetFullscreenDisplayModes(display, &count))
            {
                for (int i = 0; i < count; ++i)
                {
                    ResolutionInfo info;
                    info.w = modes[i]->w;
                    info.h = modes[i]->h;
                    info.bpp = SDL_BITSPERPIXEL(modes[i]->format);
                    res.AddUnique(info);
                }
                SDL_free(modes);
            }
        }
        // Engine's ListResolutions can include duplicates at different bpp;
        // we only care about unique (w, h) pairs.
        m_resolutionLabels.emplace_back(LocalizeString("STR_DISP_MAIN_OPT_DISPLAY_NATIVE"));
        for (int i = 0; i < res.Size(); ++i)
        {
            std::pair<int, int> wh{res[i].w, res[i].h};
            bool dup = false;
            for (auto& existing : m_resolutions)
                if (existing == wh)
                {
                    dup = true;
                    break;
                }
            if (dup)
                continue;
            m_resolutions.push_back(wh);
            char buf[32];
            snprintf(buf, sizeof(buf), "%dx%d", wh.first, wh.second);
            m_resolutionLabels.emplace_back(buf);
        }
    }
    for (auto& s : m_resolutionLabels)
        m_resolutionCStrs.push_back(s.c_str());

    m_refreshRates.clear();
    m_refreshLabels.clear();
    m_refreshCStrs.clear();
    {
        FindArray<int> rates;
        if (GEngine)
            GEngine->ListRefreshRates(rates);
        if ((rates.Size() == 0 || (rates.Size() == 1 && rates[0] == 0)) &&
            m_pending.windowMode == DisplayConfig::Fullscreen)
        {
            int targetW = m_pending.resolutionWidth;
            int targetH = m_pending.resolutionHeight;
            if ((targetW <= 0 || targetH <= 0) && m_pending.monitor >= 0 && m_pending.monitor < (int)m_monitors.size())
            {
                targetW = m_monitors[m_pending.monitor].w;
                targetH = m_monitors[m_pending.monitor].h;
            }

            SDL_DisplayID display = DisplayIdForMonitorIndex(m_pending.monitor);
            int count = 0;
            if (SDL_DisplayMode** modes = SDL_GetFullscreenDisplayModes(display, &count))
            {
                for (int i = 0; i < count; ++i)
                {
                    if (targetW > 0 && targetH > 0 && (modes[i]->w != targetW || modes[i]->h != targetH))
                        continue;
                    rates.AddUnique(static_cast<int>(modes[i]->refresh_rate));
                }
                SDL_free(modes);
            }
        }
        m_refreshLabels.emplace_back(LocalizeString("STR_DISP_MAIN_OPT_SYSTEM_DEFAULT"));
        for (int i = 0; i < rates.Size(); ++i)
        {
            int hz = rates[i];
            bool dup = false;
            for (int existing : m_refreshRates)
                if (existing == hz)
                {
                    dup = true;
                    break;
                }
            if (dup)
                continue;
            m_refreshRates.push_back(hz);
            char buf[16];
            snprintf(buf, sizeof(buf), LocalizeString("STR_DISP_MAIN_OPT_DISPLAY_REFRESH_RATE_FORMAT"), hz);
            m_refreshLabels.emplace_back(buf);
        }
    }
    for (auto& s : m_refreshLabels)
        m_refreshCStrs.push_back(s.c_str());
}

void DisplayPage::RefreshLocalizedChoices()
{
    m_windowModeLabels[0] = LocalizeString("STR_DISP_MAIN_OPT_DISPLAY_FULLSCREEN");
    m_windowModeLabels[1] = LocalizeString("STR_DISP_MAIN_OPT_DISPLAY_BORDERLESS");
    m_windowModeLabels[2] = LocalizeString("STR_DISP_MAIN_OPT_DISPLAY_WINDOWED");
    for (size_t i = 0; i < m_windowModeLabels.size(); ++i)
        m_windowModeCStrs[i] = m_windowModeLabels[i].c_str();

    m_styleLabels[0] = LocalizeString("STR_DISP_MAIN_OPT_DISPLAY_STYLE_MODERN");
    m_styleLabels[1] = LocalizeString("STR_DISP_MAIN_OPT_DISPLAY_STYLE_CLASSIC");
    for (size_t i = 0; i < m_styleLabels.size(); ++i)
        m_styleCStrs[i] = m_styleLabels[i].c_str();

    m_clampLabels[0] = LocalizeString("STR_DISP_MAIN_OPT_VALUE_OFF");
    m_clampLabels[1] = LocalizeString("STR_DISP_MAIN_OPT_DISPLAY_CLAMP_21X9");
    m_clampLabels[2] = LocalizeString("STR_DISP_MAIN_OPT_DISPLAY_CLAMP_16X9");
    for (size_t i = 0; i < m_clampLabels.size(); ++i)
        m_clampCStrs[i] = m_clampLabels[i].c_str();
}

void DisplayPage::ApplyToEngine(const DisplayConfig& cfg)
{
    if (!GEngine)
        return;
    // Monitor first — the window may move, which can cascade other
    // state.  Window mode next.  Resolution + refresh last.
    if (cfg.monitor != GEngine->GetCurrentMonitor())
        GEngine->SwitchMonitor(cfg.monitor);
    if (cfg.windowMode == DisplayConfig::Fullscreen && cfg.refreshRate > 0)
        GEngine->SwitchRefreshRate(cfg.refreshRate);
    if (cfg.windowMode == DisplayConfig::Fullscreen)
    {
        int targetW = cfg.resolutionWidth;
        int targetH = cfg.resolutionHeight;
        if ((targetW <= 0 || targetH <= 0) && cfg.monitor >= 0 && cfg.monitor < (int)m_monitors.size())
        {
            targetW = m_monitors[cfg.monitor].w;
            targetH = m_monitors[cfg.monitor].h;
        }

        GEngine->SetWindowMode(WindowMode::Fullscreen);
        if (targetW > 0 && targetH > 0)
            GEngine->SwitchRes(targetW, targetH, /*bpp*/ 32);
    }
    else if (cfg.windowMode == DisplayConfig::Windowed)
    {
        // Mode first so the window is actually windowed (not fullscreen) before sizing;
        // then apply the chosen window size (0 = leave the engine's windowed size).
        GEngine->SetWindowMode(WindowMode::Windowed);
        if (cfg.resolutionWidth > 0 && cfg.resolutionHeight > 0)
            GEngine->SwitchRes(cfg.resolutionWidth, cfg.resolutionHeight, /*bpp*/ 32);
    }
    else
    {
        GEngine->SetWindowMode(static_cast<WindowMode>(cfg.windowMode));
    }
    Presentation::SetPolicy(cfg.displayStyle, cfg.ultrawideClamp);
    ApplyDisplayPolicy(GEngine->Width(), GEngine->Height());
}

void DisplayPage::SaveToCfg() const
{
    const std::string path = DisplayCfgPath();
    if (!m_applied.Save(path))
        LOG_WARN(Graphics, "DisplayPage::SaveToCfg: failed to write '{}'", path);
    else
        LOG_DEBUG(Graphics, "DisplayPage::SaveToCfg: wrote '{}'", path);
    if (GEngine)
        GEngine->SaveConfig();
}

} // namespace Poseidon
