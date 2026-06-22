// Unit tests for Poseidon/Foundation/Types/EnumDecl.hpp - Portable enum forward declaration system
// Tests all three compilation modes and enum usage patterns

// This file tests Microsoft enum forward declaration extensions
#include <catch2/catch_message.hpp>
#include <Poseidon/Foundation/platform.hpp>
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmicrosoft-enum-forward-reference"
#endif

#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Poseidon/Foundation/Types/EnumDecl.hpp>

// Test Enums - Forward Declaration and Definition

// Forward declare enum (allows use in headers before definition)
DECL_ENUM(TestEnum)
DECL_ENUM(ColorEnum)
DECL_ENUM(StatusEnum)

// Define enum values
DEFINE_ENUM_BEG(TestEnum)
TestValue0 = 0, TestValue1 = 1, TestValue2 = 2, TestValue3 = 10,
                TestValueMax = 100 DEFINE_ENUM_END(TestEnum)

                    DEFINE_ENUM_BEG(ColorEnum) ColorRed = 1,
                ColorGreen = 2, ColorBlue = 4, ColorYellow = ColorRed | ColorGreen, ColorMagenta = ColorRed | ColorBlue,
                ColorCyan = ColorGreen | ColorBlue DEFINE_ENUM_END(ColorEnum)

                                             DEFINE_ENUM_BEG(StatusEnum) StatusInactive,
                StatusActive, StatusPending, StatusError DEFINE_ENUM_END(StatusEnum)

// Helper Functions Using Enum Parameters

TestEnum GetTestValue()
{
    return TestValue1;
}

void SetTestValue(TestEnum value)
{
    // Function that takes enum as parameter
}

int GetEnumAsInt(TestEnum value)
{
    return value;
}

// Basic Enum Tests

TEST_CASE("enum_decl - Basic enum values", "[enum_decl][basic]")
{
    SECTION("Enum values have correct integer values")
    {
        REQUIRE(TestValue0 == 0);
        REQUIRE(TestValue1 == 1);
        REQUIRE(TestValue2 == 2);
        REQUIRE(TestValue3 == 10);
        REQUIRE(TestValueMax == 100);
    }

    SECTION("Enum can be assigned")
    {
        TestEnum e = TestValue1;
        REQUIRE(e == TestValue1);
        REQUIRE(e == 1);

        e = TestValue3;
        REQUIRE(e == TestValue3);
        REQUIRE(e == 10);
    }

    SECTION("Enum with sequential values")
    {
        REQUIRE(StatusInactive == 0);
        REQUIRE(StatusActive == 1);
        REQUIRE(StatusPending == 2);
        REQUIRE(StatusError == 3);
    }

    SECTION("Enum with bit flags")
    {
        REQUIRE(ColorRed == 1);
        REQUIRE(ColorGreen == 2);
        REQUIRE(ColorBlue == 4);
        REQUIRE(ColorYellow == 3);  // Red | Green
        REQUIRE(ColorMagenta == 5); // Red | Blue
        REQUIRE(ColorCyan == 6);    // Green | Blue
    }
}

TEST_CASE("enum_decl - Enum comparison", "[enum_decl][compare]")
{
    SECTION("Compare enum values")
    {
        TestEnum e1 = TestValue1;
        TestEnum e2 = TestValue1;
        TestEnum e3 = TestValue2;

        REQUIRE(e1 == e2);
        REQUIRE(e1 != e3);
        REQUIRE(e1 == TestValue1);
        REQUIRE(e3 == TestValue2);
    }

    SECTION("Compare with integer")
    {
        TestEnum e = TestValue3;
        REQUIRE(e == 10);
        REQUIRE(10 == e);

        int i = e;
        REQUIRE(i == 10);
    }

    SECTION("Use in conditionals")
    {
        StatusEnum status = StatusActive;

        if (status == StatusActive)
        {
            REQUIRE(true);
        }
        else
        {
            REQUIRE(false);
        }

        bool isActive = (status == StatusActive);
        REQUIRE(isActive);
    }
}

TEST_CASE("enum_decl - Function parameters and return values", "[enum_decl][function]")
{
    SECTION("Return enum from function")
    {
        TestEnum e = GetTestValue();
        REQUIRE(e == TestValue1);
    }

    SECTION("Pass enum to function")
    {
        SetTestValue(TestValue2);
        REQUIRE(true); // Should compile and run
    }

    SECTION("Convert enum to int")
    {
        int value = GetEnumAsInt(TestValue3);
        REQUIRE(value == 10);
    }
}

TEST_CASE("enum_decl - Default constructor and assignment", "[enum_decl][constructor]")
{
    SECTION("Default construction")
    {
        TestEnum e;
        // Default constructed enum has undefined value
        // Just verify it compiles
        e = TestValue1;
        REQUIRE(e == TestValue1);
    }

    SECTION("Construction from value")
    {
#if _MSC_VER && !NO_ENUM_FORWARD_DECLARATION
        // Native enum - direct construction works
        TestEnum e = TestValue2;
        REQUIRE(e == TestValue2);
#else
        // Workaround mode - construction from enum value
        TestEnum e(TestValue2);
        REQUIRE(e == TestValue2);
#endif
    }

    SECTION("Copy construction")
    {
        TestEnum e1 = TestValue3;
        TestEnum e2 = e1;
        REQUIRE(e2 == TestValue3);
    }

    SECTION("Assignment")
    {
        TestEnum e1 = TestValue1;
        TestEnum e2 = TestValue2;

        e1 = e2;
        REQUIRE(e1 == TestValue2);
        REQUIRE(e2 == TestValue2);
    }
}

TEST_CASE("enum_decl - Implicit conversions", "[enum_decl][conversion]")
{
    SECTION("Enum to int conversion")
    {
        TestEnum e = TestValue3;
        int i = e;
        REQUIRE(i == 10);
    }

    SECTION("Use in arithmetic")
    {
        TestEnum e = TestValue1;
        int sum = e + 5;
        REQUIRE(sum == 6);

        int product = e * 3;
        REQUIRE(product == 3);
    }

    SECTION("Use in switch statement")
    {
        StatusEnum status = StatusPending;
        int result = 0;

        switch (status)
        {
            case StatusInactive:
                result = 1;
                break;
            case StatusActive:
                result = 2;
                break;
            case StatusPending:
                result = 3;
                break;
            case StatusError:
                result = 4;
                break;
        }

        REQUIRE(result == 3);
    }
}

TEST_CASE("enum_decl - Array and container usage", "[enum_decl][container]")
{
    SECTION("Array of enums")
    {
        TestEnum arr[3];
        arr[0] = TestValue0;
        arr[1] = TestValue1;
        arr[2] = TestValue2;

        REQUIRE(arr[0] == TestValue0);
        REQUIRE(arr[1] == TestValue1);
        REQUIRE(arr[2] == TestValue2);
    }

    SECTION("Enum as array index")
    {
        int counts[4] = {0};
        StatusEnum status = StatusPending;

        counts[status]++;
        REQUIRE(counts[StatusPending] == 1);
    }

    SECTION("Initialize array with enum values")
    {
        TestEnum values[] = {TestValue0, TestValue1, TestValue2, TestValue3};

        REQUIRE(values[0] == TestValue0);
        REQUIRE(values[1] == TestValue1);
        REQUIRE(values[2] == TestValue2);
        REQUIRE(values[3] == TestValue3);
    }
}

TEST_CASE("enum_decl - Bitwise operations (flags)", "[enum_decl][bitwise]")
{
    SECTION("Bitwise OR")
    {
        ColorEnum color = ColorRed;
        int combined = color | ColorBlue;
        REQUIRE(combined == (1 | 4));
        REQUIRE(combined == 5);
    }

    SECTION("Bitwise AND for flag testing")
    {
        ColorEnum color = ColorYellow; // Red | Green

        int hasRed = color & ColorRed;
        int hasBlue = color & ColorBlue;

        REQUIRE(hasRed != 0);
        REQUIRE(hasBlue == 0);
    }

    SECTION("Multiple flags")
    {
        int flags = ColorRed | ColorGreen | ColorBlue;
        REQUIRE(flags == 7);

        REQUIRE((flags & ColorRed) != 0);
        REQUIRE((flags & ColorGreen) != 0);
        REQUIRE((flags & ColorBlue) != 0);
    }
}

TEST_CASE("enum_decl - Edge cases", "[enum_decl][edge]")
{
    SECTION("Enum with single value")
    {
        DECL_ENUM(SingleValueEnum)
        DEFINE_ENUM_BEG(SingleValueEnum)
        SingleValue = 42 DEFINE_ENUM_END(SingleValueEnum)

            SingleValueEnum e = SingleValue;
        REQUIRE(e == 42);
    }

    SECTION("Enum with negative values")
    {
        DECL_ENUM(SignedEnum)
        DEFINE_ENUM_BEG(SignedEnum)
        NegativeValue = -1, ZeroValue = 0,
        PositiveValue = 1 DEFINE_ENUM_END(SignedEnum)

            SignedEnum e = NegativeValue;
        REQUIRE(e == -1);

        e = ZeroValue;
        REQUIRE(e == 0);

        e = PositiveValue;
        REQUIRE(e == 1);
    }

    SECTION("Enum with large values")
    {
        DECL_ENUM(LargeEnum)
        DEFINE_ENUM_BEG(LargeEnum)
        LargeValue = 1000000 DEFINE_ENUM_END(LargeEnum)

            LargeEnum e = LargeValue;
        REQUIRE(e == 1000000);
    }
}

TEST_CASE("enum_decl - Documentation and usage patterns", "[enum_decl][doc]")
{
    SECTION("Forward declaration pattern")
    {
        INFO("Step 1: Forward declare enum in header with DECL_ENUM(Name)");
        INFO("Step 2: Define enum values with DEFINE_ENUM_BEG/END");
        INFO("Step 3: Use enum type before definition");

        // This is the pattern used throughout the codebase
        // Allows header files to reference enum types before they're defined

        REQUIRE(true);
    }

    SECTION("Three compilation modes")
    {
        INFO("Mode 1 (_MSC_VER && !NO_ENUM_FORWARD_DECLARATION):");
        INFO("  - Native MSVC enum forward declaration");
        INFO("  - Most efficient, direct enum usage");

        INFO("Mode 2 (!NO_UNDEF_ENUM_REFERENCE):");
        INFO("  - Workaround using enum_base class");
        INFO("  - Supports implicit conversion from enum values");

        INFO("Mode 3 (default):");
        INFO("  - Workaround with template constructor");
        INFO("  - Most compatible but may need explicit construction");

        REQUIRE(true);
    }

    SECTION("Best practices")
    {
        INFO("1. Always forward declare enums before use");
        INFO("2. Use meaningful enum names (not just E1, E2)");
        INFO("3. Document enum values, especially bit flags");
        INFO("4. Consider using explicit values for stability");
        INFO("5. Use bit flags (powers of 2) for combinable options");

        REQUIRE(true);
    }
}

TEST_CASE("enum_decl - Real-world usage patterns", "[enum_decl][realworld]")
{
    SECTION("State machine")
    {
        StatusEnum state = StatusInactive;

        // Initialize
        state = StatusActive;
        REQUIRE(state == StatusActive);

        // Transition
        state = StatusPending;
        REQUIRE(state == StatusPending);

        // Error handling
        state = StatusError;
        REQUIRE(state == StatusError);
    }

    SECTION("Configuration flags")
    {
        int flags = 0;

        // Enable features
        flags |= ColorRed;
        flags |= ColorGreen;

        // Check if enabled
        REQUIRE((flags & ColorRed) != 0);
        REQUIRE((flags & ColorGreen) != 0);
        REQUIRE((flags & ColorBlue) == 0);

        // Disable feature
        flags &= ~ColorRed;
        REQUIRE((flags & ColorRed) == 0);
        REQUIRE((flags & ColorGreen) != 0);
    }

    SECTION("Function return codes")
    {
        auto getStatus = []() -> StatusEnum
        {
            // Simulate some operation
            return StatusActive;
        };

        StatusEnum result = getStatus();

        if (result == StatusActive)
        {
            REQUIRE(true);
        }
        else if (result == StatusError)
        {
            REQUIRE(false);
        }
    }
}

// Compilation Mode Tests

TEST_CASE("enum_decl - Verify compilation mode", "[enum_decl][mode]")
{
    SECTION("Detect compilation mode")
    {
#if _MSC_VER && !NO_ENUM_FORWARD_DECLARATION
        INFO("Mode: Native MSVC enum forward declaration");
        INFO("  - Using 'enum Name;' syntax");
        INFO("  - Most efficient implementation");
        REQUIRE(true);
#elif !NO_UNDEF_ENUM_REFERENCE
        INFO("Mode: enum_base workaround with enum reference");
        INFO("  - Using class-based enum wrapper");
        INFO("  - Supports implicit conversion");
        REQUIRE(true);
#else
        INFO("Mode: enum_base workaround with template constructor");
        INFO("  - Using class-based enum wrapper");
        INFO("  - Requires explicit construction in some cases");
        REQUIRE(true);
#endif
    }

    SECTION("All modes support basic operations")
    {
        TestEnum e1 = TestValue1;
        TestEnum e2 = TestValue2;

        // Assignment
        e1 = e2;
        REQUIRE(e1 == e2);

        // Comparison
        REQUIRE(e1 == TestValue2);

        // Conversion to int
        int i = e1;
        REQUIRE(i == 2);

        // Function parameter
        SetTestValue(e1);

        REQUIRE(true);
    }
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif
