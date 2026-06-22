#ifdef _MSC_VER
#pragma once
#endif

#ifndef _ENUM_NAMES_HPP
#define _ENUM_NAMES_HPP

#include <Poseidon/Foundation/Strings/RString.hpp>
#include <optional>
#include <string>
#include <string_view>


namespace Poseidon::Foundation
{
struct EnumName
{
    int value = -1;
    RStringB name;

    EnumName() = default;
    EnumName(int v, const RStringB& n)
        : value(v)
        , name(n)
    {
    }
    EnumName(int v, const char* n)
        : value(v)
        , name(n)
    {
    }

    [[nodiscard]] const char* GetKey() const { return name; }
    [[nodiscard]] RStringB GetName() const { return name; }
    [[nodiscard]] std::string_view GetNameView() const
    {
        return std::string_view(name.Data(), static_cast<size_t>(name.GetLength()));
    }
    [[nodiscard]] int GetValue() const { return value; }
    [[nodiscard]] bool IsValid() const { return name.GetLength() > 0; }
};

size_t GetEnumNameCount(const EnumName* names);
std::optional<int> TryGetEnumValue(const EnumName* names, std::string_view name);
std::optional<std::string_view> TryGetEnumNameView(const EnumName* names, int value);
int GetEnumValue(const EnumName* names, const RStringB& name);
int GetEnumValue(const EnumName* names, const char* name);
RStringB GetEnumName(const EnumName* names, int value);

template <class Type>
const EnumName* GetEnumNames(Type);

template <class Type>
Type GetEnumCount(Type);

template <class Type>
Type GetEnumValue(const RStringB& name)
{
    Type dummy = (Type)0;
    return (Type)GetEnumValue(GetEnumNames(dummy), name);
}

template <class Type>
Type GetEnumValue(const char* name)
{
    Type dummy = (Type)0;
    return (Type)GetEnumValue(GetEnumNames(dummy), name);
}

template <class Type>
RStringB FindEnumName(Type value)
{
    if (std::optional<std::string_view> name = TryGetEnumNameView(GetEnumNames(value), value))
    {
        return RStringB(std::string(*name).c_str());
    }
    return "ERROR";
}

#define ENUM_VALUE(type, typeprefix, name) typeprefix##name,

#define ENUM_NAME(type, typeprefix, name) ::Poseidon::Foundation::EnumName(typeprefix##name, #name),

#define DECLARE_ENUM(type, prefix, ENUM_DEF)                                      \
    enum type                                                                     \
    {                                                                             \
        ENUM_DEF(type, prefix, ENUM_VALUE) N##type                                \
    };                                                                            \
    template <>                                                                   \
    const ::Poseidon::Foundation::EnumName* ::Poseidon::Foundation::GetEnumNames(type);

#define DEFINE_ENUM(type, prefix, ENUM_DEF)                                                                            \
    template <>                                                                                                        \
    const ::Poseidon::Foundation::EnumName* ::Poseidon::Foundation::GetEnumNames(type)                                \
    {                                                                                                                  \
        static const ::Poseidon::Foundation::EnumName type##Names[] = {ENUM_DEF(type, prefix, ENUM_NAME) ::Poseidon::Foundation::EnumName()}; \
        return type##Names;                                                                                            \
    }                                                                                                                  \
                                                                                                                       \
    template <>                                                                                                        \
    type ::Poseidon::Foundation::GetEnumCount(type)                                                                    \
    {                                                                                                                  \
        return N##type;                                                                                                \
    }

#define DECLARE_DEFINE_ENUM(type, prefix, def) \
    DECLARE_ENUM(type, prefix, def)            \
    DEFINE_ENUM(type, prefix, def)

} // namespace Poseidon::Foundation

#endif
