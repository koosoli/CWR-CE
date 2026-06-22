#include "TetrisNotebookUI.hpp"

#include <Poseidon/Core/Global.hpp>
#include <Poseidon/Graphics/Core/Engine.hpp>
#include <Poseidon/IO/ParamFileExt.hpp>
#include <Poseidon/UI/Controls/UIControls.hpp>

#include <filesystem>
#include <fstream>
#include <string>
#include <stdint.h>
#include <array>
#include <Poseidon/Foundation/Containers/Array.hpp>
#include <Poseidon/Foundation/Framework/Log.hpp>
#include <Poseidon/Foundation/Math/Math3D.hpp>
#include <Poseidon/Foundation/Math/Math3DP.hpp>
#include "TetrisGame.hpp"

using namespace Poseidon;
namespace
{
constexpr int kTetrisNotebookScreenIdc = 48001;

PackedColor ModAlphaLocal(PackedColor color, float alpha)
{
    Color c = color;
    c.SetA(c.A() * alpha);
    return PackedColor(c);
}

class TetrisNotebookScreenControl : public C3DStatic
{
  public:
    TetrisNotebookScreenControl(ControlsContainer* parent, int idc, const ParamEntry& cls, const TetrisGame* game)
        : C3DStatic(parent, idc, cls), _game(game)
    {
    }

    void OnDraw(float alpha) override
    {
        if (!_game)
        {
            return;
        }

        const PackedColor frameColor = ModAlphaLocal(_color, alpha);
        const PackedColor screenFill = ModAlphaLocal(_bgColor, alpha);
        const PackedColor blockFill(Color(0.05f, 0.30f, 0.12f, 0.65f * alpha));
        const Vector3 normal = _down.CrossProduct(_right).Normalized();

        DrawRect(0.0f, 0.0f, 1.0f, 1.0f, screenFill, normal, 0.0035f);
        DrawOutline(0.0f, 0.0f, 1.0f, 1.0f, frameColor, normal, 0.0038f);

        constexpr float kBoardX = 0.09f;
        constexpr float kBoardY = 0.12f;
        constexpr float kBoardW = 0.46f;
        constexpr float kBoardH = 0.76f;

        DrawOutline(kBoardX, kBoardY, kBoardW, kBoardH, frameColor, normal, 0.0042f);

        const Vector3 cellRight = (kBoardW / static_cast<float>(TetrisGame::BoardWidth)) * _right;
        const Vector3 cellDown = (kBoardH / static_cast<float>(TetrisGame::BoardHeight)) * _down;

        for (int32_t y = 0; y < TetrisGame::BoardHeight; ++y)
        {
            for (int32_t x = 0; x < TetrisGame::BoardWidth; ++x)
            {
                if (!_game->IsCellOccupied(x, y))
                {
                    continue;
                }

                const float cellX =
                    kBoardX + kBoardW * (static_cast<float>(x) / static_cast<float>(TetrisGame::BoardWidth));
                const float cellY =
                    kBoardY + kBoardH * (static_cast<float>(y) / static_cast<float>(TetrisGame::BoardHeight));
                DrawRect(cellX, cellY, kBoardW / static_cast<float>(TetrisGame::BoardWidth),
                         kBoardH / static_cast<float>(TetrisGame::BoardHeight), blockFill, normal, 0.0044f);
                DrawOutline(cellX, cellY, kBoardW / static_cast<float>(TetrisGame::BoardWidth),
                            kBoardH / static_cast<float>(TetrisGame::BoardHeight), frameColor, normal, 0.0048f);
            }
        }

        for (const TetrisCellPosition& block : _game->ActivePieceBlocks())
        {
            const Vector3 cellPos = _position + (kBoardX * _right) + (kBoardY * _down) +
                                    static_cast<float>(block.x) * cellRight + static_cast<float>(block.y) * cellDown;
            DrawOutline(cellPos, cellRight, cellDown, frameColor, normal, 0.0052f);
        }

        DrawLabel("POSEIDON TETRIS", 0.07f, 0.06f, 0.055f, frameColor, normal, 0.0055f);
        DrawLabel(("SCORE " + std::to_string(_game->Score())).c_str(), 0.62f, 0.30f, 0.050f, frameColor, normal,
                  0.0055f);
        DrawLabel(("LINES " + std::to_string(_game->LinesCleared())).c_str(), 0.62f, 0.40f, 0.050f, frameColor, normal,
                  0.0055f);
        DrawLabel("ARROWS/X", 0.62f, 0.50f, 0.045f, frameColor, normal, 0.0055f);
    }

  private:
    void DrawRect(float x, float y, float w, float h, PackedColor color, Vector3Par normal, float bias) const
    {
        DrawRect(_position + x * _right + y * _down, w * _right, h * _down, color, normal, bias);
    }

    void DrawRect(Vector3Par pos, Vector3Par right, Vector3Par down, PackedColor color, Vector3Par normal,
                  float bias) const
    {
        GEngine->Draw3D(pos - bias * normal, down, right, ClipAll, color, DisableSun, nullptr);
    }

    void DrawOutline(float x, float y, float w, float h, PackedColor color, Vector3Par normal, float bias) const
    {
        DrawOutline(_position + x * _right + y * _down, w * _right, h * _down, color, normal, bias);
    }

    void DrawOutline(Vector3Par pos, Vector3Par right, Vector3Par down, PackedColor color, Vector3Par normal,
                     float bias) const
    {
        const Vector3 surface = pos - bias * normal;
        GEngine->DrawLine3D(surface, surface + right, color, DisableSun);
        GEngine->DrawLine3D(surface + right, surface + right + down, color, DisableSun);
        GEngine->DrawLine3D(surface + right + down, surface + down, color, DisableSun);
        GEngine->DrawLine3D(surface + down, surface, color, DisableSun);
    }

    void DrawLabel(const char* text, float x, float y, float height, PackedColor color, Vector3Par normal,
                   float bias) const
    {
        if (!_font)
        {
            return;
        }

        const Vector3 pos = _position + x * _right + y * _down - bias * normal;
        const Vector3 up = -height * _down;
        const Vector3 right = 0.75f * up.Size() * _right.Normalized();
        GEngine->DrawText3D(pos, up, right, ClipAll, _font, color, DisableSun, text);
    }

    const TetrisGame* _game = nullptr;
};

std::filesystem::path MakeNotebookConfigPath()
{
    return std::filesystem::temp_directory_path() / "poseidon_tetris_notebook_ui.hpp";
}

const char* kNotebookConfigText = R"CFG(
class TetrisNotebook
{
    type=83;
    idc=105;
    model="notebook.p3d";
    animation="notebook.rtm";
    autoOpen=0;
    autoZoom=1;
    animSpeed=1;
    animPhase=0;
    position[]={0,-0.175000,0.250000};
    direction[]={0,0.500000,0.8660254};
    up[]={0,0.8660254,-0.500000};
    positionBack[]={0,-0.040000,0.600000};
    inBack=1;
    enableZoom=0;
    zoomDuration=1;
    scale=1;
    controls[]={};
};

class TetrisScreen
{
    type=20;
    idc=48001;
    style=80;
    selection="display";
    angle=0;
    text="";
    color[]={0.3,1,0.3,1};
    colorBackground[]={0,0,0,0.98};
    font="cwrMonoB64";
    size=0.001;
    x=0;
    y=0;
    w=1;
    h=1;
};
)CFG";
ParamFile* GetNotebookResourceDefs()
{
    static ParamFile defs;
    static bool loaded = false;
    static bool failed = false;

    if (failed)
    {
        return nullptr;
    }
    if (!loaded)
    {
        const std::filesystem::path path = MakeNotebookConfigPath();
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        if (!out)
        {
            failed = true;
            return nullptr;
        }
        out << kNotebookConfigText;
        out.close();

        if (defs.Parse(path.string().c_str()) != LSOK)
        {
            failed = true;
            return nullptr;
        }
        loaded = true;
    }

    return &defs;
}
} // namespace

Poseidon::Foundation::UITime AdvanceTetrisNotebookUiTime(Poseidon::Foundation::UITime baseTime, float deltaT)
{
    baseTime += deltaT;
    return baseTime;
}

TetrisNotebookDisplay::TetrisNotebookDisplay(TetrisGame* game) : ControlsContainer(nullptr), _game(game)
{
    ParamFile* resourceDefs = GetNotebookResourceDefs();
    if (!resourceDefs)
    {
        LOG_ERROR(Core, "TetrisNotebookDisplay: failed to load local notebook resource definitions");
        return;
    }

    const ParamEntry* notebookClass = resourceDefs->FindEntry("TetrisNotebook");
    const ParamEntry* screenClass = resourceDefs->FindEntry("TetrisScreen");
    if (!notebookClass || !screenClass)
    {
        LOG_ERROR(Core, "TetrisNotebookDisplay: missing notebook resource class");
        return;
    }

    LoadObject(*notebookClass);
    if (_objects.Size() == 0)
    {
        LOG_ERROR(Core, "TetrisNotebookDisplay: failed to create notebook object");
        return;
    }

    auto* notebook = dynamic_cast<ControlObjectContainerAnim*>(_objects[0].GetRef());
    if (!notebook)
    {
        LOG_ERROR(Core, "TetrisNotebookDisplay: created object is not animated notebook container");
        return;
    }
    _notebook = notebook;

    if (!notebook->LoadControl(*screenClass))
    {
        LOG_ERROR(Core, "TetrisNotebookDisplay: failed to mount Tetris screen control");
        return;
    }

    _loaded = true;
}

bool TetrisNotebookDisplay::IsLoaded() const
{
    return _loaded;
}

void TetrisNotebookDisplay::Tick(float deltaT)
{
    if (!_loaded)
    {
        return;
    }

    Glob.uiTime = AdvanceTetrisNotebookUiTime(Glob.uiTime, deltaT);
    OnSimulate(nullptr);
}

void TetrisNotebookDisplay::ToggleOpen()
{
    if (!_loaded || !_notebook || _notebook->IsAnimating())
    {
        return;
    }

    if (_notebook->IsOpen())
    {
        _notebook->Close();
    }
    else
    {
        _notebook->Open();
    }
}

bool TetrisNotebookDisplay::IsOpen() const
{
    return _notebook && _notebook->IsOpen();
}

Control* TetrisNotebookDisplay::OnCreateCtrl(int type, int idc, const ParamEntry& cls)
{
    if (idc == kTetrisNotebookScreenIdc)
    {
        return new TetrisNotebookScreenControl(this, idc, cls, _game);
    }

    return ControlsContainer::OnCreateCtrl(type, idc, cls);
}
