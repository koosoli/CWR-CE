#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Network/XML/Xml.hpp>
#include <Poseidon/IO/Streams/QBStream.hpp>
#include "../../test_fixtures.hpp"

#include <vector>
#include <string.h>
#include <Poseidon/Foundation/Strings/RString.hpp>

// XML Module Testing - SAX Parser
// Tests for core SAX parsing capabilities

using namespace TestFixtures;

TEST_CASE("SAXParser - Parse empty document", "[xml][parser][basic]")
{
    class TestParser : public SAXParser
    {
      public:
        bool startCalled = false;
        bool endCalled = false;

        void OnStartDocument() override { startCalled = true; }
        void OnEndDocument() override { endCalled = true; }
    };

    TestParser parser;
    const char* xml = "";
    QIStream in(xml, 0);

    bool result = parser.Parse(in);

    REQUIRE(result == true);
    REQUIRE(parser.startCalled == true);
    REQUIRE(parser.endCalled == true);
}

TEST_CASE("SAXParser - Parse simple element", "[xml][parser][basic]")
{
    class TestParser : public SAXParser
    {
      public:
        RString elementName;
        int startCount = 0;
        int endCount = 0;

        void OnStartElement(RString name, XMLAttributes& attrs) override
        {
            (void)attrs;
            elementName = name;
            startCount++;
        }

        void OnEndElement(RString name) override
        {
            (void)name;
            endCount++;
        }
    };

    TestParser parser;
    const char* xml = "<root></root>";
    QIStream in(xml, strlen(xml));

    bool result = parser.Parse(in);

    REQUIRE(result == true);
    REQUIRE(strcmp(parser.elementName.Data(), "root") == 0);
    REQUIRE(parser.startCount == 1);
    REQUIRE(parser.endCount == 1);
}

TEST_CASE("SAXParser - Parse element with attributes", "[xml][parser][attributes]")
{
    class TestParser : public SAXParser
    {
      public:
        RString attrId;
        RString attrNick;
        int attrCount = 0;

        void OnStartElement(RString name, XMLAttributes& attrs) override
        {
            (void)name;
            attrCount = attrs.Size();

            const XMLAttribute* id = attrs.Find("id");
            if (id)
            {
                attrId = id->value;
            }

            const XMLAttribute* nick = attrs.Find("nick");
            if (nick)
            {
                attrNick = nick->value;
            }
        }
    };

    TestParser parser;
    const char* xml = "<member id=\"12345\" nick=\"Player\"></member>";
    QIStream in(xml, strlen(xml));

    bool result = parser.Parse(in);

    REQUIRE(result == true);
    REQUIRE(parser.attrCount == 2);
    REQUIRE(strcmp(parser.attrId.Data(), "12345") == 0);
    REQUIRE(strcmp(parser.attrNick.Data(), "Player") == 0);
}

TEST_CASE("SAXParser - Parse nested elements", "[xml][parser][nested]")
{
    class TestParser : public SAXParser
    {
      public:
        std::vector<RString> elements;

        void OnStartElement(RString name, XMLAttributes& attrs) override
        {
            (void)attrs;
            elements.push_back(name);
        }
    };

    TestParser parser;
    const char* xml = "<squad><name></name><member></member></squad>";
    QIStream in(xml, strlen(xml));

    bool result = parser.Parse(in);

    REQUIRE(result == true);
    REQUIRE(parser.elements.size() == 3);
    REQUIRE(strcmp(parser.elements[0].Data(), "squad") == 0);
    REQUIRE(strcmp(parser.elements[1].Data(), "name") == 0);
    REQUIRE(strcmp(parser.elements[2].Data(), "member") == 0);
}

TEST_CASE("SAXParser - Parse element text content", "[xml][parser][text]")
{
    class TestParser : public SAXParser
    {
      public:
        RString text;

        void OnCharacters(RString chars) override { text = chars; }
    };

    TestParser parser;
    const char* xml = "<name>Squad Name</name>";
    QIStream in(xml, strlen(xml));

    bool result = parser.Parse(in);

    REQUIRE(result == true);
    REQUIRE(strcmp(parser.text.Data(), "Squad Name") == 0);
}

TEST_CASE("SAXParser - Parse self-closing tag", "[xml][parser][selfclosing]")
{
    class TestParser : public SAXParser
    {
      public:
        int startCount = 0;
        int endCount = 0;
        RString elementName;

        void OnStartElement(RString name, XMLAttributes& attrs) override
        {
            (void)attrs;
            elementName = name;
            startCount++;
        }

        void OnEndElement(RString name) override
        {
            (void)name;
            endCount++;
        }
    };

    TestParser parser;
    const char* xml = "<element/>";
    QIStream in(xml, strlen(xml));

    bool result = parser.Parse(in);

    REQUIRE(result == true);
    REQUIRE(strcmp(parser.elementName.Data(), "element") == 0);
    REQUIRE(parser.startCount == 1);
    REQUIRE(parser.endCount == 1);
}

TEST_CASE("SAXParser - Parse multiple attributes", "[xml][parser][attributes]")
{
    class TestParser : public SAXParser
    {
      public:
        int attrCount = 0;
        RString id, nick, rank;

        void OnStartElement(RString name, XMLAttributes& attrs) override
        {
            (void)name;
            attrCount = attrs.Size();

            const XMLAttribute* attr;
            if ((attr = attrs.Find("id")))
            {
                id = attr->value;
            }
            if ((attr = attrs.Find("nick")))
            {
                nick = attr->value;
            }
            if ((attr = attrs.Find("rank")))
            {
                rank = attr->value;
            }
        }
    };

    TestParser parser;
    const char* xml = "<member id=\"123\" nick=\"Player\" rank=\"Sergeant\"/>";
    QIStream in(xml, strlen(xml));

    bool result = parser.Parse(in);

    REQUIRE(result == true);
    REQUIRE(parser.attrCount == 3);
    REQUIRE(strcmp(parser.id.Data(), "123") == 0);
    REQUIRE(strcmp(parser.nick.Data(), "Player") == 0);
    REQUIRE(strcmp(parser.rank.Data(), "Sergeant") == 0);
}

TEST_CASE("SAXParser - Abort parsing midway", "[xml][parser][abort]")
{
    class TestParser : public SAXParser
    {
      public:
        int elementCount = 0;

        void OnStartElement(RString name, XMLAttributes& attrs) override
        {
            (void)name;
            (void)attrs;
            elementCount++;
            if (elementCount >= 2)
            {
                Abort();
            }
        }
    };

    TestParser parser;
    const char* xml = "<root><a/><b/><c/></root>";
    QIStream in(xml, strlen(xml));

    (void)parser.Parse(in);

    // Parser should stop after second element
    REQUIRE(parser.elementCount == 2);
}
