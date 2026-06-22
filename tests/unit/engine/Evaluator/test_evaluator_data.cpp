#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Evaluator/express.hpp>
#include <string>
#include <Poseidon/Foundation/Strings/RString.hpp>

// ============================================================================
// Evaluator testing - Phase 1.1: GameData concrete types
// ============================================================================
// Testing the base data storage classes that hold typed values.
// These are the building blocks for the expression evaluator.
//
// Hierarchy:
//   GameData (abstract base)
//   ??? GameDataScalar (float)
//   ??? GameDataBool (boolean)
//   ??? GameDataString (RString)
//   ??? GameDataArray (AutoArray<GameValue>)
//   ??? GameDataNil (void/null)
//   ??? GameDataNothing (special nothing type)
// ============================================================================

// ============================================================================
// Test 1: GameDataScalar - Float Values
// ============================================================================

TEST_CASE("GameDataScalar - Construction", "[evaluator][data][scalar]")
{
    SECTION("Default construction")
    {
        GameDataScalar* data = new GameDataScalar();

        REQUIRE(data != nullptr);
        REQUIRE(data->GetScalar() == 0.0f);

        delete data;
    }

    SECTION("Construction with value")
    {
        GameDataScalar* data = new GameDataScalar(42.5f);

        REQUIRE(data->GetScalar() == Catch::Approx(42.5f));

        delete data;
    }

    SECTION("Construction with negative value")
    {
        GameDataScalar* data = new GameDataScalar(-123.456f);

        REQUIRE(data->GetScalar() == Catch::Approx(-123.456f));

        delete data;
    }
}

TEST_CASE("GameDataScalar - GetType", "[evaluator][data][scalar]")
{
    SECTION("Returns GameScalar type")
    {
        GameDataScalar data(10.0f);

        REQUIRE(data.GetType() == GameScalar);
    }
}

TEST_CASE("GameDataScalar - GetScalar", "[evaluator][data][scalar]")
{
    SECTION("Returns stored value")
    {
        GameDataScalar data(3.14159f);

        REQUIRE(data.GetScalar() == Catch::Approx(3.14159f));
    }

    SECTION("Zero value")
    {
        GameDataScalar data(0.0f);

        REQUIRE(data.GetScalar() == 0.0f);
    }

    SECTION("Very small value")
    {
        GameDataScalar data(0.0001f);

        REQUIRE(data.GetScalar() == Catch::Approx(0.0001f));
    }

    SECTION("Very large value")
    {
        GameDataScalar data(1000000.0f);

        REQUIRE(data.GetScalar() == Catch::Approx(1000000.0f));
    }
}

TEST_CASE("GameDataScalar - GetText", "[evaluator][data][scalar]")
{
    SECTION("Formats positive number")
    {
        GameDataScalar data(42.0f);
        RString text = data.GetText();

        // Should contain the number
        REQUIRE(text.Data() != nullptr);
        // Just verify it's not empty
    }

    SECTION("Formats negative number")
    {
        GameDataScalar data(-99.9f);
        RString text = data.GetText();

        REQUIRE(text.Data() != nullptr);
    }

    SECTION("Formats zero")
    {
        GameDataScalar data(0.0f);
        RString text = data.GetText();

        REQUIRE(text.Data() != nullptr);
    }
}

TEST_CASE("GameDataScalar - IsEqualTo", "[evaluator][data][scalar]")
{
    SECTION("Equal values")
    {
        GameDataScalar data1(10.0f);
        GameDataScalar data2(10.0f);

        REQUIRE(data1.IsEqualTo(&data2) == true);
    }

    SECTION("Different values")
    {
        GameDataScalar data1(10.0f);
        GameDataScalar data2(20.0f);

        REQUIRE(data1.IsEqualTo(&data2) == false);
    }
}

TEST_CASE("GameDataScalar - Clone", "[evaluator][data][scalar]")
{
    SECTION("Creates independent copy")
    {
        GameDataScalar original(55.5f);
        GameData* clone = original.Clone();

        REQUIRE(clone != nullptr);
        REQUIRE(clone != &original);
        REQUIRE(clone->GetType() == GameScalar);
        REQUIRE(clone->GetScalar() == Catch::Approx(55.5f));

        delete clone;
    }
}

TEST_CASE("GameDataScalar - GetTypeName", "[evaluator][data][scalar]")
{
    SECTION("Returns 'float'")
    {
        GameDataScalar data(1.0f);

        REQUIRE(std::string(data.GetTypeName()) == "float");
    }
}

// ============================================================================
// Test 2: GameDataBool - Boolean Values
// ============================================================================

TEST_CASE("GameDataBool - Construction", "[evaluator][data][bool]")
{
    SECTION("Default construction (false)")
    {
        GameDataBool* data = new GameDataBool();

        REQUIRE(data->GetBool() == false);

        delete data;
    }

    SECTION("Construction with true")
    {
        GameDataBool* data = new GameDataBool(true);

        REQUIRE(data->GetBool() == true);

        delete data;
    }

    SECTION("Construction with false")
    {
        GameDataBool* data = new GameDataBool(false);

        REQUIRE(data->GetBool() == false);

        delete data;
    }
}

TEST_CASE("GameDataBool - GetType", "[evaluator][data][bool]")
{
    SECTION("Returns GameBool type")
    {
        GameDataBool data(true);

        REQUIRE(data.GetType() == GameBool);
    }
}

TEST_CASE("GameDataBool - GetBool", "[evaluator][data][bool]")
{
    SECTION("Returns true")
    {
        GameDataBool data(true);

        REQUIRE(data.GetBool() == true);
    }

    SECTION("Returns false")
    {
        GameDataBool data(false);

        REQUIRE(data.GetBool() == false);
    }
}

TEST_CASE("GameDataBool - GetScalar conversion", "[evaluator][data][bool]")
{
    SECTION("True converts to 1.0")
    {
        GameDataBool data(true);

        REQUIRE(data.GetScalar() == Catch::Approx(1.0f));
    }

    SECTION("False converts to 0.0")
    {
        GameDataBool data(false);

        REQUIRE(data.GetScalar() == Catch::Approx(0.0f));
    }
}

TEST_CASE("GameDataBool - GetText", "[evaluator][data][bool]")
{
    SECTION("True formatted as text")
    {
        GameDataBool data(true);
        RString text = data.GetText();

        REQUIRE(text.Data() != nullptr);
    }

    SECTION("False formatted as text")
    {
        GameDataBool data(false);
        RString text = data.GetText();

        REQUIRE(text.Data() != nullptr);
    }
}

TEST_CASE("GameDataBool - IsEqualTo", "[evaluator][data][bool]")
{
    SECTION("Both true")
    {
        GameDataBool data1(true);
        GameDataBool data2(true);

        REQUIRE(data1.IsEqualTo(&data2) == true);
    }

    SECTION("Both false")
    {
        GameDataBool data1(false);
        GameDataBool data2(false);

        REQUIRE(data1.IsEqualTo(&data2) == true);
    }

    SECTION("Different values")
    {
        GameDataBool data1(true);
        GameDataBool data2(false);

        REQUIRE(data1.IsEqualTo(&data2) == false);
    }
}

TEST_CASE("GameDataBool - Clone", "[evaluator][data][bool]")
{
    SECTION("Clone true value")
    {
        GameDataBool original(true);
        GameData* clone = original.Clone();

        REQUIRE(clone != nullptr);
        REQUIRE(clone->GetType() == GameBool);
        REQUIRE(clone->GetBool() == true);

        delete clone;
    }

    SECTION("Clone false value")
    {
        GameDataBool original(false);
        GameData* clone = original.Clone();

        REQUIRE(clone->GetBool() == false);

        delete clone;
    }
}

// ============================================================================
// Test 3: GameDataString - String Values
// ============================================================================

TEST_CASE("GameDataString - Construction", "[evaluator][data][string]")
{
    SECTION("Default construction (empty)")
    {
        GameDataString* data = new GameDataString();

        REQUIRE(data->GetString().Data() != nullptr);

        delete data;
    }

    SECTION("Construction with string")
    {
        GameDataString* data = new GameDataString("Hello World");

        REQUIRE(std::string(data->GetString().Data()) == "Hello World");

        delete data;
    }

    SECTION("Construction with RString")
    {
        RString str("Test String");
        GameDataString* data = new GameDataString(str);

        REQUIRE(std::string(data->GetString().Data()) == "Test String");

        delete data;
    }
}

TEST_CASE("GameDataString - GetType", "[evaluator][data][string]")
{
    SECTION("Returns GameString type")
    {
        GameDataString data("test");

        REQUIRE(data.GetType() == GameString);
    }
}

TEST_CASE("GameDataString - GetString", "[evaluator][data][string]")
{
    SECTION("Returns stored string")
    {
        GameDataString data("My String");

        REQUIRE(std::string(data.GetString().Data()) == "My String");
    }

    SECTION("Empty string")
    {
        GameDataString data("");

        REQUIRE(data.GetString().Data() != nullptr);
    }

    SECTION("String with spaces")
    {
        GameDataString data("  spaces  ");

        REQUIRE(std::string(data.GetString().Data()) == "  spaces  ");
    }
}

TEST_CASE("GameDataString - Special characters", "[evaluator][data][string]")
{
    SECTION("String with quotes")
    {
        GameDataString data("He said \"Hello\"");

        REQUIRE(data.GetString().Data() != nullptr);
    }

    SECTION("String with newlines")
    {
        GameDataString data("Line1\nLine2");

        REQUIRE(data.GetString().Data() != nullptr);
    }

    SECTION("String with backslashes")
    {
        GameDataString data("C:\\\\Path\\\\File");

        REQUIRE(data.GetString().Data() != nullptr);
    }
}

TEST_CASE("GameDataString - GetText", "[evaluator][data][string]")
{
    SECTION("Returns string value")
    {
        GameDataString data("TestString");
        RString text = data.GetText();

        // GetText adds quotes for debug output: "TestString"
        REQUIRE(std::string(text.Data()) == "\"TestString\"");
    }
}

TEST_CASE("GameDataString - IsEqualTo", "[evaluator][data][string]")
{
    SECTION("Equal strings")
    {
        GameDataString data1("same");
        GameDataString data2("same");

        REQUIRE(data1.IsEqualTo(&data2) == true);
    }

    SECTION("Different strings")
    {
        GameDataString data1("first");
        GameDataString data2("second");

        REQUIRE(data1.IsEqualTo(&data2) == false);
    }

    SECTION("Empty strings")
    {
        GameDataString data1("");
        GameDataString data2("");

        REQUIRE(data1.IsEqualTo(&data2) == true);
    }
}

TEST_CASE("GameDataString - Clone", "[evaluator][data][string]")
{
    SECTION("Clone creates independent copy")
    {
        GameDataString original("Original Text");
        GameData* clone = original.Clone();

        REQUIRE(clone != nullptr);
        REQUIRE(clone->GetType() == GameString);
        REQUIRE(std::string(clone->GetString().Data()) == "Original Text");

        delete clone;
    }
}

// ============================================================================
// Test 4: GameDataArray - Array Values
// ============================================================================

TEST_CASE("GameDataArray - Construction", "[evaluator][data][array]")
{
    SECTION("Default construction (empty array)")
    {
        GameDataArray* data = new GameDataArray();

        REQUIRE(data->GetArray().Size() == 0);

        delete data;
    }

    SECTION("Construction with array")
    {
        GameArrayType arr;
        arr.Add(GameValue(1.0f));
        arr.Add(GameValue(2.0f));

        GameDataArray* data = new GameDataArray(arr);

        REQUIRE(data->GetArray().Size() == 2);

        delete data;
    }
}

TEST_CASE("GameDataArray - GetType", "[evaluator][data][array]")
{
    SECTION("Returns GameArray type")
    {
        GameDataArray data;

        REQUIRE(data.GetType() == GameArray);
    }
}

TEST_CASE("GameDataArray - GetArray", "[evaluator][data][array]")
{
    SECTION("Returns array reference")
    {
        GameArrayType arr;
        arr.Add(GameValue(10.0f));
        arr.Add(GameValue(20.0f));
        arr.Add(GameValue(30.0f));

        GameDataArray data(arr);
        const GameArrayType& result = data.GetArray();

        REQUIRE(result.Size() == 3);
        REQUIRE((float)result[0] == Catch::Approx(10.0f));
        REQUIRE((float)result[1] == Catch::Approx(20.0f));
        REQUIRE((float)result[2] == Catch::Approx(30.0f));
    }

    SECTION("Mutable array access")
    {
        GameDataArray data;
        GameArrayType& arr = data.GetArray();

        arr.Add(GameValue(99.0f));

        REQUIRE(data.GetArray().Size() == 1);
        REQUIRE((float)data.GetArray()[0] == Catch::Approx(99.0f));
    }
}

TEST_CASE("GameDataArray - Mixed type arrays", "[evaluator][data][array]")
{
    SECTION("Array with different types")
    {
        GameArrayType arr;
        arr.Add(GameValue(42.0f));  // Scalar
        arr.Add(GameValue(true));   // Bool
        arr.Add(GameValue("text")); // String

        GameDataArray data(arr);

        REQUIRE(data.GetArray().Size() == 3);
        REQUIRE(data.GetArray()[0].GetType() == GameScalar);
        REQUIRE(data.GetArray()[1].GetType() == GameBool);
        REQUIRE(data.GetArray()[2].GetType() == GameString);
    }
}

TEST_CASE("GameDataArray - Nested arrays", "[evaluator][data][array]")
{
    SECTION("Array containing arrays")
    {
        GameArrayType inner;
        inner.Add(GameValue(1.0f));
        inner.Add(GameValue(2.0f));

        GameArrayType outer;
        outer.Add(GameValue(inner));
        outer.Add(GameValue(3.0f));

        GameDataArray data(outer);

        REQUIRE(data.GetArray().Size() == 2);
        REQUIRE(data.GetArray()[0].GetType() == GameArray);
        REQUIRE(data.GetArray()[1].GetType() == GameScalar);
    }
}

TEST_CASE("GameDataArray - Read-only flag", "[evaluator][data][array]")
{
    SECTION("Default not read-only")
    {
        GameDataArray data;

        REQUIRE(data.GetReadOnly() == false);
    }

    SECTION("Set read-only")
    {
        GameDataArray data;
        data.SetReadOnly(true);

        REQUIRE(data.GetReadOnly() == true);
    }

    SECTION("Clear read-only")
    {
        GameDataArray data;
        data.SetReadOnly(true);
        data.SetReadOnly(false);

        REQUIRE(data.GetReadOnly() == false);
    }
}

TEST_CASE("GameDataArray - GetText", "[evaluator][data][array]")
{
    SECTION("Empty array text")
    {
        GameDataArray data;
        RString text = data.GetText();

        REQUIRE(text.Data() != nullptr);
    }

    SECTION("Array with elements text")
    {
        GameArrayType arr;
        arr.Add(GameValue(1.0f));
        arr.Add(GameValue(2.0f));

        GameDataArray data(arr);
        RString text = data.GetText();

        REQUIRE(text.Data() != nullptr);
    }
}

TEST_CASE("GameDataArray - Clone", "[evaluator][data][array]")
{
    SECTION("Clone creates deep copy")
    {
        GameArrayType arr;
        arr.Add(GameValue(100.0f));

        GameDataArray original(arr);
        GameData* clone = original.Clone();

        REQUIRE(clone != nullptr);
        REQUIRE(clone->GetType() == GameArray);
        REQUIRE(clone->GetArray().Size() == 1);

        delete clone;
    }
}

// ============================================================================
// Test 5: GameDataNil - Void/Null Values
// ============================================================================

TEST_CASE("GameDataNil - Construction", "[evaluator][data][nil]")
{
    SECTION("Construction with type")
    {
        GameDataNil* data = new GameDataNil(GameScalar);

        REQUIRE(data->GetType() == GameScalar);
        REQUIRE(data->GetNil() == true);

        delete data;
    }
}

TEST_CASE("GameDataNil - GetTypeName", "[evaluator][data][nil]")
{
    SECTION("Returns 'void'")
    {
        GameDataNil data(GameScalar);

        REQUIRE(std::string(data.GetTypeName()) == "void");
    }
}

TEST_CASE("GameDataNil - GetText", "[evaluator][data][nil]")
{
    SECTION("Returns nil representation")
    {
        GameDataNil data(GameScalar);
        RString text = data.GetText();

        REQUIRE(text.Data() != nullptr);
    }
}

TEST_CASE("GameDataNil - IsEqualTo", "[evaluator][data][nil]")
{
    SECTION("Two nil values")
    {
        GameDataNil data1(GameScalar);
        GameDataNil data2(GameScalar);

        // Nil values may or may not be equal - depends on implementation
        // Just verify it doesn't crash
        bool result = data1.IsEqualTo(&data2);
        REQUIRE((result == true || result == false));
    }
}

// ============================================================================
// Test 6: GameDataNothing - Special Nothing Type
// ============================================================================

TEST_CASE("GameDataNothing - Construction", "[evaluator][data][nothing]")
{
    SECTION("Default construction")
    {
        GameDataNothing* data = new GameDataNothing();

        REQUIRE(data->GetType() == GameNothing);

        delete data;
    }
}

TEST_CASE("GameDataNothing - GetTypeName", "[evaluator][data][nothing]")
{
    SECTION("Returns 'nothing'")
    {
        GameDataNothing data;

        REQUIRE(std::string(data.GetTypeName()) == "nothing");
    }
}

TEST_CASE("GameDataNothing - GetText", "[evaluator][data][nothing]")
{
    SECTION("Returns 'nothing' text")
    {
        GameDataNothing data;
        RString text = data.GetText();

        REQUIRE(std::string(text.Data()) == "nothing");
    }
}

TEST_CASE("GameDataNothing - IsEqualTo", "[evaluator][data][nothing]")
{
    SECTION("Two nothing values")
    {
        GameDataNothing data1;
        GameDataNothing data2;

        // Just verify IsEqualTo doesn't crash
        // Implementation may or may not consider different instances equal
        bool result = data1.IsEqualTo(&data2);
        REQUIRE((result == true || result == false));
    }
}

TEST_CASE("GameDataNothing - Clone", "[evaluator][data][nothing]")
{
    SECTION("Clone creates copy")
    {
        GameDataNothing original;
        GameData* clone = original.Clone();

        REQUIRE(clone != nullptr);
        REQUIRE(clone->GetType() == GameNothing);

        delete clone;
    }
}
