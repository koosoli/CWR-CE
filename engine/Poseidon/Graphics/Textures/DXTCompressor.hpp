#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>

namespace Poseidon
{

namespace DXTCompressor
{
// Compress RGBA8888 image to DXT1 (BC1). Returns compressed data.
// Width and height must be multiples of 4 (padded internally if not).
std::vector<uint8_t> CompressDXT1(const uint8_t* rgba, int width, int height);

// Compress RGBA8888 image to DXT3 (BC2). Returns compressed data.
std::vector<uint8_t> CompressDXT3(const uint8_t* rgba, int width, int height);

// Compress RGBA8888 image to DXT5 (BC3). Returns compressed data.
std::vector<uint8_t> CompressDXT5(const uint8_t* rgba, int width, int height);

// Expected compressed data size
size_t CompressedSize(int width, int height, bool isDXT1);
} // namespace DXTCompressor

} // namespace Poseidon
