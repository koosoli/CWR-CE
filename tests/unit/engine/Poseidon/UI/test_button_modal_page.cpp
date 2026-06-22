#include <Poseidon/Core/resincl.hpp>
#include <Poseidon/UI/Options/ButtonModalPage.hpp>
#include <Poseidon/UI/Options/OptionsShell.hpp>
#include <catch2/catch_test_macros.hpp>

#include <SDL3/SDL_keycode.h>
#include <memory>
#include <utility>
#include <vector>

using namespace Poseidon;
namespace
{
class TestableOptionsShell : public OptionsShell
{
  public:
    TestableOptionsShell() : OptionsShell(nullptr, true, false) {}
};

struct ModalResult
{
    int resolvedIdc = -1;
    int resolveCalls = 0;
};

class TestButtonModalPage : public ButtonModalPage
{
  public:
    explicit TestButtonModalPage(ModalResult& result) : ButtonModalPage({9201, 9202}, 9202), m_result(result) {}

    const char* TitleText() const override { return ""; }
    int DefaultFocusIdc() const override { return -1; }
    const char* ResourceClassName() const override { return ""; }

  protected:
    void OnResolved(OptionsShell&, int activatedIdc) override
    {
        m_result.resolvedIdc = activatedIdc;
        ++m_result.resolveCalls;
    }

  private:
    ModalResult& m_result;
};
} // namespace

TEST_CASE("ButtonModalPage resolves the clicked button exactly once", "[UI][ButtonModalPage]")
{
    TestableOptionsShell shell;
    ModalResult result;
    auto page = std::make_unique<TestButtonModalPage>(result);
    TestButtonModalPage* raw = page.get();

    shell.PushPage(std::move(page));

    CHECK(raw->OnButtonClicked(shell, 9201));
    CHECK(result.resolveCalls == 1);
    CHECK(result.resolvedIdc == 9201);
}

TEST_CASE("ButtonModalPage routes IDC_CANCEL to the configured cancel button", "[UI][ButtonModalPage]")
{
    TestableOptionsShell shell;
    ModalResult result;
    auto page = std::make_unique<TestButtonModalPage>(result);
    TestButtonModalPage* raw = page.get();

    shell.PushPage(std::move(page));

    CHECK(raw->OnButtonClicked(shell, IDC_CANCEL));
    CHECK(result.resolveCalls == 1);
    CHECK(result.resolvedIdc == 9202);
}

TEST_CASE("ButtonModalPage routes Esc to the configured cancel button", "[UI][ButtonModalPage]")
{
    TestableOptionsShell shell;
    ModalResult result;
    auto page = std::make_unique<TestButtonModalPage>(result);
    TestButtonModalPage* raw = page.get();

    shell.PushPage(std::move(page));

    CHECK(raw->OnKeyDown(shell, SDLK_ESCAPE));
    CHECK(result.resolveCalls == 1);
    CHECK(result.resolvedIdc == 9202);
}
