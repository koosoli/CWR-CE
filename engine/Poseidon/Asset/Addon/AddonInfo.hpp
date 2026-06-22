#pragma once

#include <Poseidon/Foundation/Strings/RString.hpp>
#include <Poseidon/Foundation/Containers/Array.hpp>
#include <Poseidon/IO/Streams/QBStream.hpp>
#include <Poseidon/IO/ParamFile/ParamFile.hpp>

namespace Poseidon
{

class AddonInfo
{
    RString _name;
    RString _prefix;

  public:
    AddonInfo() = default;
    AddonInfo(RString name, RString prefix) : _name(name), _prefix(prefix) {}

    const char* GetKey() const { return _name; }
    RString     GetName() const { return _name; }
    RString     GetPrefix() const { return _prefix; }
};

struct AddonDependency
{
    bool                    preloaded;
    const ParamFile*        addon;
    AutoArray<const ParamFile*> dependsOn;

    void Resolved(const AddonDependency& dep);
};

struct CheckAddonContext : public BankContextBase
{
    const char** productList;
    bool         encryptionRequired;
};

struct LoadBanksContext
{
    RString path;
    bool    emptyPrefix;
    bool    parseConfig;
};

} // namespace Poseidon

// TypeIsMovable* macros expand to template specializations that must be at global scope.
