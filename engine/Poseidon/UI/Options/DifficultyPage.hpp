#pragma once

#include <Poseidon/UI/Options/ScrollListPage.hpp>

#include <Poseidon/Core/Profile/DifficultyTypes.hpp>

#include <array>
#include <string>


namespace Poseidon
{
class DifficultyPage : public ScrollListPage
{
  public:
    const char* TitleText() const override;

    void Mount(OptionsShell& shell) override;
    void OnReshown(OptionsShell& shell) override;
    void Unmount(OptionsShell& shell) override;

  protected:
    OptionsScrollList::Provider& ProviderRef() override { return m_provider; }

  private:
    static const char* CloseLabel();
    static const char* CloseDescription();

    class DifficultyProvider : public OptionsScrollList::Provider
    {
      public:
        enum : int
        {
            kRowMode = 0,
            kRowFirstSetting = 1,
            kRowReset = kRowFirstSetting + DTN,
        };

        void SetPage(DifficultyPage* page) { m_page = page; }

        int RowCount() const override { return kRowReset + 1; }
        const char* RowLabel(int row) const override;
        const char* RowDescription(int row) const override;
        OptionsScrollList::RowDef RowFor(int row) const override;
        int RowValue(int row) const override;
        void SetRowValue(int row, int value) override;
        OptionsScrollList::Kind RowKind(int row) const override;
        void OnRowAction(int row, Display& host) override;

      private:
        int SettingIndex(int row) const { return row - kRowFirstSetting; }
        bool* ActiveDifficultyArray() const;
        void Persist() const;

        DifficultyPage* m_page = nullptr;
    };

    DifficultyProvider m_difficulty;
    OptionsScrollList::WithCloseRow m_provider{m_difficulty, CloseLabel(), CloseDescription()};
    std::array<bool, DTN> m_cadetDifficulty{};
    std::array<bool, DTN> m_veteranDifficulty{};
    std::array<std::string, 2> m_modeLabels;
    std::array<const char*, 2> m_modeOptions{};
    std::array<std::string, 2> m_toggleLabels;
    std::array<const char*, 2> m_toggleOptions{};
    int m_selectedMode = 0;
};

} // namespace Poseidon
