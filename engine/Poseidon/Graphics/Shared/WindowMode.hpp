#pragma once

// Window mode shared between the live renderer (EngineGL33) and the
// pure placement resolver (WindowPlacement).  Lives in its own header
// so unit tests that only need the enum don't have to pull in the
// full Engine.hpp render graph (TexMaterial, font, etc.).

namespace Poseidon
{
enum class WindowMode
{
    Fullscreen = 0,
    Borderless = 1,
    Windowed = 2,
};

} // namespace Poseidon
