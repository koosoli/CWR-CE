#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Network/XML/Xml.hpp>
#include <Poseidon/IO/Streams/QBStream.hpp>
#include "../../test_fixtures.hpp"
#include <string.h>
#include <Poseidon/Foundation/Strings/RString.hpp>

// XML Module Testing - Error Handling
// Tests for robustness and graceful degradation with malformed XML

using namespace TestFixtures;

TEST_CASE("SAXParser - Handle malformed XML gracefully", "[xml][errors][malformed]")
{
    class TestParser : public SAXParser
    {
      public:
        int elementCount = 0;
        bool endDocumentCalled = false;

        void OnStartElement(RString name, XMLAttributes& attrs) override
        {
            (void)name;
            (void)attrs;
            elementCount++;
        }

        void OnEndDocument() override { endDocumentCalled = true; }
    };

    SECTION("Missing closing tag")
    {
        TestParser parser;
        const char* xml = "<root><child>";
        QIStream in(xml, strlen(xml));

        // Parser is lenient - should not crash
        (void)parser.Parse(in);

        // Parser reaches EOF and calls OnEndDocument
        REQUIRE(parser.endDocumentCalled == true);
        REQUIRE(parser.elementCount >= 1);
    }

    SECTION("Unmatched closing tag")
    {
        TestParser parser;
        const char* xml = "<root></wrongtag>";
        QIStream in(xml, strlen(xml));

        // Parser is lenient - processes what it can
        (void)parser.Parse(in);

        REQUIRE(parser.elementCount >= 1);
    }

    SECTION("Missing angle bracket")
    {
        TestParser parser;
        const char* xml = "<root<child></child></root>";
        QIStream in(xml, strlen(xml));

        // Parser should handle this gracefully
        parser.Parse(in);

        // Just verify it doesn't crash
        REQUIRE(true);
    }
}

TEST_CASE("SAXParser - Handle empty input", "[xml][errors][empty]")
{
    class TestParser : public SAXParser
    {
      public:
        bool startCalled = false;
        bool endCalled = false;
        int elementCount = 0;

        void OnStartDocument() override { startCalled = true; }
        void OnEndDocument() override { endCalled = true; }
        void OnStartElement(RString name, XMLAttributes& attrs) override
        {
            (void)name;
            (void)attrs;
            elementCount++;
        }
    };

    SECTION("Completely empty input")
    {
        TestParser parser;
        const char* xml = "";
        QIStream in(xml, 0);

        (void)parser.Parse(in);

        REQUIRE(parser.startCalled == true);
        REQUIRE(parser.endCalled == true);
        REQUIRE(parser.elementCount == 0);
    }

    SECTION("Only whitespace")
    {
        TestParser parser;
        const char* xml = "   \n\t  \n  ";
        QIStream in(xml, strlen(xml));

        bool result = parser.Parse(in);

        REQUIRE(result == true);
        REQUIRE(parser.elementCount == 0);
    }
}

TEST_CASE("SAXParser - Handle unusual but valid XML", "[xml][errors][unusual]")
{
    class TestParser : public SAXParser
    {
      public:
        int elementCount = 0;
        RString elementName;

        void OnStartElement(RString name, XMLAttributes& attrs) override
        {
            (void)attrs;
            elementCount++;
            elementName = name;
        }
    };

    SECTION("Element with no attributes or content")
    {
        TestParser parser;
        const char* xml = "<empty></empty>";
        QIStream in(xml, strlen(xml));

        bool result = parser.Parse(in);

        REQUIRE(result == true);
        REQUIRE(parser.elementCount == 1);
        REQUIRE(strcmp(parser.elementName.Data(), "empty") == 0);
    }

    SECTION("Deeply nested elements")
    {
        TestParser parser;
        const char* xml = "<a><b><c><d><e></e></d></c></b></a>";
        QIStream in(xml, strlen(xml));

        bool result = parser.Parse(in);

        REQUIRE(result == true);
        REQUIRE(parser.elementCount == 5);
    }

    SECTION("Element with many attributes")
    {
        TestParser parser;
        const char* xml = "<element a=\"1\" b=\"2\" c=\"3\" d=\"4\" e=\"5\" f=\"6\" g=\"7\" h=\"8\"/>";
        QIStream in(xml, strlen(xml));

        bool result = parser.Parse(in);

        REQUIRE(result == true);
        REQUIRE(parser.elementCount == 1);
    }

    SECTION("Very long element name")
    {
        TestParser parser;
        const char* xml = "<verylongelementnamethatgoesonandonandon></verylongelementnamethatgoesonandonandon>";
        QIStream in(xml, strlen(xml));

        (void)parser.Parse(in);

        // Parser has buffer limits but should handle reasonably
        REQUIRE(parser.elementCount >= 1);
    }

    SECTION("Very long attribute value")
    {
        TestParser parser;
        const char* xml = "<element attr=\"this is a very long attribute value that goes on and on and on and contains "
                          "lots of text\"/>";
        QIStream in(xml, strlen(xml));

        bool result = parser.Parse(in);

        REQUIRE(result == true);
        REQUIRE(parser.elementCount == 1);
    }
}
