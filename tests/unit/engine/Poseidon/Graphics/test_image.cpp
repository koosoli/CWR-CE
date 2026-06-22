#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <Poseidon/Graphics/Textures/Image.hpp>
#include <Poseidon/Graphics/Textures/ImageContainer.hpp>
#include "test_fixtures.hpp"
#include <cmath>
#include <numeric>
#include <stdint.h>
#include <string>
#include <utility>
#include <vector>

using namespace Poseidon;

// --- Helper: create solid-color RGBA test image ---

static std::vector<uint8_t> solidRGBA(int w, int h, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    std::vector<uint8_t> data(w * h * 4);
    for (int i = 0; i < w * h; ++i)
    {
        data[i * 4 + 0] = r;
        data[i * 4 + 1] = g;
        data[i * 4 + 2] = b;
        data[i * 4 + 3] = a;
    }
    return data;
}

// --- Helper: gradient RGBA test image ---

static std::vector<uint8_t> gradientRGBA(int w, int h)
{
    std::vector<uint8_t> data(w * h * 4);
    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            int i = y * w + x;
            data[i * 4 + 0] = static_cast<uint8_t>(x * 255 / (w - 1));
            data[i * 4 + 1] = static_cast<uint8_t>(y * 255 / (h - 1));
            data[i * 4 + 2] = static_cast<uint8_t>((x + y) * 255 / (w + h - 2));
            data[i * 4 + 3] = 255;
        }
    }
    return data;
}

// --- Helper: compute PSNR between two RGBA buffers ---

static double computePSNR(const uint8_t* a, const uint8_t* b, int w, int h)
{
    double mse = 0;
    int n = w * h * 4;
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

// --- Image construction ---

TEST_CASE("Image: FromRGBA creates valid image", "[Graphics][Image]")
{
    auto rgba = solidRGBA(4, 4, 255, 0, 0, 255);
    auto img = Image::FromRGBA(4, 4, std::move(rgba));
    REQUIRE(img.valid());
    REQUIRE(img.width() == 4);
    REQUIRE(img.height() == 4);
    REQUIRE(img.format() == PixelFormat::RGBA8888);
    REQUIRE(img.dataSize() == 4 * 4 * 4);
}

TEST_CASE("Image: FromRGBA rejects wrong-sized data", "[Graphics][Image]")
{
    std::vector<uint8_t> bad(10);
    auto img = Image::FromRGBA(4, 4, std::move(bad));
    REQUIRE_FALSE(img.valid());
}

TEST_CASE("Image: default is invalid", "[Graphics][Image]")
{
    Image img;
    REQUIRE_FALSE(img.valid());
}

// --- ARGB4444 round-trip ---

TEST_CASE("Image: RGBA->ARGB4444->RGBA round-trip", "[Graphics][Image]")
{
    auto rgba = solidRGBA(8, 8, 0xAA, 0x55, 0xCC, 0xFF);
    auto img = Image::FromRGBA(8, 8, rgba);

    auto a4444 = img.ConvertTo(PixelFormat::ARGB4444);
    REQUIRE(a4444.valid());
    REQUIRE(a4444.format() == PixelFormat::ARGB4444);
    REQUIRE(a4444.dataSize() == 8 * 8 * 2);

    auto back = a4444.ToRGBA();
    REQUIRE(back.valid());
    REQUIRE(back.format() == PixelFormat::RGBA8888);
    // 4-bit precision: max error is 8 per channel (16 values -> 255 range)
    double psnr = computePSNR(rgba.data(), back.data().data(), 8, 8);
    REQUIRE(psnr > 30.0);
}

// --- ARGB1555 round-trip ---

TEST_CASE("Image: RGBA->ARGB1555->RGBA round-trip", "[Graphics][Image]")
{
    auto rgba = solidRGBA(8, 8, 0x80, 0x40, 0xC0, 0xFF);
    auto img = Image::FromRGBA(8, 8, rgba);

    auto a1555 = img.ConvertTo(PixelFormat::ARGB1555);
    REQUIRE(a1555.valid());
    REQUIRE(a1555.format() == PixelFormat::ARGB1555);
    REQUIRE(a1555.dataSize() == 8 * 8 * 2);

    auto back = a1555.ToRGBA();
    REQUIRE(back.valid());
    // 5-bit precision
    double psnr = computePSNR(rgba.data(), back.data().data(), 8, 8);
    REQUIRE(psnr > 25.0);
}

// --- RGB565 round-trip ---

TEST_CASE("Image: RGBA->RGB565->RGBA round-trip", "[Graphics][Image]")
{
    auto rgba = solidRGBA(8, 8, 0x80, 0x40, 0xC0, 0xFF);
    auto img = Image::FromRGBA(8, 8, rgba);

    auto rgb565 = img.ConvertTo(PixelFormat::RGB565);
    REQUIRE(rgb565.valid());
    REQUIRE(rgb565.format() == PixelFormat::RGB565);
    REQUIRE(rgb565.dataSize() == 8 * 8 * 2);

    auto back = rgb565.ToRGBA();
    REQUIRE(back.valid());
    // RGB565 drops alpha
    REQUIRE(back.data()[3] == 255);
    double psnr = computePSNR(rgba.data(), back.data().data(), 8, 8);
    REQUIRE(psnr > 25.0);
}

// --- AI88 round-trip ---

TEST_CASE("Image: RGBA->AI88->RGBA round-trip preserves grayscale", "[Graphics][Image]")
{
    // Use a gray color so luma conversion is lossless
    auto rgba = solidRGBA(8, 8, 128, 128, 128, 200);
    auto img = Image::FromRGBA(8, 8, rgba);

    auto ai = img.ConvertTo(PixelFormat::AI88);
    REQUIRE(ai.valid());
    REQUIRE(ai.format() == PixelFormat::AI88);
    REQUIRE(ai.dataSize() == 8 * 8 * 2);

    auto back = ai.ToRGBA();
    REQUIRE(back.valid());
    // Gray->AI88->RGBA should be nearly exact
    REQUIRE(back.data()[0] == 128); // R=intensity
    REQUIRE(back.data()[1] == 128); // G=intensity
    REQUIRE(back.data()[2] == 128); // B=intensity
    REQUIRE(back.data()[3] == 200); // A preserved
}

// --- ARGB8888 round-trip ---

TEST_CASE("Image: RGBA->ARGB8888->RGBA round-trip is lossless", "[Graphics][Image]")
{
    auto rgba = gradientRGBA(16, 16);
    auto img = Image::FromRGBA(16, 16, rgba);

    auto argb = img.ConvertTo(PixelFormat::ARGB8888);
    REQUIRE(argb.valid());
    REQUIRE(argb.format() == PixelFormat::ARGB8888);
    REQUIRE(argb.dataSize() == 16 * 16 * 4);

    auto back = argb.ToRGBA();
    REQUIRE(back.valid());
    // 8-bit to 8-bit: must be lossless
    REQUIRE(back.data() == img.data());
}

// --- Cross-format conversion ---

TEST_CASE("Image: ARGB4444->ARGB1555 via RGBA intermediate", "[Graphics][Image]")
{
    auto rgba = gradientRGBA(8, 8);
    auto img = Image::FromRGBA(8, 8, rgba);

    auto a4444 = img.ConvertTo(PixelFormat::ARGB4444);
    REQUIRE(a4444.valid());

    auto a1555 = a4444.ConvertTo(PixelFormat::ARGB1555);
    REQUIRE(a1555.valid());
    REQUIRE(a1555.format() == PixelFormat::ARGB1555);

    auto back = a1555.ToRGBA();
    REQUIRE(back.valid());
}

// --- ConvertTo same format is no-op ---

TEST_CASE("Image: ConvertTo same format returns identical image", "[Graphics][Image]")
{
    auto rgba = solidRGBA(4, 4, 100, 200, 50, 255);
    auto img = Image::FromRGBA(4, 4, rgba);

    auto same = img.ConvertTo(PixelFormat::RGBA8888);
    REQUIRE(same.valid());
    REQUIRE(same.data() == img.data());
}

// --- FromFile with PAA fixtures ---

TEST_CASE("Image: FromFile reads PAA (DXT1)", "[Graphics][Image]")
{
    auto img = Image::FromFile(GET_FIXTURE("texture/paa/synthetic_dxt1.paa"));
    REQUIRE(img.valid());
    REQUIRE(img.width() == 64);
    REQUIRE(img.height() == 64);
    REQUIRE(img.format() == PixelFormat::RGBA8888);
}

TEST_CASE("Image: FromFile reads PAA (DXT5)", "[Graphics][Image]")
{
    auto img = Image::FromFile(GET_FIXTURE("texture/paa/synthetic_dxt5.paa"));
    REQUIRE(img.valid());
    REQUIRE(img.width() == 32);
    REQUIRE(img.height() == 32);
    REQUIRE(img.format() == PixelFormat::RGBA8888);
}

TEST_CASE("Image: FromFile reads PAC", "[Graphics][Image]")
{
    auto img = Image::FromFile(GET_FIXTURE("texture/pac/synthetic_default.pac"));
    REQUIRE(img.valid());
    REQUIRE(img.width() == 64);
    REQUIRE(img.height() == 64);
}

TEST_CASE("Image: FromFile returns invalid for nonexistent file", "[Graphics][Image]")
{
    auto img = Image::FromFile("nonexistent.paa");
    REQUIRE_FALSE(img.valid());
}

TEST_CASE("Image: FromFile returns invalid for unknown extension", "[Graphics][Image]")
{
    auto img = Image::FromFile("test.xyz");
    REQUIRE_FALSE(img.valid());
}

// --- ImageContainer registry ---

TEST_CASE("ImageContainerRegistry: lookup by extension", "[Graphics][ImageContainer]")
{
    const auto* paa = ImageContainerRegistry::FindByExtension(".paa");
    REQUIRE(paa != nullptr);
    REQUIRE(paa->container == ImageContainer::PAA);

    const auto* png = ImageContainerRegistry::FindByExtension(".png");
    REQUIRE(png != nullptr);
    REQUIRE(png->container == ImageContainer::PNG);

    const auto* bmp = ImageContainerRegistry::FindByExtension(".bmp");
    REQUIRE(bmp != nullptr);
    REQUIRE(bmp->container == ImageContainer::BMP);

    const auto* tga = ImageContainerRegistry::FindByExtension(".tga");
    REQUIRE(tga != nullptr);
    REQUIRE(tga->container == ImageContainer::TGA);
}

TEST_CASE("ImageContainerRegistry: case-insensitive extension", "[Graphics][ImageContainer]")
{
    const auto* paa = ImageContainerRegistry::FindByExtension(".PAA");
    REQUIRE(paa != nullptr);
    REQUIRE(paa->container == ImageContainer::PAA);
}

TEST_CASE("ImageContainerRegistry: unknown extension returns nullptr", "[Graphics][ImageContainer]")
{
    REQUIRE(ImageContainerRegistry::FindByExtension(".xyz") == nullptr);
    REQUIRE(ImageContainerRegistry::FindByExtension(nullptr) == nullptr);
}

TEST_CASE("ImageContainerRegistry: ContainerCount", "[Graphics][ImageContainer]")
{
    REQUIRE(ImageContainerRegistry::ContainerCount() == 6);
}

// --- ComputeDataSize ---

TEST_CASE("ComputeDataSize: uncompressed formats", "[Graphics][Image]")
{
    REQUIRE(ComputeDataSize(16, 16, PixelFormat::RGBA8888) == 16 * 16 * 4);
    REQUIRE(ComputeDataSize(16, 16, PixelFormat::ARGB4444) == 16 * 16 * 2);
    REQUIRE(ComputeDataSize(16, 16, PixelFormat::P8) == 16 * 16 * 1);
}

TEST_CASE("ComputeDataSize: DXT formats", "[Graphics][Image]")
{
    // DXT1: 8 bytes per 4x4 block
    REQUIRE(ComputeDataSize(16, 16, PixelFormat::DXT1) == 4 * 4 * 8);
    // DXT5: 16 bytes per 4x4 block
    REQUIRE(ComputeDataSize(16, 16, PixelFormat::DXT5) == 4 * 4 * 16);
    // Non-power-of-2: rounds up
    REQUIRE(ComputeDataSize(5, 5, PixelFormat::DXT1) == 2 * 2 * 8);
}

// --- Image::Save TGA ---

TEST_CASE("Image: Save PAA fixture to TGA creates valid file", "[Graphics][Image][image]")
{
    auto img = Image::FromFile(GET_FIXTURE("texture/paa/synthetic_dxt1.paa"));
    REQUIRE(img.valid());

    REQUIRE(img.Save(GET_TEMP_FILE("test_save.tga")));

    auto loaded = Image::FromFile(GET_TEMP_FILE("test_save.tga"));
    REQUIRE(loaded.valid());
    REQUIRE(loaded.width() == img.width());
    REQUIRE(loaded.height() == img.height());
}

TEST_CASE("Image: PAA to TGA round-trip preserves dimensions", "[Graphics][Image][image]")
{
    auto paa = Image::FromFile(GET_FIXTURE("texture/paa/synthetic_dxt1.paa"));
    REQUIRE(paa.valid());

    REQUIRE(paa.Save(GET_TEMP_FILE("test_paa_to_tga.tga")));

    auto tga = Image::FromFile(GET_TEMP_FILE("test_paa_to_tga.tga"));
    REQUIRE(tga.valid());
    REQUIRE(tga.width() == paa.width());
    REQUIRE(tga.height() == paa.height());
}
