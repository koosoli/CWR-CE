#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <Poseidon/Graphics/Rendering/Draw/FontData.hpp>
#include <Poseidon/Graphics/Rendering/Draw/Font.hpp>
#include "test_fixtures.hpp"

#include <filesystem>
#include <string>
#include <system_error>

using Poseidon::HasFreeTypeFontMapping;

using Poseidon::BucketFTPixelSize;

// BucketFTPixelSize picks the atlas rasterization pixel size from an ideal
// (screen-matched) target. Rounds to the nearest multiple of 4 and clamps to
// [8, 160]. The draw path relies on this to keep the per-size glyph cache
// bounded (~20 buckets across the useful UI range) while staying close enough
// to on-screen pixels that atlas↔screen sampling is near-1:1.

TEST_CASE("BucketFTPixelSize rounds to nearest multiple of 4", "[font][bucket]")
{
    CHECK(BucketFTPixelSize(14.0f) == 16);
    CHECK(BucketFTPixelSize(15.9f) == 16);
    CHECK(BucketFTPixelSize(16.0f) == 16);
    CHECK(BucketFTPixelSize(17.9f) == 16);
    CHECK(BucketFTPixelSize(18.0f) == 20);
    CHECK(BucketFTPixelSize(22.0f) == 24);
    CHECK(BucketFTPixelSize(46.0f) == 48);
    CHECK(BucketFTPixelSize(82.5f) == 84);
}

TEST_CASE("BucketFTPixelSize clamps to [8, 160]", "[font][bucket]")
{
    CHECK(BucketFTPixelSize(0.0f) == 8);
    CHECK(BucketFTPixelSize(2.0f) == 8);
    CHECK(BucketFTPixelSize(7.9f) == 8);
    CHECK(BucketFTPixelSize(8.0f) == 8);

    CHECK(BucketFTPixelSize(160.0f) == 160);
    CHECK(BucketFTPixelSize(200.0f) == 160);
    CHECK(BucketFTPixelSize(9999.0f) == 160);
}

TEST_CASE("BucketFTPixelSize produces only multiples of 4 within bounds", "[font][bucket]")
{
    for (float ideal = 1.0f; ideal < 200.0f; ideal += 0.25f)
    {
        int bucket = BucketFTPixelSize(ideal);
        CHECK(bucket % 4 == 0);
        CHECK(bucket >= 8);
        CHECK(bucket <= 160);
    }
}

TEST_CASE("Common runtime fonts keep a FreeType mapping", "[font][bucket]")
{
    CHECK(HasFreeTypeFontMapping("cwrbody12"));
    CHECK(HasFreeTypeFontMapping("tahomab36"));
    CHECK(HasFreeTypeFontMapping("tahomab48"));
    CHECK(HasFreeTypeFontMapping("couriernewb64"));
    CHECK(HasFreeTypeFontMapping("audreyshand24"));
    CHECK_FALSE(HasFreeTypeFontMapping("nonexistentfont"));
}

TEST_CASE("Unmapped addon font names still load via FXY fallback", "[font][bucket]")
{
    namespace fs = std::filesystem;

    const fs::path sourceFxy = GET_FIXTURE("font/legacy.fxy");
    const fs::path sourcePaa = GET_FIXTURE("font/legacy-01.paa");
    const fs::path tempDir = fs::temp_directory_path() / fs::path("cwr-font-fallback-addon");
    const fs::path addonStem = tempDir / fs::path("addoncustom42");
    const fs::path addonFxy = addonStem;
    const fs::path addonPaa = tempDir / fs::path("addoncustom42-01.paa");
    const fs::path originalCwd = fs::current_path();

    std::error_code ec;
    fs::remove_all(tempDir, ec);
    fs::create_directories(tempDir);
    fs::copy_file(sourceFxy, addonFxy.string() + ".fxy", fs::copy_options::overwrite_existing);
    fs::copy_file(sourcePaa, addonPaa, fs::copy_options::overwrite_existing);

    REQUIRE_FALSE(HasFreeTypeFontMapping("addoncustom42"));

    fs::current_path(tempDir);
    Font font;
    font.Load("addoncustom42");
    fs::current_path(originalCwd);
    fs::remove_all(tempDir, ec);

    CHECK_FALSE(font.IsFreeType());
    CHECK(font.FTRenderer() == nullptr);
    CHECK(font.FTReferencePx() == 0);
    CHECK(font.MaxHeight() == 3);
    CHECK(font.Height() == Catch::Approx(3.0f / 600.0f));
}
