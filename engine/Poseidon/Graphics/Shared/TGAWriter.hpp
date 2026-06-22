#pragma once
#include <cstdint>

namespace Poseidon
{
namespace TGAWriter
{

bool WriteTGA(const char* path, int w, int h, int channels, const uint8_t* pixels);

inline bool WriteRGBA(const char* path, int w, int h, const uint8_t* rgba)
{
    return WriteTGA(path, w, h, 4, rgba);
}

} // namespace TGAWriter

} // namespace Poseidon
