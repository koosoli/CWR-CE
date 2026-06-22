#include <Poseidon/UI/Options/ConfirmRevertPage.hpp>
#include <Poseidon/UI/Options/OptionsShell.hpp>
#include <catch2/catch_test_macros.hpp>

#include <SDL3/SDL_keycode.h>
#include <memory>
#include <utility>

using namespace Poseidon;
namespace
{
class TestableOptionsShell : public OptionsShell
{
  public:
    TestableOptionsShell() : OptionsShell(nullptr, true, false) {}
};

struct ConfirmResult
{
    int keepCalls = 0;
    int revertCalls = 0;
};
} // namespace

TEST_CASE("ConfirmRevertPage Keep fires the keep callback", "[UI][ConfirmRevertPage]")
{
    TestableOptionsShell shell;
    ConfirmResult result;
    auto page =
        std::make_unique<ConfirmRevertPage>([&result]() { ++result.keepCalls; }, [&result]() { ++result.revertCalls; });
    ConfirmRevertPage* raw = page.get();

    shell.PushPage(std::move(page));
    CHECK(raw->OnButtonClicked(shell, 9201));

    CHECK(result.keepCalls == 1);
    CHECK(result.revertCalls == 0);
}

TEST_CASE("ConfirmRevertPage Esc fires the revert callback", "[UI][ConfirmRevertPage]")
{
    TestableOptionsShell shell;
    ConfirmResult result;
    auto page =
        std::make_unique<ConfirmRevertPage>([&result]() { ++result.keepCalls; }, [&result]() { ++result.revertCalls; });
    ConfirmRevertPage* raw = page.get();

    shell.PushPage(std::move(page));
    CHECK(raw->OnKeyDown(shell, SDLK_ESCAPE));

    CHECK(result.keepCalls == 0);
    CHECK(result.revertCalls == 1);
}

TEST_CASE("ConfirmRevertPage zero timeout auto-reverts on simulate", "[UI][ConfirmRevertPage]")
{
    TestableOptionsShell shell;
    ConfirmResult result;
    auto page = std::make_unique<ConfirmRevertPage>([&result]() { ++result.keepCalls; },
                                                    [&result]() { ++result.revertCalls; }, 0.0f);
    ConfirmRevertPage* raw = page.get();

    shell.PushPage(std::move(page));
    raw->OnSimulate(shell);

    CHECK(result.keepCalls == 0);
    CHECK(result.revertCalls == 1);
}
