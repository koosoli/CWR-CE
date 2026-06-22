#ifdef _MSC_VER
#pragma once
#endif

#ifndef _LOCALIZED_STRING_HPP
#define _LOCALIZED_STRING_HPP

#include <Poseidon/Foundation/Strings/RString.hpp>
#include <cstdint>

namespace Poseidon
{
class ParamEntry;

// A localized config string that is resolved on demand rather than materialized
// once. It binds to a config value node (e.g. `cls >> "displayName"`) and leaves
// the actual `$STR_…` -> text expansion to ParamEntry, which already localizes
// against the *current* language on every read. The resolved text is cached and
// only re-resolved when the language generation advances (see
// GetLanguageGeneration), so steady-state reads are a single integer compare and
// a runtime language switch transparently produces fresh text everywhere the
// value is read.
//
// Use this instead of snapshotting a localized config value into an RStringB at
// load time: a snapshot leaves HUD / menu text stuck in the boot language after
// an in-game language change.
//
// Lifetime: the bound entry must outlive this object. Config nodes live for the
// lifetime of the loaded config (Pars and friends), which is the whole process,
// so binding to `someClass >> "key"` is safe. Not thread-safe; resolve on the
// thread that owns the config/UI.
class LocalizedString
{
    const ParamEntry* _value = nullptr; // bound config value node, or null when unbound
    mutable RStringB _cache;            // last resolved text
    mutable uint32_t _gen = 0;          // language generation _cache was resolved under (0 = never)

public:
    LocalizedString() = default;

    // Bind to a config value entry (typically `cls >> "key"`). Passing a missing
    // entry is fine: ParamEntry returns the empty/error value, which resolves to "".
    void Bind(const ParamEntry& value)
    {
        _value = &value;
        _gen = 0; // force a resolve on next read
    }

    void Clear()
    {
        _value = nullptr;
        _cache = RStringB();
        _gen = 0;
    }

    bool IsBound() const { return _value != nullptr; }

    // Resolve against the current language, re-resolving only when the language
    // generation has advanced since the last call. Returns a reference into the
    // cached value (valid for the lifetime of this object).
    const RStringB& Get() const;

    operator RStringB() const { return Get(); }
    const char* CStr() const { return Get().Data(); }
};

} // namespace Poseidon

#endif
