#pragma once


/// ConfigSystem reports which engine config files actually loaded
/// during boot.  `ConfigurationSystem::InitializeGameConfiguration`
/// records the result of each `ModSystem::EnumDirectories` pass into
/// this façade; engine code can then check `IsConfigAvailable()` (or
/// equivalents) before reaching into `Pars >> "CfgFoo"` paths.
///
/// Apps that ship `bin/config.bin` + `bin/resource.cpp` (Game) see
/// every flag true.  Apps that ship none (Tetris, utility tools) see
/// every flag false — without ERR log noise from the missing files.

namespace Poseidon
{
class ConfigSystem
{
public:
    static ConfigSystem& Instance();

    /// True if at least one mod directory contributed a `config.bin`
    /// or `config.cpp`.  Drives whether `Pars` holds engine class
    /// definitions (CfgWorlds, CfgVehicles, etc.).
    bool IsConfigAvailable() const { return _configAvailable; }

    /// True if at least one mod directory contributed a `resource.cpp`
    /// or `resource.bin` (UI definitions: RscTitles, RscDisplay*, …).
    bool IsResourceAvailable() const { return _resourceAvailable; }

    /// True if at least one mod directory contributed a `remaster.cpp`
    /// or `remaster.bin` (Remaster-only asset catalog).
    bool IsRemasterAvailable() const { return _remasterAvailable; }

    /// Recorders called by the configuration boot path.  Not for
    /// caller use outside `ConfigurationSystem`.
    void MarkConfigLoaded(bool loaded) { _configAvailable = loaded; }
    void MarkResourceLoaded(bool loaded) { _resourceAvailable = loaded; }
    void MarkRemasterLoaded(bool loaded) { _remasterAvailable = loaded; }

    /// Reset all flags.  Test-only helper.
    void Shutdown();

private:
    ConfigSystem() = default;
    bool _configAvailable = false;
    bool _resourceAvailable = false;
    bool _remasterAvailable = false;
};
} // namespace Poseidon
