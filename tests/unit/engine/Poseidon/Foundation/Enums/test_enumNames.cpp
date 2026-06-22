#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Foundation/Enums/EnumNames.hpp>
#include <optional>
#include <string_view>
#include <string>
#include <Poseidon/Foundation/Strings/RString.hpp>

using Poseidon::Foundation::EnumName;
using Poseidon::Foundation::FindEnumName;
using Poseidon::Foundation::GetEnumCount;
using Poseidon::Foundation::GetEnumName;
using Poseidon::Foundation::GetEnumNameCount;
using Poseidon::Foundation::GetEnumNames;
using Poseidon::Foundation::GetEnumValue;
using Poseidon::Foundation::TryGetEnumNameView;
using Poseidon::Foundation::TryGetEnumValue;

#define TEST_COLOR_ENUM(type, prefix, XX) \
    XX(type, prefix, Red)                 \
    XX(type, prefix, Green)               \
    XX(type, prefix, Blue)                \
    XX(type, prefix, Yellow)

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"
// Hand-expanded DECLARE_DEFINE_ENUM(TestColor, TC, TEST_COLOR_ENUM)
// : int — the "invalid value" test casts 999 to TestColor; a fixed underlying
// type makes every int a valid representation, so FindEnumName can load it
// (and return "ERROR") without the out-of-range-enum UB.
enum TestColor : int
{
    TCRed,
    TCGreen,
    TCBlue,
    TCYellow,
    NTestColor
};
template <>
const EnumName* Poseidon::Foundation::GetEnumNames(TestColor)
{
    static const EnumName n[] = {EnumName(TCRed, "Red"), EnumName(TCGreen, "Green"), EnumName(TCBlue, "Blue"),
                                 EnumName(TCYellow, "Yellow"), EnumName()};
    return n;
}
template <>
TestColor Poseidon::Foundation::GetEnumCount(TestColor)
{
    return NTestColor;
}
#pragma clang diagnostic pop

#define TEST_STATE_ENUM(type, prefix, XX) \
    XX(type, prefix, Idle)                \
    XX(type, prefix, Running)             \
    XX(type, prefix, Jumping)             \
    XX(type, prefix, Falling)             \
    XX(type, prefix, Dead)

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"
// Hand-expanded DECLARE_DEFINE_ENUM(TestState, TS, TEST_STATE_ENUM)
enum TestState
{
    TSIdle,
    TSRunning,
    TSJumping,
    TSFalling,
    TSDead,
    NTestState
};
template <>
const EnumName* Poseidon::Foundation::GetEnumNames(TestState)
{
    static const EnumName n[] = {EnumName(TSIdle, "Idle"),       EnumName(TSRunning, "Running"),
                                 EnumName(TSJumping, "Jumping"), EnumName(TSFalling, "Falling"),
                                 EnumName(TSDead, "Dead"),       EnumName()};
    return n;
}
template <>
TestState Poseidon::Foundation::GetEnumCount(TestState)
{
    return NTestState;
}
#pragma clang diagnostic pop

TEST_CASE("EnumName basic operations", "[base][enum][enumname]")
{
    SECTION("Construction with value and name")
    {
        EnumName en(42, "TestValue");

        REQUIRE(en.GetValue() == 42);
        REQUIRE(std::string(en.GetName().Data()) == "TestValue");
        REQUIRE(en.GetNameView() == "TestValue");
        REQUIRE(std::string(en.GetKey()) == "TestValue");
        REQUIRE(en.IsValid());
    }

    SECTION("Default construction")
    {
        EnumName en;

        REQUIRE(en.GetValue() == -1);
        REQUIRE_FALSE(en.IsValid());
    }

    SECTION("IsValid() checks for non-empty name")
    {
        EnumName valid(1, "Valid");
        EnumName invalid(1, "");

        REQUIRE(valid.IsValid());
        REQUIRE_FALSE(invalid.IsValid());
    }
}

TEST_CASE("Modern enum helpers preserve legacy tables", "[base][enum][helpers]")
{
    TestColor dummy = TCRed;
    const EnumName* names = GetEnumNames(dummy);

    SECTION("GetEnumNameCount() counts all valid entries")
    {
        REQUIRE(GetEnumNameCount(names) == 4);
    }

    SECTION("GetEnumNameCount() handles null tables")
    {
        REQUIRE(GetEnumNameCount(nullptr) == 0);
    }

    SECTION("TryGetEnumValue() returns optional matches")
    {
        REQUIRE(TryGetEnumValue(names, "Blue") == std::optional<int>(TCBlue));
        REQUIRE(TryGetEnumValue(names, "blue") == std::optional<int>(TCBlue));
        REQUIRE(TryGetEnumValue(names, "BLUE") == std::optional<int>(TCBlue));
        REQUIRE_FALSE(TryGetEnumValue(names, "Purple").has_value());
        REQUIRE_FALSE(TryGetEnumValue(nullptr, "Blue").has_value());
    }

    SECTION("TryGetEnumNameView() returns optional matches")
    {
        REQUIRE(TryGetEnumNameView(names, TCGreen) == std::optional<std::string_view>("Green"));
        REQUIRE_FALSE(TryGetEnumNameView(names, 999).has_value());
        REQUIRE_FALSE(TryGetEnumNameView(nullptr, TCGreen).has_value());
    }
}

TEST_CASE("Enum factory macro", "[base][enum][factory]")
{
    SECTION("Enum values are defined")
    {
        TestColor red = TCRed;
        TestColor green = TCGreen;
        TestColor blue = TCBlue;
        TestColor yellow = TCYellow;

        REQUIRE(red == 0);
        REQUIRE(green == 1);
        REQUIRE(blue == 2);
        REQUIRE(yellow == 3);
    }

    SECTION("Enum count is defined")
    {
        REQUIRE(NTestColor == 4);
    }

    SECTION("GetEnumNames() returns name table")
    {
        TestColor dummy = TCRed;
        const EnumName* names = GetEnumNames(dummy);

        REQUIRE(names != nullptr);
        REQUIRE(names[0].IsValid());
        REQUIRE(std::string(names[0].GetName().Data()) == "Red");
        REQUIRE(std::string(names[1].GetName().Data()) == "Green");
        REQUIRE(std::string(names[2].GetName().Data()) == "Blue");
        REQUIRE(std::string(names[3].GetName().Data()) == "Yellow");
        REQUIRE_FALSE(names[4].IsValid());
    }

    SECTION("GetEnumCount() returns count")
    {
        TestColor dummy = TCRed;
        TestColor count = GetEnumCount(dummy);

        REQUIRE(count == NTestColor);
        REQUIRE(count == 4);
    }
}

TEST_CASE("GetEnumValue() functions", "[base][enum][getvalue]")
{
    SECTION("GetEnumValue() from RStringB")
    {
        TestColor dummy = TCRed;
        const EnumName* names = GetEnumNames(dummy);

        int red = GetEnumValue(names, RStringB("Red"));
        int green = GetEnumValue(names, RStringB("Green"));
        int blue = GetEnumValue(names, RStringB("Blue"));
        int lowerBlue = GetEnumValue(names, RStringB("blue"));
        int yellow = GetEnumValue(names, RStringB("Yellow"));

        REQUIRE(red == TCRed);
        REQUIRE(green == TCGreen);
        REQUIRE(blue == TCBlue);
        REQUIRE(lowerBlue == TCBlue);
        REQUIRE(yellow == TCYellow);
    }

    SECTION("GetEnumValue() from const char*")
    {
        TestColor dummy = TCRed;
        const EnumName* names = GetEnumNames(dummy);

        int red = GetEnumValue(names, "Red");
        int green = GetEnumValue(names, "Green");
        int lowerGreen = GetEnumValue(names, "green");

        REQUIRE(red == TCRed);
        REQUIRE(green == TCGreen);
        REQUIRE(lowerGreen == TCGreen);
    }

    SECTION("GetEnumValue() with invalid name")
    {
        TestColor dummy = TCRed;
        const EnumName* names = GetEnumNames(dummy);

        int invalid = GetEnumValue(names, "InvalidName");

        REQUIRE(invalid == -1);
    }

    SECTION("GetEnumValue() handles null arguments")
    {
        TestColor dummy = TCRed;
        const EnumName* names = GetEnumNames(dummy);

        REQUIRE(GetEnumValue(names, static_cast<const char*>(nullptr)) == -1);
        REQUIRE(GetEnumValue(nullptr, "Red") == -1);
    }

    SECTION("Template GetEnumValue() from RStringB")
    {
        TestColor red = GetEnumValue<TestColor>(RStringB("Red"));
        TestColor green = GetEnumValue<TestColor>(RStringB("Green"));

        REQUIRE(red == TCRed);
        REQUIRE(green == TCGreen);
    }

    SECTION("Template GetEnumValue() from const char*")
    {
        TestColor red = GetEnumValue<TestColor>("Red");
        TestColor blue = GetEnumValue<TestColor>("Blue");

        REQUIRE(red == TCRed);
        REQUIRE(blue == TCBlue);
    }
}

TEST_CASE("GetEnumName() function", "[base][enum][getname]")
{
    SECTION("GetEnumName() returns name for value")
    {
        TestColor dummy = TCRed;
        const EnumName* names = GetEnumNames(dummy);

        RStringB redName = GetEnumName(names, TCRed);
        RStringB greenName = GetEnumName(names, TCGreen);
        RStringB blueName = GetEnumName(names, TCBlue);

        REQUIRE(std::string(redName.Data()) == "Red");
        REQUIRE(std::string(greenName.Data()) == "Green");
        REQUIRE(std::string(blueName.Data()) == "Blue");
    }

    SECTION("GetEnumName() with invalid value")
    {
        TestColor dummy = TCRed;
        const EnumName* names = GetEnumNames(dummy);

        RStringB invalid = GetEnumName(names, 999);

        REQUIRE(invalid.GetLength() == 0);
    }

    SECTION("GetEnumName() handles null table")
    {
        REQUIRE(GetEnumName(nullptr, TCRed).GetLength() == 0);
    }
}

TEST_CASE("FindEnumName() template function", "[base][enum][findname]")
{
    SECTION("FindEnumName() returns name for value")
    {
        RStringB redName = FindEnumName(TCRed);
        RStringB greenName = FindEnumName(TCGreen);
        RStringB blueName = FindEnumName(TCBlue);
        RStringB yellowName = FindEnumName(TCYellow);

        REQUIRE(std::string(redName.Data()) == "Red");
        REQUIRE(std::string(greenName.Data()) == "Green");
        REQUIRE(std::string(blueName.Data()) == "Blue");
        REQUIRE(std::string(yellowName.Data()) == "Yellow");
    }

    SECTION("FindEnumName() with invalid value returns ERROR")
    {
        RStringB errorName = FindEnumName((TestColor)999);

        REQUIRE(std::string(errorName.Data()) == "ERROR");
    }
}

TEST_CASE("Multiple enums coexist", "[base][enum][multiple]")
{
    SECTION("Different enums have separate name tables")
    {
        TestColor red = TCRed;
        const EnumName* colorNames = GetEnumNames(red);

        TestState idle = TSIdle;
        const EnumName* stateNames = GetEnumNames(idle);

        REQUIRE(colorNames != stateNames);
        REQUIRE(std::string(colorNames[0].GetName().Data()) == "Red");
        REQUIRE(std::string(stateNames[0].GetName().Data()) == "Idle");
    }

    SECTION("GetEnumValue() works for different enums")
    {
        TestColor red = GetEnumValue<TestColor>("Red");
        TestState idle = GetEnumValue<TestState>("Idle");

        REQUIRE(red == TCRed);
        REQUIRE(idle == TSIdle);
    }

    SECTION("FindEnumName() works for different enums")
    {
        RStringB colorName = FindEnumName(TCGreen);
        RStringB stateName = FindEnumName(TSRunning);

        REQUIRE(std::string(colorName.Data()) == "Green");
        REQUIRE(std::string(stateName.Data()) == "Running");
    }
}
