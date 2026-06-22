#pragma once
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <algorithm>
#include <vector>

namespace Poseidon
{
namespace BMPWriter
{

inline void SwapRGBtoBGR(uint8_t* pixels, int w, int h)
{
    for (int i = 0; i < w * h * 3; i += 3)
        std::swap(pixels[i], pixels[i + 2]);
}

inline bool WriteBMP(const char* path, int w, int h, const uint8_t* rgbPixels)
{
    int rowStride = (w * 3 + 3) & ~3;

    // Copy pixels and swap R<->B for BMP BGR format
    std::vector<uint8_t> pixels(rgbPixels, rgbPixels + w * h * 3);
    SwapRGBtoBGR(pixels.data(), w, h);

#pragma pack(push, 1)
    struct
    {
        uint16_t type;
        uint32_t size;
        uint16_t r1, r2;
        uint32_t offBits;
    } fh = {};
    struct
    {
        uint32_t size;
        int32_t w, h;
        uint16_t planes, bpp;
        uint32_t comp, imgSize;
        int32_t xppm, yppm;
        uint32_t clrUsed, clrImp;
    } ih = {};
#pragma pack(pop)

    fh.type = 0x4D42;
    fh.offBits = sizeof(fh) + sizeof(ih);
    fh.size = fh.offBits + rowStride * h;
    ih.size = sizeof(ih);
    ih.w = w;
    ih.h = h;
    ih.planes = 1;
    ih.bpp = 24;
    ih.imgSize = rowStride * h;

    FILE* f = fopen(path, "wb");
    if (!f)
        return false;
    fwrite(&fh, sizeof(fh), 1, f);
    fwrite(&ih, sizeof(ih), 1, f);
    std::vector<uint8_t> row(rowStride, 0);
    for (int y = 0; y < h; y++)
    {
        const uint8_t* src = pixels.data() + y * w * 3;
        memcpy(row.data(), src, w * 3);
        fwrite(row.data(), rowStride, 1, f);
    }
    fclose(f);
    return true;
}

} // namespace BMPWriter

} // namespace Poseidon
