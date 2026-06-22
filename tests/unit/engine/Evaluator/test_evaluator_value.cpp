#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Evaluator/express.hpp>
#include <string>
#include <Poseidon/Foundation/Strings/RString.hpp>

// ============================================================================
// Evaluator testing - Phase 1.2: GameValue wrapper
// ============================================================================
// Testing the GameValue class that wraps GameData objects with type-safe
// conversions and automatic reference counting.
//
// GameValue is the main interface for working with evaluator values:
// - Type-safe constructors for each data type
// - Implicit conversions to native types
// - Copy semantics with reference counting
// - Shared instance detection (for arrays)
// ============================================================================

// ============================================================================
// Test 1: Construction from Different Types
// ============================================================================

TEST_CASE("GameValue - Construct from scalar", "[evaluator][value][construct]")
{
    SECTION("Construct from float")
    {
        GameValue value(42.5f);

        REQUIRE(value.GetType() == GameScalar);
        REQUIRE((float)value == Catch::Approx(42.5f));
    }

    SECTION("Construct from integer (implicit cast)")
    {
        // Use explicit float to avoid ambiguity with bool constructor
        GameValue value(100.0f);

        REQUIRE(value.GetType() == GameScalar);
        REQUIRE((float)value == Catch::Approx(100.0f));
    }

    SECTION("Construct from zero")
    {
        GameValue value(0.0f);

        REQUIRE(value.GetType() == GameScalar);
        REQUIRE((float)value == 0.0f);
    }

    SECTION("Construct from negative")
    {
        GameValue value(-123.456f);

        REQUIRE((float)value == Catch::Approx(-123.456f));
    }
}

TEST_CASE("GameValue - Construct from bool", "[evaluator][value][construct]")
{
    SECTION("Construct from true")
    {
        GameValue value(true);

        REQUIRE(value.GetType() == GameBool);
        REQUIRE((bool)value == true);
    }

    SECTION("Construct from false")
    {
        GameValue value(false);

        REQUIRE(value.GetType() == GameBool);
        REQUIRE((bool)value == false);
    }
}

TEST_CASE("GameValue - Construct from string", "[evaluator][value][construct]")
{
    SECTION("Construct from const char*")
    {
        GameValue value("Hello World");

        REQUIRE(value.GetType() == GameString);
        REQUIRE(std::string(((GameStringType)value).Data()) == "Hello World");
    }

    SECTION("Construct from RString")
    {
        RString str("Test String");
        GameValue value(str);

        REQUIRE(value.GetType() == GameString);
        REQUIRE(std::string(((GameStringType)value).Data()) == "Test String");
    }

    SECTION("Construct from empty string")
    {
        GameValue value("");

        REQUIRE(value.GetType() == GameString);
    }
}

TEST_CASE("GameValue - Construct from array", "[evaluator][value][construct]")
{
    SECTION("Construct from empty array")
    {
        GameArrayType arr;
        GameValue value(arr);

        REQUIRE(value.GetType() == GameArray);
        REQUIRE(((const GameArrayType&)value).Size() == 0);
    }

    SECTION("Construct from populated array")
    {
        GameArrayType arr;
        arr.Add(GameValue(1.0f));
        arr.Add(GameValue(2.0f));
        arr.Add(GameValue(3.0f));

        GameValue value(arr);

        REQUIRE(value.GetType() == GameArray);
        REQUIRE(((const GameArrayType&)value).Size() == 3);
    }

    SECTION("Construct from mixed-type array")
    {
        GameArrayType arr;
        arr.Add(GameValue(10.0f));  // Scalar
        arr.Add(GameValue(true));   // Bool
        arr.Add(GameValue("text")); // String

        GameValue value(arr);

        REQUIRE(((const GameArrayType&)value).Size() == 3);
    }
}

TEST_CASE("GameValue - Default construction", "[evaluator][value][construct]")
{
    SECTION("Default constructor creates valid value")
    {
        GameValue value;

        // Default GameValue should have some type
        GameType type = value.GetType();
        REQUIRE((type == GameScalar || type == GameBool || type == GameString || type == GameArray ||
                 type == GameNothing || type == GameAny));
    }
}

TEST_CASE("GameValue - Construct from GameData pointer", "[evaluator][value][construct]")
{
    SECTION("Construct from GameDataScalar pointer")
    {
        GameDataScalar* data = new GameDataScalar(99.9f);
        GameValue value(data);

        REQUIRE(value.GetType() == GameScalar);
        REQUIRE((float)value == Catch::Approx(99.9f));
    }

    SECTION("Construct from GameDataString pointer")
    {
        GameDataString* data = new GameDataString("Pointer String");
        GameValue value(data);

        REQUIRE(value.GetType() == GameString);
    }
}

// ============================================================================
// Test 2: Assignment Operators
// ============================================================================

TEST_CASE("GameValue - Assign scalar", "[evaluator][value][assign]")
{
    SECTION("Assign float to GameValue")
    {
        GameValue value;
        value = 123.45f;

        REQUIRE(value.GetType() == GameScalar);
        REQUIRE((float)value == Catch::Approx(123.45f));
    }

    SECTION("Reassign different scalar")
    {
        GameValue value(10.0f);
        value = 20.0f;

        REQUIRE((float)value == Catch::Approx(20.0f));
    }
}

TEST_CASE("GameValue - Assign bool", "[evaluator][value][assign]")
{
    SECTION("Assign true")
    {
        GameValue value;
        value = true;

        REQUIRE(value.GetType() == GameBool);
        REQUIRE((bool)value == true);
    }

    SECTION("Assign false")
    {
        GameValue value;
        value = false;

        REQUIRE((bool)value == false);
    }
}

TEST_CASE("GameValue - Assign string", "[evaluator][value][assign]")
{
    SECTION("Assign const char*")
    {
        GameValue value;
        value = "Assigned String";

        REQUIRE(value.GetType() == GameString);
        REQUIRE(std::string(((GameStringType)value).Data()) == "Assigned String");
    }

    SECTION("Assign RString")
    {
        GameValue value;
        RString str("RString Assignment");
        value = str;

        REQUIRE(std::string(((GameStringType)value).Data()) == "RString Assignment");
    }
}

TEST_CASE("GameValue - Assign array", "[evaluator][value][assign]")
{
    SECTION("Assign array")
    {
        GameValue value;

        GameArrayType arr;
        arr.Add(GameValue(5.0f));
        arr.Add(GameValue(6.0f));

        value = arr;

        REQUIRE(value.GetType() == GameArray);
        REQUIRE(((const GameArrayType&)value).Size() == 2);
    }
}

// ============================================================================
// Test 3: Type Queries
// ============================================================================

TEST_CASE("GameValue - GetType for each type", "[evaluator][value][type]")
{
    SECTION("Scalar type")
    {
        GameValue value(42.0f);
        REQUIRE(value.GetType() == GameScalar);
    }

    SECTION("Bool type")
    {
        GameValue value(true);
        REQUIRE(value.GetType() == GameBool);
    }

    SECTION("String type")
    {
        GameValue value("text");
        REQUIRE(value.GetType() == GameString);
    }

    SECTION("Array type")
    {
        GameArrayType arr;
        GameValue value(arr);
        REQUIRE(value.GetType() == GameArray);
    }
}

TEST_CASE("GameValue - GetNil detection", "[evaluator][value][type]")
{
    SECTION("Non-nil value")
    {
        GameValue value(10.0f);
        REQUIRE(value.GetNil() == false);
    }

    SECTION("Nil value from GameDataNil")
    {
        GameDataNil* nilData = new GameDataNil(GameScalar);
        GameValue value(nilData);

        REQUIRE(value.GetNil() == true);
    }
}

// ============================================================================
// Test 4: Implicit Type Conversions
// ============================================================================

TEST_CASE("GameValue - Implicit cast to scalar", "[evaluator][value][convert]")
{
    SECTION("Convert scalar to float")
    {
        GameValue value(3.14159f);
        float result = value;

        REQUIRE(result == Catch::Approx(3.14159f));
    }

    SECTION("Convert bool to scalar (true = 1.0)")
    {
        GameValue value(true);
        float result = value;

        REQUIRE(result == Catch::Approx(1.0f));
    }

    SECTION("Convert bool to scalar (false = 0.0)")
    {
        GameValue value(false);
        float result = value;

        REQUIRE(result == 0.0f);
    }
}

TEST_CASE("GameValue - Implicit cast to bool", "[evaluator][value][convert]")
{
    SECTION("Convert true")
    {
        GameValue value(true);
        bool result = value;

        REQUIRE(result == true);
    }

    SECTION("Convert false")
    {
        GameValue value(false);
        bool result = value;

        REQUIRE(result == false);
    }

    SECTION("Convert non-bool returns default false")
    {
        GameValue value(42.0f);
        bool result = value;

        // GameDataScalar::GetBool() returns false by default
        REQUIRE(result == false);
    }
}

TEST_CASE("GameValue - Implicit cast to string", "[evaluator][value][convert]")
{
    SECTION("Convert string value")
    {
        GameValue value("Test");
        GameStringType result = value;

        REQUIRE(std::string(result.Data()) == "Test");
    }

    SECTION("Convert non-string returns empty")
    {
        GameValue value(42.0f);
        GameStringType result = value;

        // GameDataScalar::GetString() returns empty string
        REQUIRE(result.Data() != nullptr);
    }
}

TEST_CASE("GameValue - Implicit cast to array", "[evaluator][value][convert]")
{
    SECTION("Convert array value (const)")
    {
        GameArrayType arr;
        arr.Add(GameValue(1.0f));
        arr.Add(GameValue(2.0f));

        GameValue value(arr);
        const GameArrayType& result = value;

        REQUIRE(result.Size() == 2);
    }

    SECTION("Convert array value (mutable)")
    {
        GameArrayType arr;
        GameValue value(arr);

        GameArrayType& result = value;
        result.Add(GameValue(99.0f));

        // Should modify the GameValue's array
        REQUIRE(((const GameArrayType&)value).Size() == 1);
    }
}

// ============================================================================
// Test 5: Copy Constructor and Assignment
// ============================================================================

TEST_CASE("GameValue - Copy constructor", "[evaluator][value][copy]")
{
    SECTION("Copy scalar value")
    {
        GameValue original(123.45f);
        GameValue copy(original);

        REQUIRE(copy.GetType() == GameScalar);
        REQUIRE((float)copy == Catch::Approx(123.45f));
    }

    SECTION("Copy bool value")
    {
        GameValue original(true);
        GameValue copy(original);

        REQUIRE((bool)copy == true);
    }

    SECTION("Copy string value")
    {
        GameValue original("Original Text");
        GameValue copy(original);

        REQUIRE(std::string(((GameStringType)copy).Data()) == "Original Text");
    }

    SECTION("Copy array value")
    {
        GameArrayType arr;
        arr.Add(GameValue(10.0f));
        GameValue original(arr);

        GameValue copy(original);

        REQUIRE(((const GameArrayType&)copy).Size() == 1);
    }
}

TEST_CASE("GameValue - Copy assignment operator", "[evaluator][value][copy]")
{
    SECTION("Assign scalar to scalar")
    {
        GameValue original(50.0f);
        GameValue target(100.0f);

        target = original;

        REQUIRE((float)target == Catch::Approx(50.0f));
    }

    SECTION("Assign different types")
    {
        GameValue original("String Value");
        GameValue target(42.0f); // Start as scalar

        target = original;

        REQUIRE(target.GetType() == GameString);
        REQUIRE(std::string(((GameStringType)target).Data()) == "String Value");
    }
}

// ============================================================================
// Test 6: Duplicate vs Copy Semantics
// ============================================================================

TEST_CASE("GameValue - Duplicate method", "[evaluator][value][copy]")
{
    SECTION("Duplicate creates deep copy")
    {
        GameValue original(100.0f);
        GameValue copy;

        copy.Duplicate(original);

        REQUIRE(copy.GetType() == GameScalar);
        REQUIRE((float)copy == Catch::Approx(100.0f));
    }

    SECTION("Duplicate array creates independent copy")
    {
        GameArrayType arr;
        arr.Add(GameValue(1.0f));
        GameValue original(arr);

        GameValue copy;
        copy.Duplicate(original);

        // Modify copy's array
        GameArrayType& copyArr = copy;
        copyArr.Add(GameValue(2.0f));

        // Original should be unchanged
        REQUIRE(((const GameArrayType&)original).Size() == 1);
        REQUIRE(((const GameArrayType&)copy).Size() == 2);
    }
}

TEST_CASE("GameValue - SharedInstance detection", "[evaluator][value][copy]")
{
    SECTION("Copy shares instance")
    {
        GameValue original(42.0f);
        GameValue copy(original);

        // After copy, both should share the same GameData
        REQUIRE(original.SharedInstance() == true);
        REQUIRE(copy.SharedInstance() == true);
    }

    SECTION("Duplicate does not share instance")
    {
        GameValue original(42.0f);
        GameValue copy;
        copy.Duplicate(original);

        // After duplicate, each has its own GameData
        REQUIRE(original.SharedInstance() == false);
        REQUIRE(copy.SharedInstance() == false);
    }
}

// ============================================================================
// Test 7: Read-Only Flag
// ============================================================================

TEST_CASE("GameValue - Read-only flag", "[evaluator][value][readonly]")
{
    SECTION("Default not read-only")
    {
        GameValue value(10.0f);

        REQUIRE(value.GetReadOnly() == false);
    }

    SECTION("Set read-only on array (only arrays support it)")
    {
        GameArrayType arr;
        arr.Add(GameValue(1.0f));
        GameValue value(arr);

        value.SetReadOnly(true);

        REQUIRE(value.GetReadOnly() == true);
    }

    SECTION("Clear read-only on array")
    {
        GameArrayType arr;
        arr.Add(GameValue(1.0f));
        GameValue value(arr);

        value.SetReadOnly(true);
        value.SetReadOnly(false);

        REQUIRE(value.GetReadOnly() == false);
    }

    SECTION("Scalar types don't support read-only")
    {
        GameValue value(10.0f);
        value.SetReadOnly(true);

        // Scalars don't implement read-only, so it stays false
        REQUIRE(value.GetReadOnly() == false);
    }
}

// ============================================================================
// Test 8: Text Representation
// ============================================================================

TEST_CASE("GameValue - GetText method", "[evaluator][value][text]")
{
    SECTION("Scalar text")
    {
        GameValue value(123.45f);
        RString text = value.GetText();

        REQUIRE(text.Data() != nullptr);
    }

    SECTION("Bool text")
    {
        GameValue value(true);
        RString text = value.GetText();

        REQUIRE(text.Data() != nullptr);
    }

    SECTION("String text (with quotes)")
    {
        GameValue value("TestString");
        RString text = value.GetText();

        // GetText adds quotes around strings
        REQUIRE(std::string(text.Data()) == "\"TestString\"");
    }

    SECTION("Array text")
    {
        GameArrayType arr;
        arr.Add(GameValue(1.0f));
        arr.Add(GameValue(2.0f));
        GameValue value(arr);

        RString text = value.GetText();

        REQUIRE(text.Data() != nullptr);
    }
}

TEST_CASE("GameValue - GetDebugText method", "[evaluator][value][text]")
{
    SECTION("Debug text for scalar")
    {
        GameValue value(42.0f);
        RString text = value.GetDebugText();

        REQUIRE(text.Data() != nullptr);
    }

    SECTION("Debug text for string")
    {
        GameValue value("Debug");
        RString text = value.GetDebugText();

        REQUIRE(text.Data() != nullptr);
    }
}

// ============================================================================
// Test 9: Equality Comparison
// ============================================================================

TEST_CASE("GameValue - IsEqualTo method", "[evaluator][value][compare]")
{
    SECTION("Equal scalars")
    {
        GameValue value1(10.0f);
        GameValue value2(10.0f);

        REQUIRE(value1.IsEqualTo(value2) == true);
    }

    SECTION("Different scalars")
    {
        GameValue value1(10.0f);
        GameValue value2(20.0f);

        REQUIRE(value1.IsEqualTo(value2) == false);
    }

    SECTION("Equal bools")
    {
        GameValue value1(true);
        GameValue value2(true);

        REQUIRE(value1.IsEqualTo(value2) == true);
    }

    SECTION("Different bools")
    {
        GameValue value1(true);
        GameValue value2(false);

        REQUIRE(value1.IsEqualTo(value2) == false);
    }

    SECTION("Equal strings")
    {
        GameValue value1("same");
        GameValue value2("same");

        REQUIRE(value1.IsEqualTo(value2) == true);
    }

    SECTION("Different strings")
    {
        GameValue value1("first");
        GameValue value2("second");

        REQUIRE(value1.IsEqualTo(value2) == false);
    }

    SECTION("Different types")
    {
        GameValue value1(42.0f); // Scalar
        GameValue value2("42");  // String

        REQUIRE(value1.IsEqualTo(value2) == false);
    }
}

// ============================================================================
// Test 10: Edge Cases
// ============================================================================

TEST_CASE("GameValue - Edge cases", "[evaluator][value][edge]")
{
    SECTION("Very long string")
    {
        std::string longStr(1000, 'X');
        GameValue value(longStr.c_str());

        REQUIRE(value.GetType() == GameString);
    }

    SECTION("Nested arrays")
    {
        GameArrayType inner;
        inner.Add(GameValue(1.0f));

        GameArrayType outer;
        outer.Add(GameValue(inner));

        GameValue value(outer);

        REQUIRE(((const GameArrayType&)value).Size() == 1);
    }

    SECTION("Empty string vs null")
    {
        GameValue empty("");
        GameValue str("text");

        REQUIRE(empty.IsEqualTo(str) == false);
    }

    SECTION("Zero vs false")
    {
        GameValue zero(0.0f);
        GameValue falseVal(false);

        // Different types
        REQUIRE(zero.GetType() == GameScalar);
        REQUIRE(falseVal.GetType() == GameBool);

        // Not equal (different types)
        REQUIRE(zero.IsEqualTo(falseVal) == false);
    }
}

// ============================================================================
// Test 11: GetData method
// ============================================================================

TEST_CASE("GameValue - GetData method", "[evaluator][value][data]")
{
    SECTION("GetData returns underlying GameData pointer")
    {
        GameValue value(100.0f);
        GameData* data = value.GetData();

        REQUIRE(data != nullptr);
        REQUIRE(data->GetType() == GameScalar);
        REQUIRE(data->GetScalar() == Catch::Approx(100.0f));
    }

    SECTION("GetData for string")
    {
        GameValue value("Test");
        GameData* data = value.GetData();

        REQUIRE(data != nullptr);
        REQUIRE(data->GetType() == GameString);
    }

    SECTION("GetData for array")
    {
        GameArrayType arr;
        arr.Add(GameValue(42.0f));
        GameValue value(arr);

        GameData* data = value.GetData();

        REQUIRE(data != nullptr);
        REQUIRE(data->GetType() == GameArray);
    }
}
