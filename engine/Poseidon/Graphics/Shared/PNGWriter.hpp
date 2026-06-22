#pragma once
#include <cstdint>
#include <string>

namespace Poseidon
{
namespace PNGWriter
{

bool WritePNG(const char* path, int w, int h, int channels, const uint8_t* pixels);

// RGBA convenience
inline bool WriteRGBA(const char* path, int w, int h, const uint8_t* rgba)
{
    return WritePNG(path, w, h, 4, rgba);
}

// RGB convenience
inline bool WriteRGB(const char* path, int w, int h, const uint8_t* rgb)
{
    return WritePNG(path, w, h, 3, rgb);
}

} // namespace PNGWriter

} // namespace Poseidon
