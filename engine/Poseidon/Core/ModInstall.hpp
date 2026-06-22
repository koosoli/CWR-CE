#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace Poseidon
{
/// One installed mod found on disk by ScanLocalMods.
struct ScannedMod
{
    std::string modId;     ///< catalog modId when present, else folder name
    std::string folderName;///< folder name verbatim (e.g. "@fixturemod" or "CSLA")
    std::string name;      ///< mod.json "name", else the folder name (a leading '@' trimmed)
    std::string version;   ///< mod.json "version", else ""
    int64_t sizeBytes = 0; ///< total bytes on disk (recursive), 0 if unknown
};

/// Enumerate installed mods directly under modsRoot: every immediate subdirectory
/// that looks like a mod — one carrying an addons/dta/Campaigns tree, a bin/
/// config override, or a mod.json manifest. The folder name is the mod id; no
/// '@' prefix is required (any name is valid). Sorted by display name; reads each
/// mod.json for name/version (folder-name fallback). Empty when modsRoot isn't a
/// directory.
std::vector<ScannedMod> ScanLocalMods(const std::string& modsRoot);

/// Install state of a catalog mod relative to what's on disk.
enum class ModInstallStatus
{
    NotInstalled,    ///< no matching install directory found by catalog id/folder name
    Installed,       ///< present, version matches the catalog (or version unknown)
    UpdateAvailable  ///< present, but the installed version differs from the catalog
};

/// Directory a catalog mod installs into: <modsRoot>/<folderName>, falling back to @<modId>.
std::string ModInstallDir(const std::string& modsRoot, const std::string& modId);
std::string ModInstallDir(const std::string& modsRoot, const std::string& modId, const std::string& folderName);

/// Existing install directory resolved by modId metadata/folder matching, or "" when absent.
std::string FindInstalledModDir(const std::string& modsRoot, const std::string& modId);

/// Version recorded in an installed mod's mod.json, or "" when not installed or unreadable.
std::string ReadInstalledModVersion(const std::string& modsRoot, const std::string& modId);

/// Compares the installed mod.json version against the catalog version.
ModInstallStatus GetModInstallStatus(const std::string& modsRoot, const std::string& modId,
                                     const std::string& catalogVersion);

/// Writes catalog metadata after an in-game workshop install so later scans and servers
/// resolve/advertise catalog modId even when folderName differs.
bool WriteInstalledModManifest(const std::string& installDir, const std::string& modId, const std::string& name,
                               const std::string& version, const std::string& folderName,
                               const std::string& downloadUrl, int64_t sizeBytes, std::string* error = nullptr);
} // namespace Poseidon
