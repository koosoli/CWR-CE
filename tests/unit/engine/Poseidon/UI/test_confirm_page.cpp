#include <Poseidon/UI/Options/ConfirmPage.hpp>
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
    int yesCalls = 0;
};
} // namespace

TEST_CASE("ConfirmPage Yes fires the confirm callback", "[UI][ConfirmPage]")
{
    TestableOptionsShell shell;
    ConfirmResult result;
    auto page = std::make_unique<ConfirmPage>("Reset all to defaults?", "This cannot be undone.",
                                              [&result]() { ++result.yesCalls; });
    ConfirmPage* raw = page.get();

    shell.PushPage(std::move(page));
    CHECK(raw->OnButtonClicked(shell, 9101));

    CHECK(result.yesCalls == 1);
}

TEST_CASE("ConfirmPage No does not fire the confirm callback", "[UI][ConfirmPage]")
{
    TestableOptionsShell shell;
    ConfirmResult result;
    auto page = std::make_unique<ConfirmPage>("Reset all to defaults?", "This cannot be undone.",
                                              [&result]() { ++result.yesCalls; });
    ConfirmPage* raw = page.get();

    shell.PushPage(std::move(page));
    CHECK(raw->OnButtonClicked(shell, 9102));

    CHECK(result.yesCalls == 0);
}

TEST_CASE("ConfirmPage Esc does not fire the confirm callback", "[UI][ConfirmPage]")
{
    TestableOptionsShell shell;
    ConfirmResult result;
    auto page = std::make_unique<ConfirmPage>("Reset all to defaults?", "This cannot be undone.",
                                              [&result]() { ++result.yesCalls; });
    ConfirmPage* raw = page.get();

    shell.PushPage(std::move(page));
    CHECK(raw->OnKeyDown(shell, SDLK_ESCAPE));

    CHECK(result.yesCalls == 0);
}
