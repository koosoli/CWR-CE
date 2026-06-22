#include <Poseidon/UI/Options/DisplayPage.hpp>
#include <Poseidon/UI/Options/OptionsScrollList.hpp>
#include <Poseidon/UI/Locale/Stringtable/Stringtable.hpp>
#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <string>

using namespace Poseidon;

namespace
{
class TestableDisplayPage : public DisplayPage
{
  public:
    OptionsScrollList::Provider& Provider() { return ProviderRef(); }
};

void LoadMainMenuStringtable()
{
    static bool loaded = false;
    if (loaded)
    {
        SetLanguage("English");
        return;
    }

    const std::filesystem::path csv =
        std::filesystem::path(TESTS_ROOT_DIR) / "fixtures" / "stringtable" / "STRINGTABLE_MAINMENU.utf8.csv";
    LoadStringtable("global", csv.string().c_str(), 0.0f, true);
    SetLanguage("English");
    loaded = true;
}
} // namespace

TEST_CASE("DisplayPage window mode UI mapping is stable", "[UI][DisplayPage]")
{
    CHECK(DisplayPage::WindowModeToUiIndex(DisplayConfig::Borderless) == 0);
    CHECK(DisplayPage::WindowModeToUiIndex(DisplayConfig::Fullscreen) == 1);
    CHECK(DisplayPage::WindowModeToUiIndex(DisplayConfig::Windowed) == 2);

    CHECK(DisplayPage::UiIndexToWindowMode(0) == DisplayConfig::Borderless);
    CHECK(DisplayPage::UiIndexToWindowMode(1) == DisplayConfig::Fullscreen);
    CHECK(DisplayPage::UiIndexToWindowMode(2) == DisplayConfig::Windowed);
    CHECK(DisplayPage::UiIndexToWindowMode(99) == DisplayConfig::Windowed);
}

TEST_CASE("DisplayPage display policy UI mapping is stable", "[UI][DisplayPage]")
{
    CHECK(DisplayPage::StyleToUiIndex(AspectRatio::Modern) == 0);
    CHECK(DisplayPage::StyleToUiIndex(AspectRatio::Legacy) == 1);
    CHECK(DisplayPage::UiIndexToStyle(0) == AspectRatio::Modern);
    CHECK(DisplayPage::UiIndexToStyle(1) == AspectRatio::Legacy);
    CHECK(DisplayPage::UiIndexToStyle(99) == AspectRatio::Modern);

    CHECK(DisplayPage::ClampToUiIndex(AspectRatio::ClampOff) == 0);
    CHECK(DisplayPage::ClampToUiIndex(AspectRatio::Clamp21x9) == 1);
    CHECK(DisplayPage::ClampToUiIndex(AspectRatio::Clamp16x9) == 2);
    CHECK(DisplayPage::UiIndexToClamp(0) == AspectRatio::ClampOff);
    CHECK(DisplayPage::UiIndexToClamp(1) == AspectRatio::Clamp21x9);
    CHECK(DisplayPage::UiIndexToClamp(2) == AspectRatio::Clamp16x9);
    CHECK(DisplayPage::UiIndexToClamp(99) == AspectRatio::ClampOff);
}

TEST_CASE("DisplayPage apply-enable helper tracks any pending display change", "[UI][DisplayPage]")
{
    DisplayConfig applied{};
    applied.monitor = 1;
    applied.windowMode = DisplayConfig::Fullscreen;
    applied.resolutionWidth = 1920;
    applied.resolutionHeight = 1080;
    applied.refreshRate = 60;
    applied.displayStyle = AspectRatio::Modern;
    applied.ultrawideClamp = AspectRatio::Clamp21x9;

    DisplayConfig pending = applied;
    CHECK_FALSE(DisplayPage::HasPendingChanges(pending, applied));

    pending.monitor = 0;
    CHECK(DisplayPage::HasPendingChanges(pending, applied));
    pending = applied;

    pending.windowMode = DisplayConfig::Windowed;
    CHECK(DisplayPage::HasPendingChanges(pending, applied));
    pending = applied;

    pending.resolutionWidth = 1280;
    CHECK(DisplayPage::HasPendingChanges(pending, applied));
    pending = applied;

    pending.refreshRate = 144;
    CHECK(DisplayPage::HasPendingChanges(pending, applied));
    pending = applied;

    pending.displayStyle = AspectRatio::Legacy;
    CHECK(DisplayPage::HasPendingChanges(pending, applied));
    pending = applied;

    pending.ultrawideClamp = AspectRatio::ClampOff;
    CHECK(DisplayPage::HasPendingChanges(pending, applied));
}

TEST_CASE("DisplayPage aspect descriptions keep the hint marquee active", "[UI][DisplayPage]")
{
    LoadMainMenuStringtable();

    TestableDisplayPage page;
    auto& provider = page.Provider();
    const std::string aspectDesc = provider.RowDescription(4);
    const std::string clampDesc = provider.RowDescription(5);

    CHECK(aspectDesc.size() > 42);
    CHECK(clampDesc.size() > 42);
    CHECK(OptionsScrollList::kHintInnerChars > 42);
    CHECK(OptionsScrollList::kHintInnerChars <= 50);
    CHECK(aspectDesc.size() > OptionsScrollList::kHintInnerChars);
    CHECK(clampDesc.size() > OptionsScrollList::kHintInnerChars);
}
