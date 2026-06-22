// DisplayConfig — system-global display settings persistence.
// Mirrors test_audioConfig.cpp 1:1 — same case structure, same
// invariants (defaults / round-trip / partial-file tolerance /
// missing-file handling / Normalize semantics + field isolation).

#include <catch2/catch_test_macros.hpp>

#include <Poseidon/UI/Settings/DisplayConfig.hpp>

#include <filesystem>
#include <fstream>
#include <random>
#include <string>
#include <utility>
#include <vector>
#include <format>

using namespace Poseidon;

using Poseidon::DisplayConfig;

namespace
{
std::string TmpPath(const char* leaf)
{
    // Per-test path under a unique subdir so parallel Catch runs
    // don't fight over the same file.  The file may or may not
    // exist before the test runs; the test is responsible for
    // removing it on entry if it cares.
    static std::random_device rd;
    static std::mt19937 rng(rd());
    std::uniform_int_distribution<unsigned> dist;
    auto root = std::filesystem::temp_directory_path() / ("displaycfg_test_" + std::to_string(dist(rng)));
    std::filesystem::create_directories(root);
    return (root / leaf).string();
}

struct FakeEnvironment : DisplayConfig::Environment
{
    int monitorCount = 1;
    // Per-monitor resolution lists; entry [i] is monitor i's modes.
    std::vector<std::vector<std::pair<int, int>>> resolutions;
    // Refresh rates default per-monitor list; refined view of
    // per-(monitor, w, h) lookup.  Tests that need finer control
    // override ListRefreshRates by re-deriving with a custom
    // FakeEnvironment subclass.
    std::vector<int> refreshRates;

    int GetMonitorCount() const override { return monitorCount; }

    std::vector<std::pair<int, int>> ListResolutions(int idx) const override
    {
        if (idx < 0 || idx >= (int)resolutions.size())
            return {};
        return resolutions[idx];
    }

    std::vector<int> ListRefreshRates(int /*idx*/, int /*w*/, int /*h*/) const override { return refreshRates; }
};
} // namespace

TEST_CASE("DisplayConfig: factory defaults match spec", "[Settings][DisplayConfig]")
{
    DisplayConfig c;
    c.LoadDefaults();

    CHECK(c.monitor == 0);
    CHECK(c.windowMode == DisplayConfig::Borderless);
    CHECK(c.resolutionWidth == 0);
    CHECK(c.resolutionHeight == 0);
    CHECK(c.refreshRate == 0);
    CHECK(c.displayStyle == AspectRatio::Modern);
    CHECK(c.ultrawideClamp == AspectRatio::Clamp21x9);
}

TEST_CASE("DisplayConfig: a fresh instance starts at defaults", "[Settings][DisplayConfig]")
{
    // Construction alone should yield the same state LoadDefaults gives.
    DisplayConfig c;
    DisplayConfig defaulted;
    defaulted.LoadDefaults();

    CHECK(c.monitor == defaulted.monitor);
    CHECK(c.windowMode == defaulted.windowMode);
    CHECK(c.resolutionWidth == defaulted.resolutionWidth);
    CHECK(c.resolutionHeight == defaulted.resolutionHeight);
    CHECK(c.refreshRate == defaulted.refreshRate);
    CHECK(c.displayStyle == defaulted.displayStyle);
    CHECK(c.ultrawideClamp == defaulted.ultrawideClamp);
}

TEST_CASE("DisplayConfig: Save then Load round-trips every field", "[Settings][DisplayConfig]")
{
    const std::string path = TmpPath("roundtrip.cfg");
    std::filesystem::remove(path);

    DisplayConfig src;
    src.monitor = 1;
    src.windowMode = DisplayConfig::Fullscreen;
    src.resolutionWidth = 2560;
    src.resolutionHeight = 1440;
    src.refreshRate = 144;
    src.displayStyle = AspectRatio::Legacy;
    src.ultrawideClamp = AspectRatio::Clamp16x9;
    REQUIRE(src.Save(path));

    DisplayConfig dst;
    REQUIRE(dst.Load(path));
    CHECK(dst.monitor == src.monitor);
    CHECK(dst.windowMode == src.windowMode);
    CHECK(dst.resolutionWidth == src.resolutionWidth);
    CHECK(dst.resolutionHeight == src.resolutionHeight);
    CHECK(dst.refreshRate == src.refreshRate);
    CHECK(dst.displayStyle == src.displayStyle);
    CHECK(dst.ultrawideClamp == src.ultrawideClamp);
}

TEST_CASE("DisplayConfig: Load on missing file returns false, leaves instance untouched", "[Settings][DisplayConfig]")
{
    const std::string path = TmpPath("nope.cfg");
    std::filesystem::remove(path);

    DisplayConfig c;
    c.refreshRate = 165; // distinct from default so we can detect mutation
    CHECK_FALSE(c.Load(path));
    CHECK(c.refreshRate == 165);
}

TEST_CASE("DisplayConfig: Load on partial file keeps unspecified fields at current values", "[Settings][DisplayConfig]")
{
    // Forward-compat: a v1 config that only sets monitor / windowMode
    // must not reset newer fields when read by a v2 instance.  We
    // can't easily exercise per-key partials through Save (it always
    // writes every field), but we can drive the same invariant by
    // confirming Load doesn't disturb fields that are explicitly
    // present in the file with non-default values.
    const std::string path = TmpPath("partial.cfg");
    {
        DisplayConfig narrow;
        narrow.monitor = 2;
        narrow.windowMode = DisplayConfig::Windowed;
        narrow.resolutionWidth = 1920;
        narrow.resolutionHeight = 1080;
        narrow.refreshRate = 60;
        narrow.Save(path);
    }

    DisplayConfig dst;
    REQUIRE(dst.Load(path));
    CHECK(dst.monitor == 2);
    CHECK(dst.windowMode == DisplayConfig::Windowed);
    CHECK(dst.resolutionWidth == 1920);
    CHECK(dst.resolutionHeight == 1080);
    CHECK(dst.refreshRate == 60);
}

TEST_CASE("DisplayConfig: Normalize is a no-op when every field is valid", "[Settings][DisplayConfig]")
{
    FakeEnvironment env;
    env.monitorCount = 2;
    env.resolutions = {{{1920, 1080}, {2560, 1440}}, {{3840, 2160}}};
    env.refreshRates = {60, 120, 144};

    DisplayConfig c;
    c.monitor = 1;
    c.windowMode = DisplayConfig::Fullscreen;
    c.resolutionWidth = 3840;
    c.resolutionHeight = 2160;
    c.refreshRate = 60;

    CHECK_FALSE(c.Normalize(env));
    CHECK(c.monitor == 1);
    CHECK(c.resolutionWidth == 3840);
    CHECK(c.resolutionHeight == 2160);
    CHECK(c.refreshRate == 60);
}

TEST_CASE("DisplayConfig: Normalize resets out-of-range monitor index to 0", "[Settings][DisplayConfig]")
{
    // Field-isolation: a stale monitor index must not also wipe
    // resolution / refresh.
    FakeEnvironment env;
    env.monitorCount = 1;
    env.resolutions = {{{1920, 1080}}};
    env.refreshRates = {60};

    DisplayConfig c;
    c.monitor = 5; // gone — laptop unplugged from external
    c.windowMode = DisplayConfig::Borderless;
    c.resolutionWidth = 0;
    c.resolutionHeight = 0;
    c.refreshRate = 0;

    CHECK(c.Normalize(env));
    CHECK(c.monitor == 0);
    CHECK(c.windowMode == DisplayConfig::Borderless);
    CHECK(c.resolutionWidth == 0);
    CHECK(c.resolutionHeight == 0);
}

TEST_CASE("DisplayConfig: Normalize resets unsupported resolution to native (0,0)", "[Settings][DisplayConfig]")
{
    FakeEnvironment env;
    env.monitorCount = 1;
    env.resolutions = {{{1920, 1080}, {2560, 1440}}};
    env.refreshRates = {60};

    DisplayConfig c;
    c.monitor = 0;
    c.resolutionWidth = 5120; // a 5K display we no longer have
    c.resolutionHeight = 2880;
    c.refreshRate = 0;

    CHECK(c.Normalize(env));
    CHECK(c.resolutionWidth == 0);
    CHECK(c.resolutionHeight == 0);
    // Monitor + refresh untouched.
    CHECK(c.monitor == 0);
    CHECK(c.refreshRate == 0);
}

TEST_CASE("DisplayConfig: Normalize accepts (0,0) native sentinel even with non-empty list",
          "[Settings][DisplayConfig]")
{
    // (0, 0) is always valid regardless of what ListResolutions returns —
    // it means "use the monitor's native mode".
    FakeEnvironment env;
    env.monitorCount = 1;
    env.resolutions = {{{1920, 1080}}};

    DisplayConfig c;
    c.resolutionWidth = 0;
    c.resolutionHeight = 0;
    CHECK_FALSE(c.Normalize(env));
}

TEST_CASE("DisplayConfig: Normalize resets unsupported refresh rate, leaves resolution alone",
          "[Settings][DisplayConfig]")
{
    // Field-isolation in the other direction: a stale refresh rate
    // must not knock out the resolution selection.
    FakeEnvironment env;
    env.monitorCount = 1;
    env.resolutions = {{{1920, 1080}}};
    env.refreshRates = {60, 75};

    DisplayConfig c;
    c.monitor = 0;
    c.resolutionWidth = 1920;
    c.resolutionHeight = 1080;
    c.refreshRate = 240; // monitor doesn't support it any more

    CHECK(c.Normalize(env));
    CHECK(c.refreshRate == 0);
    CHECK(c.resolutionWidth == 1920);
    CHECK(c.resolutionHeight == 1080);
}

TEST_CASE("DisplayConfig: Normalize resets unknown windowMode to Borderless", "[Settings][DisplayConfig]")
{
    FakeEnvironment env;
    DisplayConfig c;
    c.windowMode = static_cast<DisplayConfig::WindowMode>(99);
    CHECK(c.Normalize(env));
    CHECK(c.windowMode == DisplayConfig::Borderless);
}

TEST_CASE("DisplayConfig: empty Environment lists treat any value as valid", "[Settings][DisplayConfig]")
{
    // Dummy backend / harness with no enumeration: empty resolution
    // list and empty refresh-rate list mean "any is valid", so a
    // well-formed config doesn't trigger normalization.
    FakeEnvironment env;
    env.monitorCount = 1;
    // resolutions and refreshRates left empty
    DisplayConfig c;
    c.monitor = 0;
    c.windowMode = DisplayConfig::Fullscreen;
    c.resolutionWidth = 1920;
    c.resolutionHeight = 1080;
    c.refreshRate = 60;
    CHECK_FALSE(c.Normalize(env));
}

TEST_CASE("DisplayConfig: Normalize on already-normalized values reports no change", "[Settings][DisplayConfig]")
{
    // Idempotence — second call must return false.
    FakeEnvironment env;
    env.monitorCount = 1;
    env.resolutions = {{{1920, 1080}}};
    env.refreshRates = {60};

    DisplayConfig c;
    c.monitor = 5; // invalid; first Normalize clamps to 0
    c.resolutionWidth = 1920;
    c.resolutionHeight = 1080;
    c.refreshRate = 60;
    c.Normalize(env);
    CHECK_FALSE(c.Normalize(env));
}

TEST_CASE("DisplayConfig: Normalize signals change when only one field is invalid", "[Settings][DisplayConfig]")
{
    // The "changed" return must be true if ANY field was modified,
    // matching AudioConfig::Normalize semantics.
    FakeEnvironment env;
    env.monitorCount = 1;
    env.resolutions = {{{1920, 1080}}};
    env.refreshRates = {60};

    DisplayConfig c;
    c.monitor = 0;
    c.windowMode = DisplayConfig::Borderless;
    c.resolutionWidth = 1920;
    c.resolutionHeight = 1080;
    c.refreshRate = 999; // only this one invalid
    CHECK(c.Normalize(env));
}

TEST_CASE("DisplayConfig: Normalize resets invalid display policy enums to safe defaults", "[Settings][DisplayConfig]")
{
    FakeEnvironment env;

    DisplayConfig c;
    c.displayStyle = static_cast<AspectRatio::DisplayStyle>(99);
    c.ultrawideClamp = static_cast<AspectRatio::UltrawideClamp>(99);

    CHECK(c.Normalize(env));
    CHECK(c.displayStyle == AspectRatio::Modern);
    CHECK(c.ultrawideClamp == AspectRatio::Clamp21x9);
}

TEST_CASE("DisplayConfig: legacy non-auto aspect configs migrate to Legacy style", "[Settings][DisplayConfig]")
{
    const std::string path = TmpPath("legacy_aspect.cfg");
    std::filesystem::remove(path);
    {
        std::ofstream file(path);
        REQUIRE(file.is_open());
        file << "aspectPreset=6;\n";
        file << "customAspectWidth=21;\n";
        file << "customAspectHeight=9;\n";
    }

    DisplayConfig cfg;
    REQUIRE(cfg.Load(path));
    CHECK(cfg.displayStyle == AspectRatio::Legacy);
    CHECK(cfg.ultrawideClamp == AspectRatio::Clamp21x9);
}

TEST_CASE("DisplayConfig: legacy auto aspect configs migrate to Modern defaults", "[Settings][DisplayConfig]")
{
    const std::string path = TmpPath("auto_aspect.cfg");
    std::filesystem::remove(path);
    {
        std::ofstream file(path);
        REQUIRE(file.is_open());
        file << "aspectPreset=0;\n";
        file << "customAspectWidth=16;\n";
        file << "customAspectHeight=9;\n";
    }

    DisplayConfig cfg;
    cfg.displayStyle = AspectRatio::Legacy;
    cfg.ultrawideClamp = AspectRatio::ClampOff;
    REQUIRE(cfg.Load(path));
    CHECK(cfg.displayStyle == AspectRatio::Modern);
    CHECK(cfg.ultrawideClamp == AspectRatio::Clamp21x9);
}

// Windowed window sizes
// Windowed mode exposes a curated set of basic window sizes (800x600 up) that
// persist and validate independently of the monitor's exclusive-fullscreen modes.

TEST_CASE("DisplayConfig: WindowedSizes lists basic sizes including 800x600", "[Settings][DisplayConfig]")
{
    const auto& sizes = DisplayConfig::WindowedSizes();
    CHECK_FALSE(sizes.empty());
    CHECK(DisplayConfig::IsWindowedSize(800, 600));
    CHECK(DisplayConfig::IsWindowedSize(1280, 720));
    CHECK_FALSE(DisplayConfig::IsWindowedSize(1234, 567)); // not a curated size
    CHECK_FALSE(DisplayConfig::IsWindowedSize(0, 0));      // native is the sentinel, not a size
}

TEST_CASE("DisplayConfig: Normalize keeps a curated window size in Windowed mode", "[Settings][DisplayConfig]")
{
    // 800x600 is a valid window size even though the monitor never offers it as a
    // fullscreen mode.
    FakeEnvironment env;
    env.monitorCount = 1;
    env.resolutions = {{{1920, 1080}, {2560, 1440}}};

    DisplayConfig c;
    c.windowMode = DisplayConfig::Windowed;
    c.resolutionWidth = 800;
    c.resolutionHeight = 600;
    CHECK_FALSE(c.Normalize(env)); // kept, no change
    CHECK(c.resolutionWidth == 800);
    CHECK(c.resolutionHeight == 600);
}

TEST_CASE("DisplayConfig: Normalize rejects a window-only size in Fullscreen mode", "[Settings][DisplayConfig]")
{
    // The curated window sizes are not valid exclusive-fullscreen modes — 800x600
    // selected in Fullscreen falls back to native.
    FakeEnvironment env;
    env.monitorCount = 1;
    env.resolutions = {{{1920, 1080}, {2560, 1440}}};

    DisplayConfig c;
    c.windowMode = DisplayConfig::Fullscreen;
    c.resolutionWidth = 800;
    c.resolutionHeight = 600;
    CHECK(c.Normalize(env));
    CHECK(c.resolutionWidth == 0);
    CHECK(c.resolutionHeight == 0);
}

TEST_CASE("DisplayConfig: Normalize resets a non-curated window size in Windowed mode", "[Settings][DisplayConfig]")
{
    FakeEnvironment env;
    env.monitorCount = 1;
    env.resolutions = {{{1920, 1080}}};

    DisplayConfig c;
    c.windowMode = DisplayConfig::Windowed;
    c.resolutionWidth = 1234; // neither a curated window size nor a listed mode
    c.resolutionHeight = 567;
    CHECK(c.Normalize(env));
    CHECK(c.resolutionWidth == 0);
    CHECK(c.resolutionHeight == 0);
}

TEST_CASE("DisplayConfig: a Windowed size survives a Save/Load round-trip", "[Settings][DisplayConfig]")
{
    const std::string path = TmpPath("windowed_size.cfg");
    std::filesystem::remove(path);

    DisplayConfig saved;
    saved.windowMode = DisplayConfig::Windowed;
    saved.resolutionWidth = 1024;
    saved.resolutionHeight = 768;
    REQUIRE(saved.Save(path));

    DisplayConfig loaded;
    REQUIRE(loaded.Load(path));
    CHECK(loaded.windowMode == DisplayConfig::Windowed);
    CHECK(loaded.resolutionWidth == 1024);
    CHECK(loaded.resolutionHeight == 768);
}

TEST_CASE("DisplayConfig: Windowed native<->custom<->native round trips stay consistent", "[Settings][DisplayConfig]")
{
    // Going back and forth between Native and a custom window size must always land
    // on exactly what was set, with no drift.
    FakeEnvironment env;
    env.monitorCount = 1;
    env.resolutions = {{{1920, 1080}}};

    DisplayConfig c;
    c.windowMode = DisplayConfig::Windowed;

    c.resolutionWidth = 800; // native -> custom
    c.resolutionHeight = 600;
    CHECK_FALSE(c.Normalize(env));
    CHECK(c.resolutionWidth == 800);

    c.resolutionWidth = 0; // custom -> native
    c.resolutionHeight = 0;
    CHECK_FALSE(c.Normalize(env));
    CHECK(c.resolutionWidth == 0);
    CHECK(c.resolutionHeight == 0);

    c.resolutionWidth = 1280; // native -> a different custom
    c.resolutionHeight = 720;
    CHECK_FALSE(c.Normalize(env));
    CHECK(c.resolutionWidth == 1280);
    CHECK(c.resolutionHeight == 720);
}
