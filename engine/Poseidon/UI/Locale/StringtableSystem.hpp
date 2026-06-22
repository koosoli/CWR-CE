#pragma once

// StringtableSystem is the engine's gate for i18n string lookups.
// Apps that load `bin/stringtable.csv` from any mod directory during
// configuration parsing flip `IsAvailable()` true; `Localize(int)`
// then returns translated values.
// Apps that don't ship a stringtable (Tetris, utility tools) leave
// the gate false; `Localize(int)` returns "" without log noise.

namespace Poseidon
{
class StringtableSystem
{
public:
    static StringtableSystem& Instance();

    // True when at least one stringtable.csv has been loaded.
    bool IsAvailable() const;

    // Reset the available flag.  Does not unload already-cached
    // table entries — those live in the global StringTable singleton.
    // Test-only helper.
    void Shutdown();

private:
    StringtableSystem() = default;
};

} // namespace Poseidon
