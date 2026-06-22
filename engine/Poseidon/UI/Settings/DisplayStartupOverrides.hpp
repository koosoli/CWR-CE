#pragma once

#include <Poseidon/UI/Settings/DisplayConfig.hpp>

#include <optional>


namespace Poseidon
{
struct DisplayStartupOverrides
{
    std::optional<DisplayConfig::WindowMode> windowMode;
    std::optional<int> resolutionWidth;
    std::optional<int> resolutionHeight;
};

struct DisplayStartupOverrideRequest
{
    bool windowFlagExplicit = false;
    std::optional<DisplayConfig::WindowMode> windowMode;
    std::optional<int> resolutionWidth;
    std::optional<int> resolutionHeight;
    int defaultWindowWidth = 800;
    int defaultWindowHeight = 600;
};

inline DisplayStartupOverrides BuildDisplayStartupOverrides(const DisplayStartupOverrideRequest& request)
{
    DisplayStartupOverrides overrides;
    overrides.windowMode = request.windowMode;
    overrides.resolutionWidth = request.resolutionWidth;
    overrides.resolutionHeight = request.resolutionHeight;

    if (request.windowFlagExplicit)
    {
        if (!overrides.resolutionWidth)
            overrides.resolutionWidth = request.defaultWindowWidth;
        if (!overrides.resolutionHeight)
            overrides.resolutionHeight = request.defaultWindowHeight;
    }

    return overrides;
}

inline bool ApplyDisplayStartupOverrides(DisplayConfig& cfg, const DisplayStartupOverrides& overrides)
{
    bool changed = false;

    if (overrides.windowMode && cfg.windowMode != *overrides.windowMode)
    {
        cfg.windowMode = *overrides.windowMode;
        changed = true;
    }

    if (overrides.resolutionWidth && cfg.resolutionWidth != *overrides.resolutionWidth)
    {
        cfg.resolutionWidth = *overrides.resolutionWidth;
        changed = true;
    }

    if (overrides.resolutionHeight && cfg.resolutionHeight != *overrides.resolutionHeight)
    {
        cfg.resolutionHeight = *overrides.resolutionHeight;
        changed = true;
    }

    return changed;
}

} // namespace Poseidon
