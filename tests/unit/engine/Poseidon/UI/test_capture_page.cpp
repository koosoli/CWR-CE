#include <Poseidon/UI/Options/CapturePage.hpp>
#include <Poseidon/UI/Options/OptionsShell.hpp>
#include <catch2/catch_test_macros.hpp>

#include <SDL3/SDL_keycode.h>
#include <memory>
#include <string>
#include <utility>

using namespace Poseidon;
namespace
{
class TestableOptionsShell : public OptionsShell
{
  public:
    TestableOptionsShell() : OptionsShell(nullptr, true, false) {}
};

struct CaptureResult
{
    int savedCode = -1;
    int savedModifier = -1;
    bool replaceConflict = false;
    int saveCalls = 0;
};

class TestCapturePage : public CapturePage
{
  public:
    explicit TestCapturePage(CaptureResult& result)
        : CapturePage(
              Idcs{9301, 9303, 9302, 9380, 9381}, "Test Action", "Primary",
              [&result](int packedCode, int modifier, bool replaceConflict)
              {
                  result.savedCode = packedCode;
                  result.savedModifier = modifier;
                  result.replaceConflict = replaceConflict;
                  ++result.saveCalls;
              },
              [](int, int) { return UAN; })
    {
    }

    const char* ResourceClassName() const override { return "RscOptionsPagePressKey"; }
    bool Listening() const { return IsListening(); }

  protected:
    const char* PromptKey() const override { return "STR_DISP_OPT_CAP_PRESS_KEY"; }
    const char* PromptVerb() const override { return "key"; }

    Result InterpretKey(unsigned nChar, int& outPackedCode, int& outModifier) const override
    {
        switch (nChar)
        {
            case SDLK_A:
                outPackedCode = 101;
                outModifier = -1;
                return Result::Main;
            case SDLK_B:
                outPackedCode = 202;
                outModifier = -1;
                return Result::Main;
            default:
                return Result::Refused;
        }
    }
};
} // namespace

TEST_CASE("CapturePage hides save and retry until a capture succeeds", "[UI][CapturePage]")
{
    TestableOptionsShell shell;
    CaptureResult result;
    auto page = std::make_unique<TestCapturePage>(result);
    TestCapturePage* raw = page.get();

    shell.PushPage(std::move(page));

    CHECK(raw->Listening());
    CHECK(result.saveCalls == 0);

    raw->OnKeyDown(shell, SDLK_A);

    CHECK_FALSE(raw->Listening());
    CHECK(result.saveCalls == 0);
}

TEST_CASE("CapturePage retry returns to listening and later save commits the new capture", "[UI][CapturePage]")
{
    TestableOptionsShell shell;
    CaptureResult result;
    auto page = std::make_unique<TestCapturePage>(result);
    TestCapturePage* raw = page.get();

    shell.PushPage(std::move(page));

    raw->OnKeyDown(shell, SDLK_A);
    REQUIRE_FALSE(raw->Listening());

    raw->OnButtonClicked(shell, 9303);

    CHECK(raw->Listening());

    raw->OnKeyDown(shell, SDLK_B);
    REQUIRE_FALSE(raw->Listening());

    raw->OnButtonClicked(shell, 9301);

    CHECK(result.saveCalls == 1);
    CHECK(result.savedCode == 202);
    CHECK(result.savedModifier == -1);
    CHECK_FALSE(result.replaceConflict);
}

TEST_CASE("CapturePage cancel from captured state closes without saving", "[UI][CapturePage]")
{
    TestableOptionsShell shell;
    CaptureResult result;
    auto page = std::make_unique<TestCapturePage>(result);
    TestCapturePage* raw = page.get();

    shell.PushPage(std::move(page));

    raw->OnKeyDown(shell, SDLK_A);
    REQUIRE_FALSE(raw->Listening());

    raw->OnKeyDown(shell, SDLK_ESCAPE);

    CHECK(result.saveCalls == 0);
}
