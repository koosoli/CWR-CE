#pragma once

#include "TetrisGame.hpp"

#include <Poseidon/IO/ParamFile/ParamFile.hpp>
#include <Poseidon/UI/Controls/UIControls.hpp>

Poseidon::Foundation::UITime AdvanceTetrisNotebookUiTime(Poseidon::Foundation::UITime baseTime, float deltaT);

class TetrisNotebookDisplay : public ControlsContainer
{
public:
    explicit TetrisNotebookDisplay(TetrisGame* game);

    [[nodiscard]] bool IsLoaded() const;
    void Tick(float deltaT);
    void ToggleOpen();
    [[nodiscard]] bool IsOpen() const;

    Control* OnCreateCtrl(int type, int idc, const ParamEntry& cls) override;

private:
    TetrisGame* _game = nullptr;
    Ref<ControlObjectContainerAnim> _notebook;
    bool _loaded = false;
};
