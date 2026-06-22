# Apps

Application targets and tool entry points live under grouped app families. CMake
builds the compiled targets from these subdirectories; the Blender addon is a
Python package that uses the `PoseidonFormats` shared library.

## Layout

| Path | Contents |
| --- | --- |
| `cwr/Game/` | Main game client target. |
| `cwr/GameDemo/` | Demo-restricted game client target. |
| `cwr/GameBase/` | Shared application startup and runtime support for game clients. |
| `cwr/Server/` | Dedicated server target. |
| `tools/Tools/` | `PoseidonTools` command-line utility. |
| `tools/Evaluator/` | SQF evaluator command-line app. |
| `tools/Studio/` | ImGui-based editor/tool shell. |
| `tools/TcPbo/` | PBO module target with output name `pbo`. |
| `tools/TcLister/` | file-listing module target with output name `poseidon`. |
| `tools/BlenderAddon/` | Blender P3D importer addon. |
| `tetris/Tetris/` | `PoseidonTetris` sample/game target. |
| `fuzzers/Fuzzer/` | Fuzzer harness targets. |

## Build Targets

| Target | Output | Type | Notes |
| --- | --- | --- | --- |
| `PoseidonGame` | `PoseidonGame` | GUI | Main game client. |
| `PoseidonGameDemo` | `PoseidonGameDemo` | GUI | Demo package client. |
| `PoseidonServer` | `PoseidonServer` | Console | Dedicated server. |
| `PoseidonTools` | `PoseidonTools` | Console | Asset, PBO, image, sound, scan, and model utilities. |
| `PoseidonEvaluator` | `PoseidonEvaluator` | Console | SQF expression evaluator. |
| `PoseidonStudio` | `PoseidonStudio` | GUI | ImGui-based studio tooling. |
| `PoseidonTetris` | `PoseidonTetris` | GUI | Notebook/Tetris sample target. |
| `TcPbo` | `pbo` | Module | PBO archive module. |
| `TcLister` | `poseidon` | Module | File-listing module. |

Client-style GUI targets link the engine and the graphics/audio backends they
need. Tool targets link the narrower engine libraries required by their command
surface.

## Blender Addon

The maintained addon lives under `tools/BlenderAddon/`; see
[`tools/BlenderAddon/README.md`](tools/BlenderAddon/README.md) for packaging,
installation, and test commands.
