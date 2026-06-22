# Engine

Static libraries that form the engine stack.

```
Poseidon (+ PoseidonGL33, PoseidonOpenAL)  ← full game engine
```

| Library | Directory | Output | Description |
|---------|-----------|--------|-------------|
| **Poseidon** | `Poseidon/` | `Poseidon.lib` | Engine — AI, audio, entities, world, scripting, foundation/runtime support, evaluator, IO (streams, ParamFile, preproc), graphics interface + factory, UI, locale |
| **PoseidonGL33** | `PoseidonGL33/` | `PoseidonGL33.lib` | OpenGL 3.3 graphics backend (linked by client apps) |
| **PoseidonOpenAL** | `PoseidonOpenAL/` | `PoseidonOpenAL.lib` | OpenAL audio backend (linked by client apps) |
| **PoseidonFormats** | `PoseidonFormats/` | `.dll` / `.lib` | C API for P3D/PAA/PBO/RTM (used by Blender addon) |
| **Trident** | `Trident/` | `tri` | Rust-based test runner and integration tool |
