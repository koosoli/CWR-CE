#pragma once

#include <cstddef>
#include <functional>
#include <string>

namespace Poseidon
{

/// A mod's identity, normalized for matching. A mod is referred to in many shapes —
/// an absolute mount path (`C:\Workshop\@CSLA`), an `@name` token, a bare folder
/// name, in any case — but they all denote the same mod. `ModId` collapses a token
/// to its canonical form (basename, no leading '@', lowercased) on construction, so
/// every comparison across the loader, the catalog, and a server's required list
/// goes through one type instead of ad-hoc `stricmp`s.
class ModId
{
  public:
    ModId() = default;
    explicit ModId(const std::string& token);

    /// Canonical lowercased id, e.g. "csla_sounds". Empty for an empty/blank token.
    const std::string& Value() const { return _id; }
    bool Empty() const { return _id.empty(); }

    bool operator==(const ModId& o) const { return _id == o._id; }
    bool operator!=(const ModId& o) const { return _id != o._id; }
    bool operator<(const ModId& o) const { return _id < o._id; }

    /// Hash so a ModId can key an unordered_set/map.
    struct Hash
    {
        std::size_t operator()(const ModId& m) const { return std::hash<std::string>{}(m._id); }
    };

  private:
    std::string _id;
};

} // namespace Poseidon
