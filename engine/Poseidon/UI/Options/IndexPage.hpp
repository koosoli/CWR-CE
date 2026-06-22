#pragma once

// Options Index — landing screen of the Options shell.
//
// Rows: Audio, Display, Graphics, Game, Controls, Difficulty, Credits, Close.
// Each row pushes the corresponding page (or, for Credits, plays the
// cutscene, and for Close, exits the shell).
//
// Resource: RscOptionsPageIndex.  IDC 1101 (Audio), 1102 (Display),
// 1103 (Graphics), 1104 (Game), 1107 (Controls), 1108 (Difficulty),
// 1105 (Credits), 1106 (Close).  Controls and Difficulty both live on
// the top-level index; optionsUI.hpp tightens the row pitch so all 8
// rows fit in the same notebook UV span.

#include <Poseidon/UI/Options/FlatMenuPage.hpp>


namespace Poseidon
{
class IndexPage : public FlatMenuPage
{
  public:
    IndexPage() : FlatMenuPage(kCycle, kCycleSize) {}

    const char* TitleText() const override;
    int DefaultFocusIdc() const override { return 1101; } // Audio
    const char* ResourceClassName() const override { return "RscOptionsPageIndex"; }

    void Mount(OptionsShell& shell) override;
    void OnReshown(OptionsShell& shell) override;

  protected:
    bool OnNav(OptionsShell& shell, int idc) override;

  private:
    // Keyboard arrow-key cycle order — matches the visible row layout in
    // RscOptionsPageIndex's controls[] array.  Entries here drive
    // FlatMenuPage::WrapFocus; rows missing from the cycle are visible
    // but unreachable by keyboard, which is why this needs to stay
    // in sync with the resource bundle when rows are added.
    static constexpr int kCycle[] = {1101, 1102, 1103, 1104, 1107, 1108, 1105, 1106};
    static constexpr int kCycleSize = sizeof(kCycle) / sizeof(kCycle[0]);
};

} // namespace Poseidon
