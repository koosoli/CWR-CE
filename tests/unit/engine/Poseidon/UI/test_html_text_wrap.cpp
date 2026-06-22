#include <catch2/catch_test_macros.hpp>
#include <Poseidon/UI/Controls/HtmlTextWrap.hpp>
#include <vector>
#include <Poseidon/Foundation/Strings/RString.hpp>

using namespace Poseidon;

TEST_CASE("HtmlTextWrap: UTF-8 breakpoints stay on whole Cyrillic characters", "[ui][html][wrap]")
{
    const RString text = "яяя яяя";
    const auto breaks = HtmlTextWrap::ComputeWrapBreaks(
        text, 3.5f, [](RString ch) { return HtmlTextWrap::IsWrapWhitespace(ch, ch.GetLength()) ? 0.5f : 1.0f; });

    REQUIRE(breaks.size() == 1);
    CHECK(breaks[0] == 7);
    CHECK(text.Substring(0, breaks[0]) == RString("яяя "));
    CHECK(text.Substring(breaks[0], text.GetLength()) == RString("яяя"));
}

TEST_CASE("HtmlTextWrap: UTF-8 helper reports Russian letters as two-byte spans", "[ui][html][wrap]")
{
    const char* text = "я";
    REQUIRE(HtmlTextWrap::Utf8CharBytes(text, 2) == 2);
    REQUIRE(HtmlTextWrap::Utf8CharBytes(text, 1) == 1);
}
