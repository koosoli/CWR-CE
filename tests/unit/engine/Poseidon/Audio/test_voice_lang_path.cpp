#include <Poseidon/Audio/VoiceLangPath.hpp>
#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Foundation/Strings/RString.hpp>

using namespace Poseidon;
TEST_CASE("WithLangSuffix inserts before extension", "[audio][voice-lang]")
{
    REQUIRE(WithLangSuffix("missions/Demo/sound/s02v01.ogg", "Czech") ==
            RString("missions/Demo/sound/s02v01.Czech.ogg"));
    REQUIRE(WithLangSuffix("s02v01.ogg", "English") == RString("s02v01.English.ogg"));
    REQUIRE(WithLangSuffix("a/b\\c.ogg", "Czech") == RString("a/b\\c.Czech.ogg"));
}

TEST_CASE("WithLangSuffix splits on last dot only", "[audio][voice-lang]")
{
    REQUIRE(WithLangSuffix("sound/foo.tar.gz", "Czech") == RString("sound/foo.tar.Czech.gz"));
}

TEST_CASE("WithLangSuffix returns empty for invalid input", "[audio][voice-lang]")
{
    REQUIRE(WithLangSuffix("sound/s02v01.ogg", "").GetLength() == 0);
    REQUIRE(WithLangSuffix("", "Czech").GetLength() == 0);
    REQUIRE(WithLangSuffix("sound/noext", "Czech").GetLength() == 0);
    REQUIRE(WithLangSuffix("sound.dir/noextfile", "Czech").GetLength() == 0);
}

TEST_CASE("WithoutLangSuffix strips matching segment", "[audio][voice-lang]")
{
    REQUIRE(WithoutLangSuffix("missions/Demo/sound/s02v01.Czech.lip", "Czech") ==
            RString("missions/Demo/sound/s02v01.lip"));
    REQUIRE(WithoutLangSuffix("s02v01.English.ogg", "English") == RString("s02v01.ogg"));
    // Case-insensitive language match
    REQUIRE(WithoutLangSuffix("s02v01.czech.ogg", "Czech") == RString("s02v01.ogg"));
}

TEST_CASE("WithoutLangSuffix returns empty when nothing to strip", "[audio][voice-lang]")
{
    REQUIRE(WithoutLangSuffix("s02v01.ogg", "Czech").GetLength() == 0);
    REQUIRE(WithoutLangSuffix("s02v01.German.ogg", "Czech").GetLength() == 0);
    REQUIRE(WithoutLangSuffix("noext", "Czech").GetLength() == 0);
    REQUIRE(WithoutLangSuffix("sound.ogg", "").GetLength() == 0);
    REQUIRE(WithoutLangSuffix("", "Czech").GetLength() == 0);
}

TEST_CASE("WithLangSuffix and WithoutLangSuffix round-trip", "[audio][voice-lang]")
{
    const RString original = "missions/Demo/sound/s02v01.ogg";
    const RString suffixed = WithLangSuffix(original, "Czech");
    REQUIRE(suffixed == RString("missions/Demo/sound/s02v01.Czech.ogg"));
    REQUIRE(WithoutLangSuffix(suffixed, "Czech") == original);
}
