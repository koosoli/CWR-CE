#include <catch2/catch_test_macros.hpp>

#include <Poseidon/Graphics/Core/MipmapLayout.hpp>
#include <catch2/catch_message.hpp>
#include <initializer_list>

using Poseidon::PacARGB1555;
using Poseidon::PacARGB4444;
using Poseidon::PacARGB8888;
using Poseidon::PacDXT1;
using Poseidon::PacDXT5;
using Poseidon::PacFormat;
using Poseidon::PacRGB565;

using Poseidon::render::mipmap::ComputeLayout;

// I-15 / I-16: per-mip pitch + byte-size for texture uploads.
//
// B-021 was D3D11 staging-texture RowPitch mismatch (source data
// packed tighter than destination row stride).  B-016 was
// `glCompressedTexSubImage2D` called with the base-level format
// for every mip - lower mips need stricter alignment.  Both bug
// classes are now captured by a single per-mip layout function
// that the upload path calls with `(format, mip._w, mip._h)`.
//
// These tests pin: (a) the block-row math for DXT formats,
// (b) the minimum-pitch / minimum-row clamps that handle 1x1
// and 2x2 mips at the tail of a chain, (c) per-mip independence
// (the walker must not reuse base sizes).

TEST_CASE("ComputeLayout: DXT1 base mip 1024x1024 produces correct totals", "[Graphics][MipmapLayout][I-16]")
{
    const auto L = ComputeLayout(PacDXT1, 1024, 1024);
    REQUIRE(L.tightPitch == 256 * 8);       // 256 block-cols × 8 bytes
    REQUIRE(L.rowCount == 256);             // 256 block-rows
    REQUIRE(L.dataSize == 1024 * 1024 / 2); // 0.5 bpp for DXT1
}

TEST_CASE("ComputeLayout: DXT1 mip 3 of 1024x1024 is 128x128 layout (B-016)", "[Graphics][MipmapLayout][I-16]")
{
    // B-016 broken state: sub-upload at mip 3 used the base mip's
    // dataSize (524288 bytes for 1024x1024 DXT1).  Correct sub-upload
    // passes the mip-3 dataSize (8192 bytes for 128x128 DXT1).
    const auto L = ComputeLayout(PacDXT1, 128, 128);
    REQUIRE(L.tightPitch == 32 * 8);
    REQUIRE(L.rowCount == 32);
    REQUIRE(L.dataSize == 128 * 128 / 2);
}

TEST_CASE("ComputeLayout: DXT5 has 2x DXT1 byte budget", "[Graphics][MipmapLayout][I-15]")
{
    // DXT5 stores 16 bytes/block (alpha + color), DXT1 stores 8.
    const auto dxt1 = ComputeLayout(PacDXT1, 64, 64);
    const auto dxt5 = ComputeLayout(PacDXT5, 64, 64);
    REQUIRE(dxt5.tightPitch == dxt1.tightPitch * 2);
    REQUIRE(dxt5.dataSize == dxt1.dataSize * 2);
}

TEST_CASE("ComputeLayout: DXT1 4x4 base block - minimum size", "[Graphics][MipmapLayout][I-15]")
{
    // Exact-block-aligned smallest non-clamped case.
    const auto L = ComputeLayout(PacDXT1, 4, 4);
    REQUIRE(L.tightPitch == 8);
    REQUIRE(L.rowCount == 1);
    REQUIRE(L.dataSize == 8);
}

TEST_CASE("ComputeLayout: DXT1 2x2 mip rounds up to one 4x4 block", "[Graphics][MipmapLayout][I-15]")
{
    // Tail-of-chain mips below 4x4 must still produce one full
    // block; the source data is padded but the upload must pass
    // the block-aligned byte count, not the raw 2x2 size.
    const auto L = ComputeLayout(PacDXT1, 2, 2);
    REQUIRE(L.tightPitch == 8);
    REQUIRE(L.rowCount == 1);
    REQUIRE(L.dataSize == 8);
}

TEST_CASE("ComputeLayout: DXT1 1x1 mip rounds up to one 4x4 block (B-021 small-mip)", "[Graphics][MipmapLayout][I-15]")
{
    // The "small mip < block size" case that drove B-021 (tight
    // packing < driver-imposed minimum stride).  The dst here
    // must report 8 bytes regardless of the 1×1 source flat_quad.
    const auto L = ComputeLayout(PacDXT1, 1, 1);
    REQUIRE(L.tightPitch == 8);
    REQUIRE(L.rowCount == 1);
    REQUIRE(L.dataSize == 8);
}

TEST_CASE("ComputeLayout: DXT5 1x1 mip rounds to one 4x4 block of 16 bytes", "[Graphics][MipmapLayout][I-15]")
{
    const auto L = ComputeLayout(PacDXT5, 1, 1);
    REQUIRE(L.tightPitch == 16);
    REQUIRE(L.rowCount == 1);
    REQUIRE(L.dataSize == 16);
}

TEST_CASE("ComputeLayout: ARGB8888 uses w*4 byte pitch", "[Graphics][MipmapLayout][I-15]")
{
    const auto L = ComputeLayout(PacARGB8888, 64, 32);
    REQUIRE(L.tightPitch == 64 * 4);
    REQUIRE(L.rowCount == 32);
    REQUIRE(L.dataSize == 64 * 32 * 4);
}

TEST_CASE("ComputeLayout: 16-bit linear formats use w*2 byte pitch", "[Graphics][MipmapLayout][I-15]")
{
    for (PacFormat fmt : {PacARGB1555, PacRGB565, PacARGB4444})
    {
        const auto L = ComputeLayout(fmt, 32, 16);
        REQUIRE(L.tightPitch == 32 * 2);
        REQUIRE(L.rowCount == 16);
        REQUIRE(L.dataSize == 32 * 16 * 2);
    }
}

TEST_CASE("ComputeLayout: full mip chain produces shrinking sizes (B-016)", "[Graphics][MipmapLayout][I-16]")
{
    // The B-016 invariant: each mip's dataSize is computed
    // independently.  Walking 1024 → 512 → 256 → 128 → 64 → 32 →
    // 16 → 8 → 4 → 2 → 1 must yield strictly decreasing
    // dataSizes (DXT1) until the block-clamp floor at 8 bytes.
    int dims[] = {1024, 512, 256, 128, 64, 32, 16, 8, 4}; // all >= 4
    int prev = -1;
    for (int d : dims)
    {
        const auto L = ComputeLayout(PacDXT1, d, d);
        REQUIRE(L.dataSize > 0);
        if (prev >= 0)
        {
            REQUIRE(L.dataSize < prev);
        }
        prev = L.dataSize;
    }

    // Tail mips (2x2, 1x1) clamp to the 8-byte minimum.
    REQUIRE(ComputeLayout(PacDXT1, 2, 2).dataSize == 8);
    REQUIRE(ComputeLayout(PacDXT1, 1, 1).dataSize == 8);
}

// I-17 - Texture mipmap chain is GL-complete before first sample.
//
// The upload loop in `TextureGL33::UploadToGPU` walks
// `for (int i = levelMin; i < _nMipmaps; i++)` and calls
// `glCompressedTexSubImage2D` / `glTexSubImage2D` for every level.
// GL completeness requires (a) every level 0..N-1 present,
// (b) each level's storage parameters match the format's
// block-alignment / pitch rules.  These tests pin the math side
// of the contract: walking a full chain from base to 1x1 produces
// a positive-size layout at every level for every shipped format.
// A regression that returns dataSize == 0 (or skips a level) lands
// here, not as a "texture incomplete at first sample" KHR_debug
// warning on a screenshot.

namespace
{
int ExpectedMipCount(int wh)
{
    int n = 0;
    while (wh > 0)
    {
        ++n;
        wh >>= 1;
    }
    return n;
}
} // namespace

TEST_CASE("ComputeLayout: every level of a 1024-mip chain has positive size (DXT1)", "[Graphics][MipmapLayout][I-17]")
{
    const int expectedLevels = ExpectedMipCount(1024); // 11
    REQUIRE(expectedLevels == 11);
    for (int lvl = 0; lvl < expectedLevels; ++lvl)
    {
        const int dim = 1024 >> lvl;
        const int w = dim > 0 ? dim : 1;
        const int h = dim > 0 ? dim : 1;
        const auto L = ComputeLayout(PacDXT1, w, h);
        CAPTURE(lvl, w, h);
        REQUIRE(L.dataSize > 0);
        REQUIRE(L.tightPitch > 0);
        REQUIRE(L.rowCount > 0);
    }
}

TEST_CASE("ComputeLayout: every level of a 1024-mip chain has positive size (DXT5)", "[Graphics][MipmapLayout][I-17]")
{
    const int expectedLevels = ExpectedMipCount(1024);
    for (int lvl = 0; lvl < expectedLevels; ++lvl)
    {
        const int dim = 1024 >> lvl;
        const int w = dim > 0 ? dim : 1;
        const int h = dim > 0 ? dim : 1;
        const auto L = ComputeLayout(PacDXT5, w, h);
        CAPTURE(lvl, w, h);
        REQUIRE(L.dataSize >= 16); // DXT5 minimum block
        REQUIRE(L.tightPitch >= 16);
        REQUIRE(L.rowCount >= 1);
    }
}

TEST_CASE("ComputeLayout: every level of a 512-mip chain has positive size (ARGB8888)",
          "[Graphics][MipmapLayout][I-17]")
{
    const int expectedLevels = ExpectedMipCount(512); // 10
    REQUIRE(expectedLevels == 10);
    for (int lvl = 0; lvl < expectedLevels; ++lvl)
    {
        const int dim = 512 >> lvl;
        const int w = dim > 0 ? dim : 1;
        const int h = dim > 0 ? dim : 1;
        const auto L = ComputeLayout(PacARGB8888, w, h);
        CAPTURE(lvl, w, h);
        REQUIRE(L.dataSize == w * h * 4);
        REQUIRE(L.tightPitch == w * 4);
        REQUIRE(L.rowCount == h);
    }
}

TEST_CASE("ComputeLayout: non-square chain reaches 1x1 without zero-size mips (I-17)", "[Graphics][MipmapLayout][I-17]")
{
    // 256x64 reaches 1x1 in max(log2(256), log2(64))+1 = 9 levels;
    // each level's W and H halve independently (clamped to 1).
    int w = 256, h = 64;
    int levels = 0;
    while (w > 1 || h > 1)
    {
        const auto L = ComputeLayout(PacDXT1, w, h);
        CAPTURE(levels, w, h);
        REQUIRE(L.dataSize > 0);
        if (w > 1)
            w >>= 1;
        if (h > 1)
            h >>= 1;
        ++levels;
    }
    // Final 1x1 level must also be uploadable.
    const auto tail = ComputeLayout(PacDXT1, 1, 1);
    REQUIRE(tail.dataSize == 8);
    REQUIRE(levels == 8); // log2(256) = 8 halvings to reach 1
}

TEST_CASE("ComputeLayout: non-square dimensions are independent", "[Graphics][MipmapLayout][I-15]")
{
    // A 128x32 mip should yield (128 block-cols/4) * 8 bytes per
    // block-row * (32/4) block-rows = 32*8 * 8 = 2048.
    const auto L = ComputeLayout(PacDXT1, 128, 32);
    REQUIRE(L.tightPitch == (128 / 4) * 8);
    REQUIRE(L.rowCount == 32 / 4);
    REQUIRE(L.dataSize == 128 * 32 / 2);
}
