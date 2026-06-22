#include <Poseidon/Foundation/Framework/AppFrame.hpp>
#include <Poseidon/IO/ParamFile/ParamFileCtx.hpp>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Poseidon/Foundation/Framework/Log.hpp>

namespace Poseidon
{
int ParamEntryWithContext::GetEntryCount() const
{
    // count entries that are visible in given context
    int c = _entry.GetEntryCount();
    int count = 0;
    for (int i = 0; i < c; i++)
    {
        const ParamEntry& e = _entry.GetEntry(i);
        if (e.IsClass())
        {
            // check if entry is visible
            if (!_visible(_entry, e))
            {
                continue;
            }
        }
        count++;
    }
    return count++;
}

class ParamEntryCtxError : public ParamClass
{
  public:
    ParamEntryCtxError() = default;

    bool IsError() const override { return true; }
};

//! Meyer's singleton for error instance - no global constructor!
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
static ParamEntryCtxError& GetParamEntryCtxError()
{
    static ParamEntryCtxError instance;
    return instance;
}
#pragma clang diagnostic pop

const ParamEntry& ParamEntryWithContext::GetEntry(int index) const
{
    // note: can be slow
    // some enumerator interface should be used instead
    int c = _entry.GetEntryCount();
    for (int i = 0; i < c; i++)
    {
        const ParamEntry& e = _entry.GetEntry(i);
        if (!e.IsClass())
        {
            // check if entry is visible
            if (!_visible(_entry, e))
            {
                continue;
            }
        }
        if (--index < 0)
        {
            return e;
        }
    }
    Fail("Entry not found");
    return GetParamEntryCtxError();
}

ParamEntry* ParamEntryWithContext::FindEntryNoInheritance(const char* name) const
{
    return _entry.FindEntryNoInheritance(name, _visible);
}

ParamEntry* ParamEntryWithContext::FindEntry(const char* name) const
{
    return _entry.FindEntry(name, _visible);
}

ParamEntryWithContext ParamEntryWithContext::operator>>(const RStringB& name) const
{
    const ParamEntry* entry = FindEntry(name);
    if (entry)
    {
        return ParamEntryWithContext(*entry, _visible);
    }
    Poseidon::Foundation::ErrorMessage(Poseidon::Foundation::EMError, "Cannot acces entry '%s'.",
                                       (const char*)GetContext(name));
    return ParamEntryWithContext(GetParamEntryCtxError(), _visible);
}

bool ParamOwnerList::operator()(const ParamEntry& entry)
{
    const RStringB& entryOwner = entry.GetOwner();
    if (entry.GetOwner().GetLength() <= 0)
    {
        return true;
    }
    // check if owner is in the list of known owners
    for (int i = 0; i < _addons.Size(); i++)
    {
        if (_addons[i] == entryOwner)
        {
            return true;
        }
    }
    LOG_DEBUG(Core, "Access denied: {} (owner {})", (const char*)entry.GetContext(), (const char*)entryOwner);
    return false;
}

bool ParamOwnerList::operator()(const ParamEntry& parent, const ParamEntry& entry)
{
    const RStringB& parentOwner = parent.GetOwner();
    const RStringB& entryOwner = entry.GetOwner();
    if (parentOwner == entryOwner)
    {
        return true;
    }
    return (*this)(entry);
}

} // namespace Poseidon
