#include <Poseidon/Core/Global.hpp>
#include <Poseidon/Core/resincl.hpp>
#include <Poseidon/Network/Network.hpp>
#include <Poseidon/UI/Map/UIMap.hpp>
#include <Poseidon/UI/Options/IndexPage.hpp>
#include <Poseidon/UI/Options/OptionsShell.hpp>
#include <Poseidon/UI/Options/AudioPage.hpp>
#include <Poseidon/UI/Options/ControlsPage.hpp>
#include <Poseidon/UI/Options/DisplayPage.hpp>
#include <Poseidon/UI/Options/DifficultyPage.hpp>
#include <Poseidon/UI/Options/GraphicsPage.hpp>
#include <Poseidon/UI/Options/GamePage.hpp>

#include <Poseidon/UI/OptionsUICommon.hpp>
#include <Poseidon/UI/DisplayUI.hpp>

#include <Poseidon/UI/Locale/Stringtable/Stringtable.hpp>

#include <memory>
#include <Poseidon/Foundation/Strings/RString.hpp>

namespace Poseidon
{

namespace
{
void RefreshIndexLayout(OptionsShell& shell)
{
    auto setNavText = [&](int idc, const char* text)
    {
        auto* nb = shell.GetNotebook();
        auto* ctrl = nb ? nb->GetCtrl(idc) : nullptr;
        auto* active = dynamic_cast<C3DActiveText*>(ctrl);
        if (active)
            active->SetText(text ? text : "");
    };

    const bool showCredits = shell.ShouldShowCredits();
    if (auto* credits = shell.GetCtrl(1105))
        credits->ShowCtrl(true);
    if (auto* close = shell.GetCtrl(1106))
        close->ShowCtrl(showCredits);

    setNavText(1101, LocalizeString("STR_DISP_MAIN_OPT_AUDIO"));
    setNavText(1102, LocalizeString("STR_DISP_MAIN_OPT_DISPLAY"));
    setNavText(1103, LocalizeString("STR_DISP_MAIN_OPT_GRAPHICS"));
    setNavText(1104, LocalizeString("STR_DISP_MAIN_OPT_GAME"));
    setNavText(1107, LocalizeString("STR_DISP_OPT_CONTROLS"));
    setNavText(1108, LocalizeString("STR_DISP_OPTIONS_DIFFICULTY"));
    setNavText(1105, LocalizeString(showCredits ? "STR_DISP_MAIN_CREDITS" : "STR_DISP_CLOSE"));
    setNavText(1106, LocalizeString("STR_DISP_CLOSE"));
}
} // namespace

const char* IndexPage::TitleText() const
{
    return LocalizeString("STR_DISP_OPTIONS_TITLE");
}

void IndexPage::Mount(OptionsShell& shell)
{
    OptionsPage::Mount(shell);
    RefreshIndexLayout(shell);
}

void IndexPage::OnReshown(OptionsShell& shell)
{
    RefreshIndexLayout(shell);
    // Chain to the base so focus memory restores the last-clicked nav
    // row when popping back from a child page (Audio/Display/.../Game).
    FlatMenuPage::OnReshown(shell);
}

bool IndexPage::OnNav(OptionsShell& shell, int idc)
{
    switch (idc)
    {
        case 1101: // Audio
            shell.PushPage(std::make_unique<AudioPage>());
            return true;
        case 1102: // Display
            shell.PushPage(std::make_unique<DisplayPage>());
            return true;
        case 1103: // Graphics
            shell.PushPage(std::make_unique<GraphicsPage>());
            return true;
        case 1104: // Game
            shell.PushPage(std::make_unique<GamePage>());
            return true;
        case 1107: // Controls
            shell.PushPage(std::make_unique<ControlsPage>());
            return true;
        case 1108: // Difficulty
            shell.PushPage(std::make_unique<DifficultyPage>());
            return true;
        case 1105: // Credits / Close
            if (!shell.ShouldShowCredits())
            {
                shell.PopPage();
                return true;
            }
            PlayCreditsCutscene(&shell);
            return true;
        case 1106: // Close
            shell.PopPage();
            return true;
    }
    return false;
}

} // namespace Poseidon
