#pragma once

#include <string>
#include <vector>

/// FontSystem is the engine's gate for FreeType-backed text rendering.
///
/// Apps that want to render text call `Initialize()` early in their boot
/// path.  Initialize validates the active slot's TTFs by opening them
/// strictly — any missing file aborts the boot with a named error.
/// Once `IsAvailable()` is true, every `Font::Load` proceeds normally
/// and the existing slot-cycling / live-tuning code keeps working.
///
/// Apps that don't render text (utility / preview tools) skip
/// `Initialize`.  `Font::Load` then short-circuits to an empty font
/// without logging an error.  No TTF / FXY assets are required on disk.

namespace Poseidon
{
class FontSystem
{
public:
    static FontSystem& Instance();

    /// Open + parse every TTF referenced by the active font slot.
    /// Aborts via LOG_ERROR + exit if a required file is missing or
    /// fails to parse.  Idempotent — second call is a no-op.
    void Initialize();

    /// Release any open renderers and reset the available flag.
    void Shutdown();

    /// True after a successful `Initialize`, false before / after Shutdown.
    bool IsAvailable() const { return _initialized; }

    /// Returns the list of required TTF paths that aren't openable
    /// from the current VFS / cwd.  Empty list means `Initialize`
    /// will succeed.  Exposed for unit tests + diagnostics.
    static std::vector<std::string> RequiredFontsMissing();

    /// The slot-0 TTF file list FontSystem checks at Initialize.
    /// Kept in sync with `font.cpp`'s s_slot0 table; the test suite
    /// pins both lists against each other.
    static std::vector<std::string> RequiredFonts();

private:
    FontSystem() = default;
    bool _initialized = false;
};

} // namespace Poseidon
