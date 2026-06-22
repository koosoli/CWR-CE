#include <Poseidon/Graphics/Textures/PAAEncoder.hpp>
#include <Poseidon/Graphics/Textures/DXTCompressor.hpp>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <stdint.h>
#include <utility>
#include <vector>

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image_resize2.h>
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

namespace Poseidon
{

namespace PAAEncoder
{

static uint16_t paaMagicForFormat(PixelFormat fmt)
{
    const auto& info = PixelFormatRegistry::Get(fmt);
    return info.paaMagic;
}

static void writeU16(FILE* f, uint16_t v)
{
    fwrite(&v, 2, 1, f);
}

static void writeU24(FILE* f, uint32_t v)
{
    uint8_t buf[3] = {static_cast<uint8_t>(v & 0xFF), static_cast<uint8_t>((v >> 8) & 0xFF),
                      static_cast<uint8_t>((v >> 16) & 0xFF)};
    fwrite(buf, 3, 1, f);
}

// Generate mipmap levels from RGBA source using stb_image_resize2
static std::vector<Image> generateMipmaps(const Image& rgba, int maxLevels)
{
    std::vector<Image> levels;
    levels.push_back(rgba);

    int w = rgba.width();
    int h = rgba.height();

    for (int level = 1; level < maxLevels && (w > 1 || h > 1); ++level)
    {
        int nw = std::max(w / 2, 1);
        int nh = std::max(h / 2, 1);

        std::vector<uint8_t> resized(nw * nh * 4);
        stbir_resize_uint8_linear(levels.back().data().data(), w, h, w * 4, resized.data(), nw, nh, nw * 4, STBIR_RGBA);

        levels.push_back(Image::FromRGBA(nw, nh, std::move(resized)));
        w = nw;
        h = nh;
    }

    return levels;
}

// Encode a single mipmap level's pixel data in the target format
static std::vector<uint8_t> encodeLevel(const Image& rgbaLevel, PixelFormat fmt)
{
    int w = rgbaLevel.width();
    int h = rgbaLevel.height();

    if (fmt == PixelFormat::DXT1)
        return DXTCompressor::CompressDXT1(rgbaLevel.data().data(), w, h);
    if (fmt == PixelFormat::DXT3)
        return DXTCompressor::CompressDXT3(rgbaLevel.data().data(), w, h);
    if (fmt == PixelFormat::DXT5)
        return DXTCompressor::CompressDXT5(rgbaLevel.data().data(), w, h);

    // Uncompressed: convert pixels
    size_t dstSize = ComputeDataSize(w, h, fmt);
    std::vector<uint8_t> encoded(dstSize);
    ConvertPixels(rgbaLevel.data().data(), encoded.data(), w, h, PixelFormat::RGBA8888, fmt);
    return encoded;
}

bool WritePAA(const std::string& path, const Image& img, PixelFormat format)
{
    if (!img.valid())
        return false;

    uint16_t magic = paaMagicForFormat(format);
    if (magic == 0)
        return false; // Format not supported as PAA

    // Convert to RGBA first if needed
    Image rgba = img.ToRGBA();
    if (!rgba.valid())
        return false;

    // Generate mipmaps (up to 8 levels, matching engine expectations)
    auto mipmaps = generateMipmaps(rgba, 8);

    FILE* f = fopen(path.c_str(), "wb");
    if (!f)
        return false;

    // 1. Format magic word
    writeU16(f, magic);

    // 2. No TAGG sections (minimal PAA — valid without them)

    // 3. Palette (0 = no palette for non-P8 formats)
    writeU16(f, 0);

    // 4. Mipmap levels
    for (const auto& level : mipmaps)
    {
        auto encoded = encodeLevel(level, format);
        writeU16(f, static_cast<uint16_t>(level.width()));
        writeU16(f, static_cast<uint16_t>(level.height()));
        writeU24(f, static_cast<uint32_t>(encoded.size()));
        fwrite(encoded.data(), 1, encoded.size(), f);
    }

    // 5. Terminator
    writeU16(f, 0);
    writeU16(f, 0);

    fclose(f);
    return true;
}

bool WritePAA(const std::string& path, const Image& img)
{
    if (!img.valid())
        return false;

    PixelFormat fmt = img.format();
    // If image is RGBA8888, default to DXT5 (most common PAA format with alpha)
    if (fmt == PixelFormat::RGBA8888)
        fmt = PixelFormat::DXT5;

    return WritePAA(path, img, fmt);
}

} // namespace PAAEncoder

} // namespace Poseidon
