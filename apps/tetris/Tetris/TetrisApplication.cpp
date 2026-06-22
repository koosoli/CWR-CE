#include "TetrisApplication.hpp" // pulls windows.h via GameApplication.hpp
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_timer.h>
#include <Poseidon/Foundation/Framework/Log.hpp>
#include <Poseidon/Foundation/Strings/RString.hpp>
#undef DrawText // Windows GDI macro hides Engine::DrawText

#include "TetrisGame.hpp"
#include "TetrisNotebookUI.hpp"

#include <Poseidon/Audio/AudioFactory.hpp>
#include <Poseidon/Audio/IAudioSystem.hpp>
#include <Poseidon/Audio/Dummy/SoundSystemDummy.hpp>
#include <Poseidon/Foundation/Common/GamePaths.hpp>
#include <Poseidon/Graphics/Rendering/Draw/FontSystem.hpp>
#include <Poseidon/Graphics/Rendering/Draw/Font.hpp>
#include <Poseidon/Graphics/Core/Engine.hpp>
#include <Poseidon/World/Scene/Scene.hpp>
#include <Poseidon/Graphics/Rendering/Lighting/Lights.hpp>
#include <Poseidon/Foundation/Platform/AppConfig.hpp>
#include <Poseidon/Foundation/Platform/InitBridge.hpp>
#include <Poseidon/Core/Global.hpp>
#include <Poseidon/Core/Application.hpp>
#include <Poseidon/Core/ProgressSystem.hpp>
#include <Poseidon/IO/ParamFileExt.hpp>
#include <Poseidon/Foundation/Strings/Mbcs.hpp> // For FontID complete type

#include <SDL3/SDL.h>

using namespace Poseidon;

extern void CleanupSoundSystem();

namespace
{
void HandleTetrisInput(TetrisGame& game, bool& prevLeft, bool& prevRight, bool& prevRotate, bool& prevDown)
{
    const bool* keys = SDL_GetKeyboardState(nullptr);
    if (!keys)
    {
        return;
    }

    const bool left = keys[SDL_SCANCODE_LEFT];
    const bool right = keys[SDL_SCANCODE_RIGHT];
    const bool rotate = keys[SDL_SCANCODE_UP] || keys[SDL_SCANCODE_X];
    const bool down = keys[SDL_SCANCODE_DOWN];

    if (left && !prevLeft)
    {
        game.MoveLeft();
    }
    if (right && !prevRight)
    {
        game.MoveRight();
    }
    if (rotate && !prevRotate)
    {
        game.RotateClockwise();
    }
    if (down && !prevDown)
    {
        game.SoftDrop();
    }

    prevLeft = left;
    prevRight = right;
    prevRotate = rotate;
    prevDown = down;
}
} // namespace

void TetrisApplication::RegisterGameModules()
{
    // Empty — Tetris doesn't host missions / campaigns / multiplayer.
}

bool TetrisApplication::ParseCommandLine(const char* commandLine)
{
    // Seed GamePaths with a Tetris-only codename before the base call
    // resolves it.  GamePaths::Initialize is idempotent — the first
    // caller wins — so this keeps Tetris's display.cfg / graphics.cfg /
    // audio.cfg under %APPDATA%/PoseidonTetris/ instead of sharing the
    // main Cold War Assault install's %APPDATA%/CWR/.
    GamePaths::Instance().Initialize("PoseidonTetris", "PoseidonTetris");
    return GameApplication::ParseCommandLine(commandLine);
}

bool TetrisApplication::InitializeSubsystems()
{
    // FontSystem brings up FreeType so the ESC-hint overlay can draw
    // through GEngine->DrawText.  Notebook fonts (cwr_*.ttf) live in
    // packages/Tetris/fonts/.
    FontSystem::Instance().Initialize();
    return true;
}

void TetrisApplication::RegisterAudioBackends()
{
    RegisterDummyAudioBackend();
    RegisterTextAudioBackend();
}

bool TetrisApplication::InitializeSound()
{
    CleanupSoundSystem();
    GSoundsys = new SoundSystemDummy();
    return true;
}

bool TetrisApplication::InitializeWorld()
{
    // No 3D world for Tetris — no CfgWorlds / CfgVehicles dependency,
    // so the package can ship without bin/CONFIG.BIN.  Object::Draw
    // still needs GScene for camera + active lights, so allocate a
    // bare Scene + LightSun (LightSun::Recalculate tolerates a null
    // World).  Gated on GEngine because Scene::LoadConfig reads
    // GEngine->Width() — TetrisApplication unit tests construct the
    // app without booting graphics.
    if (!GScene && GEngine)
    {
        ENGINE_CONFIG.noMenuScene = true;
        GScene = new Scene();
        LightSun* mainLight = new LightSun();
        mainLight->Recalculate(nullptr);
        GScene->SetMainLight(mainLight);
        GScene->MainLightChanged();
        GPreloadedTextures_Preload(false);
    }
    return true;
}

void TetrisApplication::RunMainLoop()
{
    // Headless / --check builds don't construct GEngine; bail before
    // anything below dereferences it.
    if (!GEngine)
        return;

    GApp->m_canRender = true;

    {
        // UI overlay font — cwrBodyB24 routes through FontMapping to
        // fonts/cwr_body.ttf at 24 px.  GEngine owns the font; we just
        // borrow the pointer.
        Font* hintFont = GEngine->LoadFont(GetFontID(RString("cwrBodyB24")));
        TetrisGame game;
        TetrisNotebookDisplay notebook(&game);
        if (!notebook.IsLoaded())
        {
            LOG_ERROR(Core, "TetrisApplication: notebook UI failed to load — aborting main loop");
            return;
        }
        LOG_INFO(Core, "TetrisApplication: notebook UI ready, entering render loop");

        float gravityAccumulator = 0.0f;
        float elapsedSeconds = 0.0f;
        constexpr float kGravityStepSeconds = 0.35f;
        bool prevLeft = false;
        bool prevRight = false;
        bool prevRotate = false;
        bool prevDown = false;
        bool prevToggleNotebook = false;

        Uint64 lastTick = SDL_GetPerformanceCounter();
        const double tickFreq = (double)SDL_GetPerformanceFrequency();

        const char* kHintNotebookOpen = "SPACE - close notebook";
        const char* kHintNotebookClosed = "SPACE - open notebook";
        const char* kHintExit = "ESC - close";
        const PackedColor kHintColor(Color(1.0f, 1.0f, 1.0f, 1.0f));
        const PackedColor kBlack(Color(0.0f, 0.0f, 0.0f, 1.0f));
        constexpr float kHintSize = 0.04f;     // normalized text height
        constexpr float kHintMarginX = 0.012f; // ~1% of screen width
        constexpr float kHintMarginY = 0.012f;

        while (!m_closeRequest)
        {
            GEngine->HandleEvents();
            if (!GEngine->IsOpen())
                break;

            // ESC closes — read SDL's keyboard state directly.  The
            // engine's InputSubsystem dispatch path assumes GWorld is up,
            // and Tetris has no world.
            const bool* keys = SDL_GetKeyboardState(nullptr);
            if (keys && keys[SDL_SCANCODE_ESCAPE])
            {
                m_closeRequest = true;
                break;
            }

            const bool toggleNotebook = keys && keys[SDL_SCANCODE_SPACE];
            if (toggleNotebook && !prevToggleNotebook)
            {
                notebook.ToggleOpen();
            }
            prevToggleNotebook = toggleNotebook;

            // Frame delta — clamp so a debugger pause doesn't fast-forward
            // the animation past phase 1 on resume.
            Uint64 now = SDL_GetPerformanceCounter();
            float deltaT = (float)((now - lastTick) / tickFreq);
            lastTick = now;
            if (deltaT > 0.1f)
                deltaT = 0.1f;

            elapsedSeconds += deltaT;
            notebook.Tick(deltaT);
            const bool notebookOpen = notebook.IsOpen();
            if (!notebookOpen)
            {
                gravityAccumulator = 0.0f;
                prevLeft = false;
                prevRight = false;
                prevRotate = false;
                prevDown = false;
            }
            else
            {
                gravityAccumulator += deltaT;
                HandleTetrisInput(game, prevLeft, prevRight, prevRotate, prevDown);
                while (gravityAccumulator >= kGravityStepSeconds)
                {
                    game.AdvanceGravityStep();
                    gravityAccumulator -= kGravityStepSeconds;
                }
                if (game.IsGameOver())
                {
                    game.Reset();
                    gravityAccumulator = 0.0f;
                }
            }

            if (AppConfig::Instance().AppTimeoutSeconds() > 0.0f &&
                elapsedSeconds >= AppConfig::Instance().AppTimeoutSeconds())
            {
                m_closeRequest = true;
            }

            if (!GEngine->IsAbleToDraw())
                continue;
            GEngine->InitDraw(true, kBlack);
            notebook.OnDraw(nullptr, 1.0f);

            if (hintFont)
            {
                GEngine->DrawText(Point2DFloat(kHintMarginX, kHintMarginY), kHintSize, hintFont, kHintColor,
                                  notebook.IsOpen() ? kHintNotebookOpen : kHintNotebookClosed);
                GEngine->DrawText(Point2DFloat(kHintMarginX, kHintMarginY + kHintSize * 1.1f), kHintSize, hintFont,
                                  kHintColor, kHintExit);
            }

            GEngine->FinishDraw();
            GEngine->NextFrame();
        }
    }

    LOG_INFO(Core, "TetrisApplication: main loop exited");
    m_validateQuit = true;
    ProgressFinish();
    DDTerm();
}
