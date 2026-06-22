#include <Poseidon/Core/Application.hpp>
#include <Poseidon/Core/DynEnum.hpp>
#include <stdio.h>
#include <Poseidon/Foundation/Enums/EnumNames.hpp>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Poseidon/Foundation/Framework/Log.hpp>
#include <Poseidon/Foundation/platform.hpp>

namespace Poseidon
{
DynEnumCS::DynEnumCS() = default;

DynEnumCS::~DynEnumCS() = default;

void DynEnumCS::Clear()
{
    _values.Clear();
    _names.Clear();
}

void DynEnumCS::Close()
{
    _names.Add(Foundation::EnumName());
}

int DynEnumCS::GetValue(const char* name) const
{
    const DynEnumValue& value = _values[name];
    if (_values.NotNull(value))
    {
        return value.GetValue();
    }
    return -1;
}

int DynEnumCS::FindValueString(const char* name) const
{
    const DynEnumValue& check = _values[name];
    if (_values.NotNull(check))
    {
        return check.GetValue();
    }
    return -1;
}

int DynEnumCS::AddValueString(const RStringB& name)
{
    int value = _names.Size();
    DynEnumValue newValue(value, name);
    _values.Add(newValue);
    _names.Add(newValue);
    return value;
}

int DynEnumCS::AddValue(const RStringB& name)
{
    if (_values.NItems() != _names.Size())
    {
        Fail("DynEnum has already been closed, cannot add more items");
    }
    const DynEnumValue& check = _values[name];
    if (_values.NotNull(check))
    {
        LOG_DEBUG(Core, "DynEnum: value {} added twice", (const char*)name);
        return check.GetValue();
    }
    return AddValueString(name);
}

#if _DEBUG
struct GetNameContext
{
    int value;
    RStringB name;
};
static void GetNameFind(const DynEnumValue& value, const DynEnumBankType* bank, void* context)
{
    GetNameContext* tContext = static_cast<GetNameContext*>(context);
    if (value.GetValue() == tContext->value)
    {
        tContext->name = value.GetName();
    }
}
#endif

const RStringB& DynEnumCS::GetName(int value) const
{
#if _DEBUG
    GetNameContext context;
    context.value = value;
    _values.ForEach(GetNameFind, &context);
    PoseidonAssert(_names[value].name == context.name);
#endif
    return _names[value].name;
}

int DynEnum::GetValue(const char* name) const
{
    char lowName[256];
    snprintf(lowName, sizeof(lowName), "%s", (const char*)name);
    strlwr(lowName);
    return FindValueString(lowName);
}

int DynEnum::AddValue(const char* name)
{
    char lowName[256];
    snprintf(lowName, sizeof(lowName), "%s", (const char*)name);
    strlwr(lowName);
    int index = FindValueString(lowName);
    if (index >= 0)
    {
        LOG_DEBUG(Core, "DynEnum: value {} added twice", name);
        return index;
    }
    return AddValueString(lowName);
}
} // namespace Poseidon
