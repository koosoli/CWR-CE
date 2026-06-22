#include <catch2/catch_test_macros.hpp>

#include <Poseidon/Graphics/Rendering/Frame/TextureDesc.hpp>

// Phase E.2 — pin texture-description invariants for the frame layer.  Three
// catalog bugs covered:
//
//   B-014 — DXT1 alpha lost when format chosen without alpha.
//   B-015 — Mip chain incompleteness producing wrong-colour samples.
//   B-021 — Row-pitch mismatch for small DXT mips → black stripes.
//
// All tests are pure: no GL ctx, no upload, no driver round-trip.

namespace v2 = Poseidon::render::frame;

// I-18 / B-014 — format encodes alpha presence

TEST_CASE("Frame/I-18: HasAlpha distinguishes RGBA/BGRA/DXT-with-alpha from RGB/DXT1",
          "[render-frame][invariants][I-18][texture-desc]")
{
    REQUIRE(Poseidon::render::frame::HasAlpha(Poseidon::render::frame::PixelFormat::RGBA8));
    REQUIRE(Poseidon::render::frame::HasAlpha(Poseidon::render::frame::PixelFormat::BGRA8));
    REQUIRE(Poseidon::render::frame::HasAlpha(Poseidon::render::frame::PixelFormat::DXT1A));
    REQUIRE(Poseidon::render::frame::HasAlpha(Poseidon::render::frame::PixelFormat::DXT3));
    REQUIRE(Poseidon::render::frame::HasAlpha(Poseidon::render::frame::PixelFormat::DXT5));

    REQUIRE_FALSE(Poseidon::render::frame::HasAlpha(Poseidon::render::frame::PixelFormat::RGB8));
    REQUIRE_FALSE(Poseidon::render::frame::HasAlpha(Poseidon::render::frame::PixelFormat::R8));
    REQUIRE_FALSE(Poseidon::render::frame::HasAlpha(Poseidon::render::frame::PixelFormat::DXT1));
}

TEST_CASE("Frame/I-18: ValidateTextureDesc rejects alpha-source with alpha-less format",
          "[render-frame][invariants][I-18][texture-desc]")
{
    // The exact B-014 failure: DXT1 (no alpha) chosen for a
    // source that has chromakey alpha → transparency lost.
    Poseidon::render::frame::TextureDesc d;
    d.width = 64;
    d.height = 64;
    d.format = Poseidon::render::frame::PixelFormat::DXT1;
    d.sourceHasAlpha = true;

    const char* err = Poseidon::render::frame::ValidateTextureDesc(d);
    REQUIRE(err != nullptr);
}

TEST_CASE("Frame/I-18: ValidateTextureDesc accepts alpha-source with alpha-format",
          "[render-frame][invariants][I-18][texture-desc]")
{
    Poseidon::render::frame::TextureDesc d;
    d.width = 64;
    d.height = 64;
    d.format = Poseidon::render::frame::PixelFormat::DXT1A;
    d.sourceHasAlpha = true;
    REQUIRE(Poseidon::render::frame::ValidateTextureDesc(d) == nullptr);
}

// I-17 / B-015 — mip chain completeness

TEST_CASE("Frame/I-17: MaxMipCount caps at log2(max(w,h)) + 1", "[render-frame][invariants][I-17][texture-desc]")
{
    REQUIRE(Poseidon::render::frame::MaxMipCount(1, 1) == 1);
    REQUIRE(Poseidon::render::frame::MaxMipCount(2, 2) == 2);
    REQUIRE(Poseidon::render::frame::MaxMipCount(4, 4) == 3);
    REQUIRE(Poseidon::render::frame::MaxMipCount(64, 64) == 7); // 64,32,16,8,4,2,1
    REQUIRE(Poseidon::render::frame::MaxMipCount(256, 256) == 9);
    REQUIRE(Poseidon::render::frame::MaxMipCount(1024, 1024) == 11);

    // Non-square: max dimension drives.
    REQUIRE(Poseidon::render::frame::MaxMipCount(64, 16) == 7);
    REQUIRE(Poseidon::render::frame::MaxMipCount(32, 128) == 8);
}

TEST_CASE("Frame/I-17: MipDim halves with min clamp at 1", "[render-frame][invariants][I-17][texture-desc]")
{
    REQUIRE(Poseidon::render::frame::MipDim(64, 0) == 64);
    REQUIRE(Poseidon::render::frame::MipDim(64, 1) == 32);
    REQUIRE(Poseidon::render::frame::MipDim(64, 6) == 1);
    REQUIRE(Poseidon::render::frame::MipDim(64, 7) == 1);  // clamped, not 0
    REQUIRE(Poseidon::render::frame::MipDim(64, 99) == 1); // never returns 0
}

TEST_CASE("Frame/I-17: ValidateTextureDesc rejects mipCount > maxMips",
          "[render-frame][invariants][I-17][texture-desc]")
{
    Poseidon::render::frame::TextureDesc d;
    d.width = 64;
    d.height = 64;
    d.format = Poseidon::render::frame::PixelFormat::RGBA8;
    d.mipCount = 99; // way beyond max 7

    REQUIRE(Poseidon::render::frame::ValidateTextureDesc(d) != nullptr);
}

TEST_CASE("Frame/I-17: ValidateTextureDesc accepts mipCount within bounds",
          "[render-frame][invariants][I-17][texture-desc]")
{
    Poseidon::render::frame::TextureDesc d;
    d.width = 64;
    d.height = 64;
    d.format = Poseidon::render::frame::PixelFormat::RGBA8;
    d.mipCount = 1;
    REQUIRE(Poseidon::render::frame::ValidateTextureDesc(d) == nullptr);

    d.mipCount = 7; // exactly max
    REQUIRE(Poseidon::render::frame::ValidateTextureDesc(d) == nullptr);

    d.mipCount = 4; // partial chain
    REQUIRE(Poseidon::render::frame::ValidateTextureDesc(d) == nullptr);
}

// I-15 / B-021 — row pitch / block alignment

TEST_CASE("Frame/I-15: RowStride for uncompressed formats is width * bpp",
          "[render-frame][invariants][I-15][texture-desc]")
{
    REQUIRE(Poseidon::render::frame::RowStride(Poseidon::render::frame::PixelFormat::RGBA8, 64) == 64 * 4);
    REQUIRE(Poseidon::render::frame::RowStride(Poseidon::render::frame::PixelFormat::RGB8, 64) == 64 * 3);
    REQUIRE(Poseidon::render::frame::RowStride(Poseidon::render::frame::PixelFormat::R8, 64) == 64);
    REQUIRE(Poseidon::render::frame::RowStride(Poseidon::render::frame::PixelFormat::BGRA8, 16) == 16 * 4);
}

TEST_CASE("Frame/I-15: RowStride for DXT formats is blocks * bytesPerBlock",
          "[render-frame][invariants][I-15][texture-desc]")
{
    // 64-wide DXT1 = 16 blocks * 8 bytes = 128 bytes
    REQUIRE(Poseidon::render::frame::RowStride(Poseidon::render::frame::PixelFormat::DXT1, 64) == 128);
    // 64-wide DXT5 = 16 blocks * 16 bytes = 256 bytes
    REQUIRE(Poseidon::render::frame::RowStride(Poseidon::render::frame::PixelFormat::DXT5, 64) == 256);
    // The B-021 case: small mip where each row is < typical staging
    // pitch.  Caller must use this value, not assume base-mip pitch.
    REQUIRE(Poseidon::render::frame::RowStride(Poseidon::render::frame::PixelFormat::DXT1, 4) == 8);  // 1 block
    REQUIRE(Poseidon::render::frame::RowStride(Poseidon::render::frame::PixelFormat::DXT1, 8) == 16); // 2 blocks
}

TEST_CASE("Frame/I-15: RowStride rounds up for non-block-aligned widths",
          "[render-frame][invariants][I-15][texture-desc]")
{
    // Non-block-aligned widths produce ceil(w/4) blocks per row.
    REQUIRE(Poseidon::render::frame::RowStride(Poseidon::render::frame::PixelFormat::DXT1, 5) == 16); // 2 blocks
    REQUIRE(Poseidon::render::frame::RowStride(Poseidon::render::frame::PixelFormat::DXT1, 7) == 16);
    REQUIRE(Poseidon::render::frame::RowStride(Poseidon::render::frame::PixelFormat::DXT1, 1) == 8);
}

TEST_CASE("Frame/I-15: ValidateTextureDesc rejects compressed-format non-block-aligned base",
          "[render-frame][invariants][I-15][texture-desc]")
{
    Poseidon::render::frame::TextureDesc d;
    d.width = 5;
    d.height = 64;
    d.format = Poseidon::render::frame::PixelFormat::DXT1;

    REQUIRE(Poseidon::render::frame::ValidateTextureDesc(d) != nullptr);

    d.width = 64;
    d.height = 7;
    REQUIRE(Poseidon::render::frame::ValidateTextureDesc(d) != nullptr);
}

TEST_CASE("Frame/I-15: ValidateTextureDesc accepts compressed at block-aligned dims",
          "[render-frame][invariants][I-15][texture-desc]")
{
    Poseidon::render::frame::TextureDesc d;
    d.width = 64;
    d.height = 64;
    d.format = Poseidon::render::frame::PixelFormat::DXT1;
    REQUIRE(Poseidon::render::frame::ValidateTextureDesc(d) == nullptr);
}

// IsCompressed / BlockWidth / BytesPerElement classification tests

TEST_CASE("Frame: IsCompressed classifies formats correctly", "[render-frame][texture-desc]")
{
    REQUIRE(Poseidon::render::frame::IsCompressed(Poseidon::render::frame::PixelFormat::DXT1));
    REQUIRE(Poseidon::render::frame::IsCompressed(Poseidon::render::frame::PixelFormat::DXT1A));
    REQUIRE(Poseidon::render::frame::IsCompressed(Poseidon::render::frame::PixelFormat::DXT3));
    REQUIRE(Poseidon::render::frame::IsCompressed(Poseidon::render::frame::PixelFormat::DXT5));

    REQUIRE_FALSE(Poseidon::render::frame::IsCompressed(Poseidon::render::frame::PixelFormat::RGBA8));
    REQUIRE_FALSE(Poseidon::render::frame::IsCompressed(Poseidon::render::frame::PixelFormat::RGB8));
    REQUIRE_FALSE(Poseidon::render::frame::IsCompressed(Poseidon::render::frame::PixelFormat::BGRA8));
    REQUIRE_FALSE(Poseidon::render::frame::IsCompressed(Poseidon::render::frame::PixelFormat::R8));
}

TEST_CASE("Frame: BlockWidth is 4 for DXT, 1 for uncompressed", "[render-frame][texture-desc]")
{
    REQUIRE(Poseidon::render::frame::BlockWidth(Poseidon::render::frame::PixelFormat::DXT1) == 4);
    REQUIRE(Poseidon::render::frame::BlockWidth(Poseidon::render::frame::PixelFormat::DXT5) == 4);
    REQUIRE(Poseidon::render::frame::BlockWidth(Poseidon::render::frame::PixelFormat::RGBA8) == 1);
    REQUIRE(Poseidon::render::frame::BlockWidth(Poseidon::render::frame::PixelFormat::R8) == 1);
}
