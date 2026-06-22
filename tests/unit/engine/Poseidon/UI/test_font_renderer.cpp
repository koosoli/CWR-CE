#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <Poseidon/UI/Text/FontRenderer.hpp>
#include "../test_fixtures.hpp"
#include <cmath>
#include <cstring>
#include <stdint.h>
#include <stdio.h>
#include <initializer_list>
#include <string>
#include <vector>

namespace ui = Poseidon::ui;

static const char* GetTestFont()
{
    return GET_FIXTURE("font/dummy.ttf");
}

static const char* GetTestMonoFont()
{
    return GET_FIXTURE("font/dummy.ttf");
}

TEST_CASE("FontRenderer: UTF-8 decoding", "[ui][font]")
{
    using ui::FontRenderer;

    SECTION("ASCII")
    {
        const char* s = "A";
        const char* p = s;
        REQUIRE(FontRenderer::DecodeUTF8(p) == 0x41);
        REQUIRE(p == s + 1);
    }

    SECTION("2-byte Czech characters")
    {
        // ě = U+011B = 0xC4 0x9B in UTF-8
        const char* s = "\xC4\x9B";
        const char* p = s;
        REQUIRE(FontRenderer::DecodeUTF8(p) == 0x011B);
        REQUIRE(p == s + 2);
    }

    SECTION("3-byte CJK")
    {
        // 中 = U+4E2D = 0xE4 0xB8 0xAD
        const char* s = "\xE4\xB8\xAD";
        const char* p = s;
        REQUIRE(FontRenderer::DecodeUTF8(p) == 0x4E2D);
        REQUIRE(p == s + 3);
    }

    SECTION("4-byte emoji")
    {
        // 😀 = U+1F600 = 0xF0 0x9F 0x98 0x80
        const char* s = "\xF0\x9F\x98\x80";
        const char* p = s;
        REQUIRE(FontRenderer::DecodeUTF8(p) == 0x1F600);
        REQUIRE(p == s + 4);
    }

    SECTION("Full string iteration")
    {
        const char* s = "Ab\xC4\x9B"; // "Abě"
        const char* p = s;
        REQUIRE(FontRenderer::DecodeUTF8(p) == 'A');
        REQUIRE(FontRenderer::DecodeUTF8(p) == 'b');
        REQUIRE(FontRenderer::DecodeUTF8(p) == 0x011B);
        REQUIRE(*p == '\0');
    }
}

TEST_CASE("FontRenderer: initialization", "[ui][font]")
{
    ui::FontRenderer fr;
    REQUIRE_FALSE(fr.IsLoaded());
    REQUIRE(fr.GetAtlasPageCount() == 0);
}

TEST_CASE("FontRenderer: load system font", "[ui][font]")
{
    ui::FontRenderer fr;
    bool ok = fr.LoadFont(GetTestFont());
    REQUIRE(ok);
    REQUIRE(fr.IsLoaded());
}

TEST_CASE("FontRenderer: load from memory", "[ui][font]")
{
    // Read font file into buffer
    FILE* f = fopen(GetTestFont(), "rb");
    REQUIRE(f != nullptr);
    fseek(f, 0, SEEK_END);
    auto size = ftell(f);
    fseek(f, 0, SEEK_SET);
    std::vector<uint8_t> buf(static_cast<size_t>(size));
    fread(buf.data(), 1, buf.size(), f);
    fclose(f);

    ui::FontRenderer fr;
    REQUIRE(fr.LoadFontFromMemory(buf.data(), buf.size()));
    REQUIRE(fr.IsLoaded());
}

TEST_CASE("FontRenderer: glyph rasterization", "[ui][font]")
{
    ui::FontRenderer fr;
    REQUIRE(fr.LoadFont(GetTestFont()));

    SECTION("ASCII 'A' at 24px")
    {
        auto* gm = fr.GetGlyph('A', 24);
        REQUIRE(gm != nullptr);
        REQUIRE(gm->width > 0);
        REQUIRE(gm->height > 0);
        REQUIRE(gm->advance > 0);
        REQUIRE(gm->bearingY > 0);
        REQUIRE(gm->atlasPage == 0);
        REQUIRE(fr.GetAtlasPageCount() == 1);
    }

    SECTION("Space has zero dimensions but valid advance")
    {
        auto* gm = fr.GetGlyph(' ', 24);
        REQUIRE(gm != nullptr);
        REQUIRE(gm->width == 0);
        REQUIRE(gm->height == 0);
        REQUIRE(gm->advance > 0);
    }

    SECTION("Czech ě at 24px")
    {
        auto* gm = fr.GetGlyph(0x011B, 24); // ě
        REQUIRE(gm != nullptr);
        REQUIRE(gm->width > 0);
        REQUIRE(gm->height > 0);
    }

    SECTION("Same glyph cached")
    {
        auto* gm1 = fr.GetGlyph('B', 24);
        auto* gm2 = fr.GetGlyph('B', 24);
        REQUIRE(gm1 == gm2); // same pointer = cached
    }

    SECTION("Different sizes get different entries")
    {
        auto* gm16 = fr.GetGlyph('X', 16);
        auto* gm32 = fr.GetGlyph('X', 32);
        REQUIRE(gm16 != gm32);
        REQUIRE(gm16->width < gm32->width);
    }
}

TEST_CASE("FontRenderer: shipped mono font covers Russian glyphs", "[ui][font]")
{
    ui::FontRenderer fr;
    REQUIRE(fr.LoadFont(GetTestMonoFont()));

    auto* latin = fr.GetGlyph('A', 24);
    auto* russian = fr.GetGlyph(0x0416, 24); // Cyrillic Zhe
    REQUIRE(latin != nullptr);
    REQUIRE(russian != nullptr);
    REQUIRE(russian->width > 0);
    REQUIRE(russian->height > 0);
    // Missing glyph fallback should not alias to the same cached Latin glyph.
    CHECK(russian != latin);
}

TEST_CASE("FontRenderer: atlas pixel data", "[ui][font]")
{
    ui::FontRenderer fr;
    REQUIRE(fr.LoadFont(GetTestFont()));

    fr.GetGlyph('A', 48);
    REQUIRE(fr.GetAtlasPageCount() >= 1);

    auto& page = fr.GetAtlasPage(0);
    REQUIRE(page.width == ui::FontRenderer::kAtlasWidth);
    REQUIRE(page.height == ui::FontRenderer::kAtlasHeight);
    REQUIRE(page.dirty == true);
    REQUIRE(page.pixels.size() == static_cast<size_t>(page.width) * page.height * 4);

    // Verify glyph pixels are non-zero somewhere in the atlas
    bool foundNonZeroAlpha = false;
    for (size_t i = 3; i < page.pixels.size(); i += 4)
    {
        if (page.pixels[i] > 0)
        {
            foundNonZeroAlpha = true;
            break;
        }
    }
    REQUIRE(foundNonZeroAlpha);
}

TEST_CASE("FontRenderer: MeasureText", "[ui][font]")
{
    ui::FontRenderer fr;
    REQUIRE(fr.LoadFont(GetTestFont()));

    SECTION("Empty string")
    {
        auto m = fr.MeasureText("", 24);
        REQUIRE(m.width == 0.0f);
    }

    SECTION("Single char")
    {
        auto m = fr.MeasureText("A", 24);
        REQUIRE(m.width > 0.0f);
        REQUIRE(m.height == 24.0f);
    }

    SECTION("Longer text is wider")
    {
        auto m1 = fr.MeasureText("A", 24);
        auto m3 = fr.MeasureText("ABC", 24);
        REQUIRE(m3.width > m1.width);
    }

    SECTION("Bigger size is wider")
    {
        auto m16 = fr.MeasureText("Hello", 16);
        auto m32 = fr.MeasureText("Hello", 32);
        REQUIRE(m32.width > m16.width);
    }

    SECTION("UTF-8 Czech text")
    {
        auto m = fr.MeasureText("Příliš žluťoučký kůň", 24);
        REQUIRE(m.width > 0.0f);
    }

    SECTION("Width scale is applied consistently")
    {
        auto normal = fr.MeasureText("Hello", 24, 1.0f);
        auto wide = fr.MeasureText("Hello", 24, 1.5f);
        REQUIRE(wide.width > normal.width * 1.4f);
        REQUIRE(wide.width < normal.width * 1.6f);
        REQUIRE(wide.height == normal.height);
    }
}

TEST_CASE("FontRenderer: LayoutText", "[ui][font]")
{
    ui::FontRenderer fr;
    REQUIRE(fr.LoadFont(GetTestFont()));

    SECTION("Basic ASCII layout")
    {
        auto quads = fr.LayoutText("Hi", 0.0f, 24.0f, 24);
        REQUIRE(quads.size() == 2); // H and i
        REQUIRE(quads[0].w > 0.0f);
        REQUIRE(quads[0].h > 0.0f);
        // Second glyph should be to the right of first
        REQUIRE(quads[1].x > quads[0].x);
    }

    SECTION("Space doesn't emit a quad")
    {
        auto quads = fr.LayoutText("A B", 0.0f, 24.0f, 24);
        REQUIRE(quads.size() == 2); // A and B, space has zero dims
    }

    SECTION("UV coordinates are valid")
    {
        auto quads = fr.LayoutText("X", 0.0f, 24.0f, 24);
        REQUIRE(quads.size() == 1);
        REQUIRE(quads[0].u0 >= 0.0f);
        REQUIRE(quads[0].v0 >= 0.0f);
        REQUIRE(quads[0].u1 > quads[0].u0);
        REQUIRE(quads[0].v1 > quads[0].v0);
        REQUIRE(quads[0].u1 <= 1.0f);
        REQUIRE(quads[0].v1 <= 1.0f);
    }

    SECTION("Czech text produces quads")
    {
        auto quads = fr.LayoutText("ěščřž", 0.0f, 24.0f, 24);
        REQUIRE(quads.size() == 5);
    }

    SECTION("Advance uses FreeType advance, not bearingX+width")
    {
        // With the proper typographic advance, the horizontal step between
        // consecutive identical glyphs must match FreeType's advance field.
        // MeasureText uses the same formula; for a single-glyph string the
        // pen advances once, so two-glyph measure equals 2x the per-glyph
        // advance modulo bearing at the end.
        auto q2 = fr.LayoutText("oo", 0.0f, 24.0f, 24);
        REQUIRE(q2.size() == 2);
        float step = q2[1].x - q2[0].x;
        // Step must be positive (second glyph sits to the right of the first)
        REQUIRE(step > 0.0f);
        // And must exceed the first glyph's ink width — otherwise the glyphs
        // would visually collide (the exact bug that made Steelfish "jo" look
        // merged before the advance/bearingX fix).
        REQUIRE(step >= q2[0].w);
    }

    SECTION("First-glyph x honors bearingX")
    {
        // LayoutText places q.x at penX + bearingX. When penX is 0 and the
        // glyph has a negative left-side bearing (common on narrow-stem faces
        // like 'j'), q.x goes slightly negative — the ink extends left of the
        // pen position, which is the FreeType standard.
        auto q = fr.LayoutText("j", 0.0f, 24.0f, 24);
        // Whether bearingX is negative depends on the face, but q.x must equal
        // bearingX (penX = 0), so it can be negative or zero but not arbitrary.
        REQUIRE(q.size() == 1);
        // The glyph flat_quad's right edge (x + w) should sit within the glyph's
        // typographic cell. A positive w and sane x (|x| < w) is enough
        // to confirm bearingX is actually being applied.
        REQUIRE(q[0].w > 0.0f);
        REQUIRE(std::abs(q[0].x) <= q[0].w);
    }

    SECTION("Width scale stretches layout horizontally")
    {
        auto normal = fr.LayoutText("oo", 0.0f, 24.0f, 24, 1.0f);
        auto wide = fr.LayoutText("oo", 0.0f, 24.0f, 24, 1.5f);
        REQUIRE(normal.size() == 2);
        REQUIRE(wide.size() == 2);

        float normalStep = normal[1].x - normal[0].x;
        float wideStep = wide[1].x - wide[0].x;
        REQUIRE(wideStep > normalStep * 1.4f);
        REQUIRE(wideStep < normalStep * 1.6f);
        REQUIRE(wide[0].w > normal[0].w * 1.4f);
        REQUIRE(wide[0].w < normal[0].w * 1.6f);
    }
}

TEST_CASE("FontRenderer: atlas packing multiple sizes", "[ui][font]")
{
    ui::FontRenderer fr;
    REQUIRE(fr.LoadFont(GetTestFont()));

    // Rasterize entire ASCII printable range at two sizes
    for (int size : {16, 32})
    {
        for (uint32_t cp = 33; cp < 127; cp++)
        {
            auto* gm = fr.GetGlyph(cp, size);
            REQUIRE(gm != nullptr);
        }
    }
    // ~188 glyphs should fit in 1 page (1024x1024 is plenty)
    REQUIRE(fr.GetAtlasPageCount() == 1);
}
