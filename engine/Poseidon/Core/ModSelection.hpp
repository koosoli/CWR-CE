#pragma once

#include <string>
#include <vector>

namespace Poseidon
{
/// Reads active mod ids — one "@modId" token per line; blank lines and lines
/// starting with // or # are ignored. Returns an empty list if the file is
/// absent or unreadable.
std::vector<std::string> LoadModSelection(const std::string& cfgPath);

/// Writes active mod ids, one per line (parent directory created if needed).
/// Returns false on write failure.
bool SaveModSelection(const std::string& cfgPath, const std::vector<std::string>& mods);
} // namespace Poseidon
