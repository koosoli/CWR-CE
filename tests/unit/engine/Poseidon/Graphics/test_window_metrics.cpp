#include <catch2/catch_test_macros.hpp>

#include <Poseidon/Graphics/Shared/WindowMetrics.hpp>
#include <utility>

using Poseidon::WindowMetrics;

TEST_CASE("Windowed mode sizing prefers logical size over HiDPI drawable size", "[Graphics][WindowMetrics]")
{
    WindowMetrics metrics;
    metrics.logicalWidth = 800;
    metrics.logicalHeight = 600;
    metrics.drawableWidth = 1333;
    metrics.drawableHeight = 1000;

    const auto [width, height] = GetWindowedModeSize(metrics);
    REQUIRE(width == 800);
    REQUIRE(height == 600);
}

TEST_CASE("Windowed mode sizing falls back to drawable size when logical size is unavailable",
          "[Graphics][WindowMetrics]")
{
    WindowMetrics metrics;
    metrics.drawableWidth = 1333;
    metrics.drawableHeight = 1000;

    const auto [width, height] = GetWindowedModeSize(metrics);
    REQUIRE(width == 1333);
    REQUIRE(height == 1000);
}

TEST_CASE("Windowed resolution comparison uses logical window size", "[Graphics][WindowMetrics]")
{
    WindowMetrics metrics;
    metrics.logicalWidth = 800;
    metrics.logicalHeight = 600;
    metrics.drawableWidth = 1333;
    metrics.drawableHeight = 1000;

    REQUIRE(WindowedResolutionAlreadyApplied(metrics, 800, 600));
    REQUIRE_FALSE(WindowedResolutionAlreadyApplied(metrics, 1333, 1000));
}
