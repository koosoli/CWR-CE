#pragma once
#include <Poseidon/Graphics/Shared/PNGWriter.hpp>
#include <Poseidon/Graphics/Shared/BMPWriter.hpp>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>
#include <Poseidon/Foundation/platform.hpp>

namespace Poseidon
{
namespace ScreenshotWriter
{

// Write screenshot in format determined by file extension.
// Input: top-down RGB (row 0 = top of image, standard D3D convention).
// .png  → PNG only
// .bmp  → BMP only
// other/none → both PNG and BMP (base path + .png / .bmp)
inline void WriteRGB(const char* path, int w, int h, const uint8_t* rgb)
{
    const char* ext = strrchr(path, '.');
    const char* lastSep = strrchr(path, '/');
    const char* lastBSep = strrchr(path, '\\');
    if (lastSep && ext < lastSep)
        ext = nullptr;
    if (lastBSep && ext < lastBSep)
        ext = nullptr;

    auto writePNG = [&](const char* p) { PNGWriter::WriteRGB(p, w, h, rgb); };

    // BMPWriter expects bottom-up row order; flip top-down input
    auto writeBMP = [&](const char* p)
    {
        int rowBytes = w * 3;
        std::vector<uint8_t> flipped(rgb, rgb + rowBytes * h);
        for (int y = 0; y < h / 2; y++)
        {
            uint8_t* top = flipped.data() + y * rowBytes;
            uint8_t* bot = flipped.data() + (h - 1 - y) * rowBytes;
            std::swap_ranges(top, top + rowBytes, bot);
        }
        BMPWriter::WriteBMP(p, w, h, flipped.data());
    };

    if (ext && _stricmp(ext, ".png") == 0)
    {
        writePNG(path);
    }
    else if (ext && _stricmp(ext, ".bmp") == 0)
    {
        writeBMP(path);
    }
    else
    {
        std::string base(path);
        writePNG((base + ".png").c_str());
        writeBMP((base + ".bmp").c_str());
    }
}

} // namespace ScreenshotWriter

} // namespace Poseidon
