#include <Poseidon/Core/IdString.hpp>
#include <Poseidon/Foundation/Strings/RString.hpp>

namespace Poseidon
{
IdStringTable::IdStringTable() = default;

IdStringTable::IdStringTable(const RStringB* strings, int count)
{
    for (int i = 0; i < count; i++)
    {
        _enum.AddValue(strings[i]);
    }
    _enum.Close();
}

IdStringTable::~IdStringTable() = default;

int IdStringTable::GetId(const char* string)
{
    return _enum.GetValue(string);
}

const RStringB& IdStringTable::GetString(int id) const
{
    // Callers may pass an attacker-derived id (network string-table refs decode a
    // wire varint), so keep it inside the populated range; GetName() indexes its
    // name array unchecked. FirstInvalidValue() is the count of valid entries.
    if (id < 0 || id >= _enum.FirstInvalidValue())
    {
        static const RStringB empty;
        return empty;
    }
    return _enum.GetName(id);
}

IdString IdStringTable::GetIdString(const char* string)
{
    IdString id;
    id._id = _enum.GetValue(string);
    id._string = string;
    return id;
}
} // namespace Poseidon
