#pragma once

#include "GameApplication.hpp"

/// Tetris client.  Renders the notebook game against a black background
/// with the same shell chrome used by the in-game options menu.
class TetrisApplication : public GameApplication
{
  public:
    /// No game-content modules registered — Tetris doesn't load
    /// missions, campaigns, or multiplayer.
    void RegisterGameModules() override;

    /// Parse the command line and seed `GamePaths` with a Tetris-only
    /// user-data dir so display.cfg / graphics.cfg / audio.cfg from the
    /// main Cold War Assault install don't leak in here — Tetris always
    /// boots against fresh defaults.
    bool ParseCommandLine(const char* commandLine) override;

    /// Bring up FontSystem so the ESC hint can render through the
    /// FreeType path.  Tetris ships no scene preload / PIII tuning.
    bool InitializeSubsystems() override;
    void RegisterAudioBackends() override;

    /// Construct a dummy audio backend instead of opening an OpenAL
    /// device.  Consumers that touch GSoundsys keep working through
    /// the no-op stub; no audio.cfg parse, no listener orientation,
    /// no EFX probing.
    bool InitializeSound() override;

    /// Skip world/landscape construction.  Tetris doesn't render a 3D
    /// world — no CfgWorlds lookups, no vehicle types, no landscape.
    bool InitializeWorld() override;

    /// Custom main loop: black clear, notebook draw, ESC hint, swap.
    /// Exits when SDL window close fires or ESC is pressed.
    void RunMainLoop() override;
};
