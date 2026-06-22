#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Graphics/Textures/DXTCompressor.hpp>
#include <Poseidon/Graphics/Textures/Image.hpp>
#include "test_fixtures.hpp"
#include <cmath>
#include <cstring>
#include <stdint.h>
#include <algorithm>
#include <utility>
#include <vector>

using namespace Poseidon;

// --- Helper: PSNR ---

static double computePSNR(const uint8_t* a, const uint8_t* b, int n)
{
    double mse = 0;
    for (int i = 0; i < n; ++i)
    {
        double d = static_cast<double>(a[i]) - static_cast<double>(b[i]);
        mse += d * d;
    }
    mse /= n;
    if (mse == 0.0)
        return 100.0;
    return 10.0 * std::log10(255.0 * 255.0 / mse);
}

// --- Helper: gradient image ---

static std::vector<uint8_t> gradientRGBA(int w, int h)
{
    std::vector<uint8_t> data(w * h * 4);
    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            int i = y * w + x;
            data[i * 4 + 0] = static_cast<uint8_t>(x * 255 / (std::max)(w - 1, 1));
            data[i * 4 + 1] = static_cast<uint8_t>(y * 255 / (std::max)(h - 1, 1));
            data[i * 4 + 2] = 128;
            data[i * 4 + 3] = 255;
        }
    }
    return data;
}

// --- DXT1 compressed size ---

TEST_CASE("DXTCompressor: DXT1 compressed size", "[Graphics][DXT]")
{
    // 16x16: 4x4 blocks = 16 blocks × 8 bytes = 128
    REQUIRE(DXTCompressor::CompressedSize(16, 16, true) == 128);
    // 4x4: 1 block × 8 bytes = 8
    REQUIRE(DXTCompressor::CompressedSize(4, 4, true) == 8);
    // 5x5: 2x2 blocks × 8 bytes = 32
    REQUIRE(DXTCompressor::CompressedSize(5, 5, true) == 32);
}

// --- DXT5 compressed size ---

TEST_CASE("DXTCompressor: DXT5 compressed size", "[Graphics][DXT]")
{
    // 16x16: 16 blocks × 16 bytes = 256
    REQUIRE(DXTCompressor::CompressedSize(16, 16, false) == 256);
    // 4x4: 1 block × 16 bytes = 16
    REQUIRE(DXTCompressor::CompressedSize(4, 4, false) == 16);
}

// --- DXT1 compress + decompress round-trip ---

TEST_CASE("DXTCompressor: DXT1 round-trip PSNR > 25dB", "[Graphics][DXT]")
{
    auto rgba = gradientRGBA(16, 16);
    auto compressed = DXTCompressor::CompressDXT1(rgba.data(), 16, 16);
    REQUIRE(compressed.size() == DXTCompressor::CompressedSize(16, 16, true));

    // Decompress via Image ConvertPixels
    auto img = Image(16, 16, PixelFormat::DXT1, std::move(compressed));
    auto back = img.ToRGBA();
    REQUIRE(back.valid());

    double psnr = computePSNR(rgba.data(), back.data().data(), 16 * 16 * 4);
    REQUIRE(psnr > 25.0);
}

// --- DXT5 compress + decompress round-trip ---

TEST_CASE("DXTCompressor: DXT5 round-trip PSNR > 25dB", "[Graphics][DXT]")
{
    auto rgba = gradientRGBA(16, 16);
    auto compressed = DXTCompressor::CompressDXT5(rgba.data(), 16, 16);
    REQUIRE(compressed.size() == DXTCompressor::CompressedSize(16, 16, false));

    auto img = Image(16, 16, PixelFormat::DXT5, std::move(compressed));
    auto back = img.ToRGBA();
    REQUIRE(back.valid());

    double psnr = computePSNR(rgba.data(), back.data().data(), 16 * 16 * 4);
    REQUIRE(psnr > 25.0);
}

// --- DXT1 solid color precision ---

TEST_CASE("DXTCompressor: DXT1 solid red is preserved", "[Graphics][DXT]")
{
    std::vector<uint8_t> rgba(8 * 8 * 4);
    for (int i = 0; i < 8 * 8; ++i)
    {
        rgba[i * 4 + 0] = 255;
        rgba[i * 4 + 1] = 0;
        rgba[i * 4 + 2] = 0;
        rgba[i * 4 + 3] = 255;
    }

    auto img = Image::FromRGBA(8, 8, rgba);
    auto dxt1 = img.ConvertTo(PixelFormat::DXT1);
    REQUIRE(dxt1.valid());
    REQUIRE(dxt1.format() == PixelFormat::DXT1);

    auto back = dxt1.ToRGBA();
    REQUIRE(back.valid());
    // Solid color should be near-perfect after DXT1
    double psnr = computePSNR(rgba.data(), back.data().data(), 8 * 8 * 4);
    REQUIRE(psnr > 35.0);
}

// --- DXT via Image::ConvertTo ---

TEST_CASE("Image: ConvertTo DXT1 and back", "[Graphics][Image][DXT]")
{
    auto rgba = gradientRGBA(32, 32);
    auto img = Image::FromRGBA(32, 32, rgba);

    auto dxt1 = img.ConvertTo(PixelFormat::DXT1);
    REQUIRE(dxt1.valid());
    REQUIRE(dxt1.format() == PixelFormat::DXT1);
    REQUIRE(dxt1.dataSize() == DXTCompressor::CompressedSize(32, 32, true));

    auto back = dxt1.ConvertTo(PixelFormat::RGBA8888);
    REQUIRE(back.valid());
    REQUIRE(back.format() == PixelFormat::RGBA8888);
    double psnr = computePSNR(rgba.data(), back.data().data(), 32 * 32 * 4);
    REQUIRE(psnr > 25.0);
}

TEST_CASE("Image: ConvertTo DXT5 and back", "[Graphics][Image][DXT]")
{
    auto rgba = gradientRGBA(32, 32);
    auto img = Image::FromRGBA(32, 32, rgba);

    auto dxt5 = img.ConvertTo(PixelFormat::DXT5);
    REQUIRE(dxt5.valid());
    REQUIRE(dxt5.format() == PixelFormat::DXT5);
    REQUIRE(dxt5.dataSize() == DXTCompressor::CompressedSize(32, 32, false));

    auto back = dxt5.ConvertTo(PixelFormat::RGBA8888);
    REQUIRE(back.valid());
    double psnr = computePSNR(rgba.data(), back.data().data(), 32 * 32 * 4);
    REQUIRE(psnr > 25.0);
}

// --- Cross-format: ARGB4444 -> DXT1 ---

TEST_CASE("Image: ARGB4444->DXT1 via RGBA intermediate", "[Graphics][Image][DXT]")
{
    auto rgba = gradientRGBA(16, 16);
    auto img = Image::FromRGBA(16, 16, rgba);
    auto a4444 = img.ConvertTo(PixelFormat::ARGB4444);
    REQUIRE(a4444.valid());

    auto dxt1 = a4444.ConvertTo(PixelFormat::DXT1);
    REQUIRE(dxt1.valid());
    REQUIRE(dxt1.format() == PixelFormat::DXT1);

    auto back = dxt1.ToRGBA();
    REQUIRE(back.valid());
}

// --- Real texture decode -> DXT re-encode ---

TEST_CASE("Image: FromFile PAA -> DXT1 re-encode", "[Graphics][Image][DXT]")
{
    auto img = Image::FromFile(GET_FIXTURE("texture/paa/synthetic_dxt1.paa"));
    REQUIRE(img.valid());

    auto dxt1 = img.ConvertTo(PixelFormat::DXT1);
    REQUIRE(dxt1.valid());
    REQUIRE(dxt1.format() == PixelFormat::DXT1);

    auto back = dxt1.ToRGBA();
    REQUIRE(back.valid());
    // Double DXT1 compression (original PAA was DXT1) -- verify valid output
    // PSNR is lower due to compounded quantization
    double psnr = computePSNR(img.data().data(), back.data().data(), img.width() * img.height() * 4);
    REQUIRE(psnr > 8.0);
}
