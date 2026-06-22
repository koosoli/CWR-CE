#pragma once

// Generic Yes/No confirm modal — pushed by destructive actions like
// "Reset all to defaults".  Two-button modal on ButtonModalPage —
// `onYes` fires when the user confirms; cancellation (Esc / No) does
// nothing.  Default focus on No so a stuck Enter doesn't trigger
// the destructive path.

#include <Poseidon/UI/Options/ButtonModalPage.hpp>

#include <functional>
#include <string>


namespace Poseidon
{
class ConfirmPage : public ButtonModalPage
{
  public:
    using ConfirmCallback = std::function<void()>;

    ConfirmPage(std::string title,
                std::string body,
                ConfirmCallback onYes,
                std::string yesLabel = "Yes",
                std::string noLabel = "No")
        : ButtonModalPage({kIdcYes, kIdcNo}, kIdcNo)
        , m_title(std::move(title))
        , m_body(std::move(body))
        , m_onYes(std::move(onYes))
        , m_yesLabel(std::move(yesLabel))
        , m_noLabel(std::move(noLabel))
    {
    }

    int DefaultFocusIdc() const override { return kIdcNo; }
    const char* ResourceClassName() const override { return "RscOptionsPageConfirm"; }

    void Mount(OptionsShell& shell) override;

  protected:
    void OnResolved(OptionsShell& shell, int activatedIdc) override;

  private:
    static constexpr int kIdcYes = 9101;
    static constexpr int kIdcNo = 9102;
    static constexpr int kIdcTitle = 9180;
    static constexpr int kIdcBody = 9181;

    std::string m_title;
    std::string m_body;
    ConfirmCallback m_onYes;
    std::string m_yesLabel;
    std::string m_noLabel;
};

} // namespace Poseidon
