#pragma once

#ifndef _PARAM_FILE_CTX_HPP
#define _PARAM_FILE_CTX_HPP

#include <Poseidon/IO/ParamFile/ParamFile.hpp>

// A context-aware variant of ParamEntry: most reads forward straight to the
// wrapped ParamEntry, but some take the visibility context into account.
namespace Poseidon
{

class ParamEntryWithContext
{
    IParamVisibleTest& _visible;
    const ParamEntry& _entry;

  public:
    ParamEntryWithContext(const ParamEntry& entry, IParamVisibleTest& visible) : _visible(visible), _entry(entry) {}

    // Forwarded straight to the wrapped ParamEntry.
    const RStringB& GetName() const { return _entry.GetName(); }
    RString GetContext(const char* member = nullptr) const { return _entry.GetContext(); }
    const ParamEntry& GetEntry() const { return _entry; }

    bool IsClass() const { return _entry.IsClass(); }
    bool IsArray() const { return _entry.IsArray(); }
    bool IsError() const { return _entry.IsError(); }

    const RStringB& GetOwner() const { return _entry.GetOwner(); }

    // value conversions
    operator RStringB() const { return (RStringB)_entry; }
    operator float() const { return (float)_entry; }
    operator int() const { return (int)_entry; }
    operator bool() const { return (bool)_entry; }
    int GetInt() const { return _entry.GetInt(); }
    RStringB GetValueRaw() const { return _entry.GetValueRaw(); }
    const IParamArrayValue& operator[](int index) const { return _entry[index]; }
    int GetSize() const { return _entry.GetSize(); }

    // Take the visibility context into account.
    int GetEntryCount() const;
    const ParamEntry& GetEntry(int index) const;
    ParamEntryWithContext operator>>(const RStringB& name) const;
    ParamEntry* FindEntryNoInheritance(const char* name) const;
    ParamEntry* FindEntry(const char* name) const;
};

} // namespace Poseidon
#endif
