#pragma once

#include <utility>

namespace Poseidon
{
struct WindowMetrics
{
    int logicalWidth = 0;
    int logicalHeight = 0;
    int drawableWidth = 0;
    int drawableHeight = 0;
};

inline std::pair<int, int> GetWindowedModeSize(const WindowMetrics& metrics)
{
    if (metrics.logicalWidth > 0 && metrics.logicalHeight > 0)
        return {metrics.logicalWidth, metrics.logicalHeight};
    return {metrics.drawableWidth, metrics.drawableHeight};
}

inline bool WindowedResolutionAlreadyApplied(const WindowMetrics& metrics, int requestedWidth, int requestedHeight)
{
    return metrics.logicalWidth == requestedWidth && metrics.logicalHeight == requestedHeight;
}

} // namespace Poseidon
