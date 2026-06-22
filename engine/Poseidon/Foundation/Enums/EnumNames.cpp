#include <Poseidon/Foundation/Enums/EnumNames.hpp>
#include <Poseidon/Foundation/Strings/RString.hpp>
#include <cctype>
#include <string>


namespace Poseidon::Foundation
{
namespace
{
std::string_view ToStringView(const RStringB& name)
{
    return std::string_view(name.Data(), static_cast<size_t>(name.GetLength()));
}

RStringB ToRString(std::string_view name)
{
    std::string ownedName(name);
    return RStringB(ownedName.c_str());
}

bool EqualsIgnoreAsciiCase(std::string_view lhs, std::string_view rhs)
{
    if (lhs.size() != rhs.size())
    {
        return false;
    }

    for (size_t i = 0; i < lhs.size(); ++i)
    {
        const auto left = static_cast<unsigned char>(lhs[i]);
        const auto right = static_cast<unsigned char>(rhs[i]);
        if (std::tolower(left) != std::tolower(right))
        {
            return false;
        }
    }

    return true;
}
}

size_t GetEnumNameCount(const EnumName* names)
{
    if (!names)
    {
        return 0;
    }

    size_t count = 0;
    while (names[count].IsValid())
    {
        count++;
    }

    return count;
}

std::optional<int> TryGetEnumValue(const EnumName* names, std::string_view name)
{
    const size_t count = GetEnumNameCount(names);
    for (size_t i = 0; i < count; ++i)
    {
        if (EqualsIgnoreAsciiCase(names[i].GetNameView(), name))
        {
            return names[i].value;
        }
    }

    return std::nullopt;
}

std::optional<std::string_view> TryGetEnumNameView(const EnumName* names, int value)
{
    const size_t count = GetEnumNameCount(names);
    for (size_t i = 0; i < count; ++i)
    {
        if (names[i].value == value)
        {
            return names[i].GetNameView();
        }
    }

    return std::nullopt;
}

int GetEnumValue(const EnumName* names, const RStringB& name)
{
    return TryGetEnumValue(names, ToStringView(name)).value_or(-1);
}

int GetEnumValue(const EnumName* names, const char* name)
{
    if (!names || !name)
    {
        return -1;
    }

    return TryGetEnumValue(names, name).value_or(-1);
}

RStringB GetEnumName(const EnumName* names, int value)
{
    if (std::optional<std::string_view> name = TryGetEnumNameView(names, value))
    {
        return ToRString(*name);
    }

    return RStringB();
}

} // namespace Poseidon::Foundation
