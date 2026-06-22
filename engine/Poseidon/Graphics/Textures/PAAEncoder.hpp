#pragma once

#include <Poseidon/Graphics/Textures/Image.hpp>
#include <string>

namespace Poseidon
{

namespace PAAEncoder
{
// Write image as PAA file with specified pixel format.
// Mipmaps are auto-generated via downscaling.
// Supported formats: DXT1, DXT5, ARGB4444, ARGB1555, AI88.
bool WritePAA(const std::string& path, const Image& img, PixelFormat format);

// Write image as PAA using its current pixel format (must be PAA-compatible).
bool WritePAA(const std::string& path, const Image& img);
} // namespace PAAEncoder

} // namespace Poseidon
