#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Network/XML/Xml.hpp>
#include "../../test_fixtures.hpp"
#include <string.h>
#include <Poseidon/Foundation/Strings/RString.hpp>

// XML Module Testing - Attributes
// Tests for XMLAttribute and XMLAttributes data structures

using namespace TestFixtures;

TEST_CASE("XMLAttribute - Basic creation and assignment", "[xml][attribute]")
{
    SECTION("Create and assign attribute")
    {
        XMLAttribute attr;
        attr.name = "id";
        attr.value = "12345";

        REQUIRE(strcmp(attr.name.Data(), "id") == 0);
        REQUIRE(strcmp(attr.value.Data(), "12345") == 0);
    }

    SECTION("Empty attribute")
    {
        XMLAttribute attr;
        attr.name = "";
        attr.value = "";

        REQUIRE(attr.name.GetLength() == 0);
        REQUIRE(attr.value.GetLength() == 0);
    }

    SECTION("Attribute with special characters")
    {
        XMLAttribute attr;
        attr.name = "data-value";
        attr.value = "test value with spaces";

        REQUIRE(strcmp(attr.name.Data(), "data-value") == 0);
        REQUIRE(strcmp(attr.value.Data(), "test value with spaces") == 0);
    }
}

TEST_CASE("XMLAttributes - Add attributes", "[xml][attributes]")
{
    SECTION("Add single attribute")
    {
        XMLAttributes attrs;

        int idx = attrs.Add("name", "value");

        REQUIRE(idx == 0);
        REQUIRE(attrs.Size() == 1);
        REQUIRE(strcmp(attrs[0].name.Data(), "name") == 0);
        REQUIRE(strcmp(attrs[0].value.Data(), "value") == 0);
    }

    SECTION("Add multiple attributes")
    {
        XMLAttributes attrs;

        attrs.Add("id", "123");
        attrs.Add("nick", "Player");
        attrs.Add("rank", "Sergeant");

        REQUIRE(attrs.Size() == 3);
        REQUIRE(strcmp(attrs[0].name.Data(), "id") == 0);
        REQUIRE(strcmp(attrs[1].name.Data(), "nick") == 0);
        REQUIRE(strcmp(attrs[2].name.Data(), "rank") == 0);
    }
}

TEST_CASE("XMLAttributes - Find existing attribute", "[xml][attributes]")
{
    SECTION("Find first attribute")
    {
        XMLAttributes attrs;
        attrs.Add("id", "123");
        attrs.Add("nick", "Player");
        attrs.Add("rank", "Sergeant");

        const XMLAttribute* found = attrs.Find("id");

        REQUIRE(found != nullptr);
        REQUIRE(strcmp(found->name.Data(), "id") == 0);
        REQUIRE(strcmp(found->value.Data(), "123") == 0);
    }

    SECTION("Find middle attribute")
    {
        XMLAttributes attrs;
        attrs.Add("id", "123");
        attrs.Add("nick", "Player");
        attrs.Add("rank", "Sergeant");

        const XMLAttribute* found = attrs.Find("nick");

        REQUIRE(found != nullptr);
        REQUIRE(strcmp(found->name.Data(), "nick") == 0);
        REQUIRE(strcmp(found->value.Data(), "Player") == 0);
    }

    SECTION("Find last attribute")
    {
        XMLAttributes attrs;
        attrs.Add("id", "123");
        attrs.Add("nick", "Player");
        attrs.Add("rank", "Sergeant");

        const XMLAttribute* found = attrs.Find("rank");

        REQUIRE(found != nullptr);
        REQUIRE(strcmp(found->name.Data(), "rank") == 0);
        REQUIRE(strcmp(found->value.Data(), "Sergeant") == 0);
    }
}

TEST_CASE("XMLAttributes - Find non-existent attribute", "[xml][attributes]")
{
    SECTION("Empty attributes")
    {
        XMLAttributes attrs;

        const XMLAttribute* found = attrs.Find("missing");

        REQUIRE(found == nullptr);
    }

    SECTION("Non-existent in populated list")
    {
        XMLAttributes attrs;
        attrs.Add("id", "123");
        attrs.Add("nick", "Player");

        const XMLAttribute* found = attrs.Find("missing");

        REQUIRE(found == nullptr);
    }
}

TEST_CASE("XMLAttributes - Multiple attributes with same name", "[xml][attributes]")
{
    SECTION("Add duplicate names")
    {
        XMLAttributes attrs;
        attrs.Add("class", "first");
        attrs.Add("class", "second");
        attrs.Add("class", "third");

        REQUIRE(attrs.Size() == 3);

        // Find returns first match
        const XMLAttribute* found = attrs.Find("class");
        REQUIRE(found != nullptr);
        REQUIRE(strcmp(found->value.Data(), "first") == 0);
    }
}
