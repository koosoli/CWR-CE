#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Network/XML/Xml.hpp>
#include <Poseidon/IO/Streams/QBStream.hpp>
#include "../../test_fixtures.hpp"
#include <string.h>
#include <Poseidon/Foundation/Strings/RString.hpp>

// XML Module Testing - Squad.xml Real-World Usage
// Tests for parsing real squad.xml files used in multiplayer

using namespace TestFixtures;

TEST_CASE("SAXParser - Parse valid squad.xml", "[xml][squad][fixtures]")
{
    REQUIRE_FIXTURE("squad_valid.xml");

    class SquadParser : public SAXParser
    {
      public:
        RString squadNick;
        int memberCount = 0;

        // Store first member details
        RString firstMemberId;
        RString firstMemberNick;

        void OnStartElement(RString name, XMLAttributes& attrs) override
        {
            if (strcmp(name.Data(), "squad") == 0)
            {
                const XMLAttribute* nick = attrs.Find("nick");
                if (nick)
                {
                    squadNick = nick->value;
                }
            }
            else if (strcmp(name.Data(), "member") == 0)
            {
                memberCount++;

                // Capture first member
                if (memberCount == 1)
                {
                    const XMLAttribute* id = attrs.Find("id");
                    const XMLAttribute* nick = attrs.Find("nick");
                    if (id)
                    {
                        firstMemberId = id->value;
                    }
                    if (nick)
                    {
                        firstMemberNick = nick->value;
                    }
                }
            }
        }
    };

    SquadParser parser;
    const char* path = GetTestFixturePath("squad_valid.xml");
    QIFStream in;
    in.open(path);

    bool result = parser.Parse(in);

    REQUIRE(result == true);
    REQUIRE(strcmp(parser.squadNick.Data(), "SQTAG") == 0);
    REQUIRE(parser.memberCount == 2);
    REQUIRE(strcmp(parser.firstMemberId.Data(), "12345678") == 0);
    REQUIRE(strcmp(parser.firstMemberNick.Data(), "Player1") == 0);
}

TEST_CASE("SAXParser - Parse squad with multiple members", "[xml][squad][fixtures]")
{
    REQUIRE_FIXTURE("squad_multi_members.xml");

    class SquadParser : public SAXParser
    {
      public:
        int memberCount = 0;
        RString lastMemberNick;

        void OnStartElement(RString name, XMLAttributes& attrs) override
        {
            if (strcmp(name.Data(), "member") == 0)
            {
                memberCount++;

                const XMLAttribute* nick = attrs.Find("nick");
                if (nick)
                {
                    lastMemberNick = nick->value;
                }
            }
        }
    };

    SquadParser parser;
    const char* path = GetTestFixturePath("squad_multi_members.xml");
    QIFStream in;
    in.open(path);

    bool result = parser.Parse(in);

    REQUIRE(result == true);
    REQUIRE(parser.memberCount == 5);
    REQUIRE(strcmp(parser.lastMemberNick.Data(), "Echo") == 0);
}

TEST_CASE("SAXParser - Parse squad with special characters", "[xml][squad][fixtures]")
{
    REQUIRE_FIXTURE("squad_special_chars.xml");

    class SquadParser : public SAXParser
    {
      public:
        RString squadNick;
        RString memberNick;

        void OnStartElement(RString name, XMLAttributes& attrs) override
        {
            if (strcmp(name.Data(), "squad") == 0)
            {
                const XMLAttribute* nick = attrs.Find("nick");
                if (nick)
                {
                    squadNick = nick->value;
                }
            }
            else if (strcmp(name.Data(), "member") == 0)
            {
                const XMLAttribute* nick = attrs.Find("nick");
                if (nick)
                {
                    memberNick = nick->value;
                }
            }
        }
    };

    SquadParser parser;
    const char* path = GetTestFixturePath("squad_special_chars.xml");
    QIFStream in;
    in.open(path);

    bool result = parser.Parse(in);

    REQUIRE(result == true);
    // Should handle entities properly
    REQUIRE(parser.squadNick.GetLength() > 0);
    REQUIRE(parser.memberNick.GetLength() > 0);
}

TEST_CASE("SAXParser - Parse minimal squad.xml", "[xml][squad][fixtures]")
{
    REQUIRE_FIXTURE("squad_minimal.xml");

    class SquadParser : public SAXParser
    {
      public:
        bool hasSquad = false;
        RString squadNick;
        int memberCount = 0;

        void OnStartElement(RString name, XMLAttributes& attrs) override
        {
            if (strcmp(name.Data(), "squad") == 0)
            {
                hasSquad = true;
                const XMLAttribute* nick = attrs.Find("nick");
                if (nick)
                {
                    squadNick = nick->value;
                }
            }
            else if (strcmp(name.Data(), "member") == 0)
            {
                memberCount++;
            }
        }
    };

    SquadParser parser;
    const char* path = GetTestFixturePath("squad_minimal.xml");
    QIFStream in;
    in.open(path);

    bool result = parser.Parse(in);

    REQUIRE(result == true);
    REQUIRE(parser.hasSquad == true);
    REQUIRE(parser.squadNick.GetLength() > 0);
    REQUIRE(parser.memberCount >= 1);
}

TEST_CASE("SAXParser - Parse incomplete squad.xml", "[xml][squad][fixtures]")
{
    REQUIRE_FIXTURE("squad_incomplete.xml");

    class SquadParser : public SAXParser
    {
      public:
        bool hasSquad = false;
        int memberCount = 0;

        void OnStartElement(RString name, XMLAttributes& attrs) override
        {
            (void)attrs;
            if (strcmp(name.Data(), "squad") == 0)
            {
                hasSquad = true;
            }
            else if (strcmp(name.Data(), "member") == 0)
            {
                memberCount++;
            }
        }
    };

    SquadParser parser;
    const char* path = GetTestFixturePath("squad_incomplete.xml");
    QIFStream in;
    in.open(path);

    bool result = parser.Parse(in);

    // Parser should be lenient - parse what it can
    REQUIRE(result == true);
    REQUIRE(parser.hasSquad == true);
}
