#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <Poseidon/Asset/Probes/AssetInfo.hpp>
#include <Poseidon/Asset/Probes/AssetPreview.hpp>
#include <Poseidon/Graphics/Rendering/Draw/FontData.hpp>
#include "test_fixtures.hpp"
#include <fstream>
#include <stddef.h>
#include <string>
#include <utility>
#include <vector>

using namespace Poseidon;

TEST_CASE("InspectFont returns valid info for legacy", "[tools][font]")
{
    const char* path = GET_FIXTURE("font/legacy.fxy");
    REQUIRE(path != nullptr);

    FontInfo info = InspectFont(path);
    REQUIRE(info.valid);
    CHECK(info.name == "legacy");
    CHECK(info.glyphCount == 224);
    CHECK(info.maxHeight == 3);
    CHECK(info.maxWidth == 3);
    CHECK(info.textureSetCount == 1);
    CHECK(info.textureNames.size() == 1);
    CHECK(info.charCodeMin >= 0);
    CHECK(info.charCodeMax > info.charCodeMin);
}

TEST_CASE("InspectFont checks texture existence", "[tools][font]")
{
    const char* path = GET_FIXTURE("font/legacy.fxy");
    REQUIRE(path != nullptr);

    FontInfo info = InspectFont(path);
    REQUIRE(info.valid);
    REQUIRE(info.texturesExist.size() == info.textureNames.size());
    // Fixture directory has the PAA files, so they should be found
    for (size_t i = 0; i < info.texturesExist.size(); i++)
        CHECK(info.texturesExist[i]);
}

TEST_CASE("InspectFont uses original case for texture names", "[tools][font]")
{
    // The stem of the fixture path preserves case from the filesystem
    const char* path = GET_FIXTURE("font/legacy.fxy");
    REQUIRE(path != nullptr);

    FontInfo info = InspectFont(path);
    REQUIRE(info.valid);
    REQUIRE(!info.textureNames.empty());
    // Texture name should use stem case from the file path
    CHECK(info.textureNames[0].find("-01.paa") != std::string::npos);
}

TEST_CASE("InspectFont returns invalid for missing file", "[tools][font]")
{
    FontInfo info = InspectFont("/nonexistent/font.fxy");
    CHECK_FALSE(info.valid);
}

TEST_CASE("InspectFont char range covers non-empty glyphs", "[tools][font]")
{
    const char* path = GET_FIXTURE("font/legacy.fxy");
    REQUIRE(path != nullptr);

    FontInfo info = InspectFont(path);
    REQUIRE(info.valid);
    CHECK(info.charCodeMin >= 0);
    CHECK(info.charCodeMax <= 223);
    CHECK(info.charCodeMax >= info.charCodeMin);
}

// ============================================================================
// PreviewFont (from file)
// ============================================================================

TEST_CASE("PreviewFont returns valid RGBA for legacy", "[tools-preview][font]")
{
    const char* path = GET_FIXTURE("font/legacy.fxy");
    REQUIRE(path != nullptr);

    auto preview = PreviewFont(path);
    REQUIRE(preview.valid());
    CHECK(preview.width > 0);
    CHECK(preview.height > 0);
    CHECK(preview.channels == 4);
    CHECK(preview.data.size() == static_cast<size_t>(preview.width * preview.height * 4));
}

TEST_CASE("PreviewFont charmap mode renders all characters", "[tools-preview][font]")
{
    const char* path = GET_FIXTURE("font/legacy.fxy");
    REQUIRE(path != nullptr);

    FontPreviewOptions defaultOpts;
    auto defaultPreview = PreviewFont(path, defaultOpts);

    FontPreviewOptions charmapOpts;
    charmapOpts.charmap = true;
    auto charmapPreview = PreviewFont(path, charmapOpts);

    REQUIRE(defaultPreview.valid());
    REQUIRE(charmapPreview.valid());
    // Charmap should be larger (more characters)
    CHECK(charmapPreview.width * charmapPreview.height > defaultPreview.width * defaultPreview.height);
}

TEST_CASE("PreviewFont custom text renders differently", "[tools-preview][font]")
{
    const char* path = GET_FIXTURE("font/legacy.fxy");
    REQUIRE(path != nullptr);

    FontPreviewOptions opts1;
    opts1.sampleText = "A";
    auto preview1 = PreviewFont(path, opts1);

    FontPreviewOptions opts2;
    opts2.sampleText = "ABCDEFGHIJKLMNOP";
    auto preview2 = PreviewFont(path, opts2);

    REQUIRE(preview1.valid());
    REQUIRE(preview2.valid());
    CHECK(preview2.width > preview1.width);
}

TEST_CASE("PreviewFont returns invalid for missing file", "[tools-preview][font]")
{
    auto preview = PreviewFont("/nonexistent/font.fxy");
    CHECK_FALSE(preview.valid());
}

// ============================================================================
// PreviewFontFromData (in-memory)
// ============================================================================

TEST_CASE("PreviewFontFromData renders from memory buffers", "[tools-preview][font]")
{
    // Load FXY file into memory
    const char* fxyPath = GET_FIXTURE("font/legacy.fxy");
    REQUIRE(fxyPath != nullptr);

    std::ifstream fxyFile(fxyPath, std::ios::binary | std::ios::ate);
    REQUIRE(fxyFile.good());
    auto fxySize = fxyFile.tellg();
    fxyFile.seekg(0);
    std::vector<char> fxyBuf(static_cast<size_t>(fxySize));
    fxyFile.read(fxyBuf.data(), fxySize);

    // Load PAA texture into memory
    const char* paaPath = GET_FIXTURE("font/legacy-01.paa");
    REQUIRE(paaPath != nullptr);

    std::ifstream paaFile(paaPath, std::ios::binary | std::ios::ate);
    REQUIRE(paaFile.good());
    auto paaSize = paaFile.tellg();
    paaFile.seekg(0);
    std::vector<char> paaBuf(static_cast<size_t>(paaSize));
    paaFile.read(paaBuf.data(), paaSize);

    std::vector<std::pair<int, std::pair<const void*, size_t>>> texSets;
    texSets.push_back({1, {paaBuf.data(), paaBuf.size()}});

    auto preview = PreviewFontFromData(fxyBuf.data(), fxyBuf.size(), "legacy", texSets);
    REQUIRE(preview.valid());
    CHECK(preview.width > 0);
    CHECK(preview.height > 0);
    CHECK(preview.data.size() == static_cast<size_t>(preview.width * preview.height * 4));
}

TEST_CASE("PreviewFontFromData matches file-based PreviewFont", "[tools-preview][font]")
{
    // File-based
    const char* fxyPath = GET_FIXTURE("font/legacy.fxy");
    REQUIRE(fxyPath != nullptr);
    auto filePreview = PreviewFont(fxyPath);
    REQUIRE(filePreview.valid());

    // Memory-based
    std::ifstream fxyFile(fxyPath, std::ios::binary | std::ios::ate);
    auto fxySize = fxyFile.tellg();
    fxyFile.seekg(0);
    std::vector<char> fxyBuf(static_cast<size_t>(fxySize));
    fxyFile.read(fxyBuf.data(), fxySize);

    const char* paaPath = GET_FIXTURE("font/legacy-01.paa");
    std::ifstream paaFile(paaPath, std::ios::binary | std::ios::ate);
    auto paaSize = paaFile.tellg();
    paaFile.seekg(0);
    std::vector<char> paaBuf(static_cast<size_t>(paaSize));
    paaFile.read(paaBuf.data(), paaSize);

    std::vector<std::pair<int, std::pair<const void*, size_t>>> texSets;
    texSets.push_back({1, {paaBuf.data(), paaBuf.size()}});

    auto memPreview = PreviewFontFromData(fxyBuf.data(), fxyBuf.size(), "legacy", texSets);
    REQUIRE(memPreview.valid());
    // Same dimensions
    CHECK(memPreview.width == filePreview.width);
    CHECK(memPreview.height == filePreview.height);
}

TEST_CASE("PreviewFontFromData returns invalid with no textures", "[tools-preview][font]")
{
    const char* fxyPath = GET_FIXTURE("font/legacy.fxy");
    std::ifstream fxyFile(fxyPath, std::ios::binary | std::ios::ate);
    auto fxySize = fxyFile.tellg();
    fxyFile.seekg(0);
    std::vector<char> fxyBuf(static_cast<size_t>(fxySize));
    fxyFile.read(fxyBuf.data(), fxySize);

    std::vector<std::pair<int, std::pair<const void*, size_t>>> emptyTexSets;
    auto preview = PreviewFontFromData(fxyBuf.data(), fxyBuf.size(), "legacy", emptyTexSets);
    CHECK_FALSE(preview.valid());
}
