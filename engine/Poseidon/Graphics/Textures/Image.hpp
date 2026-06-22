#pragma once

#include <Poseidon/Graphics/Textures/PixelFormat.hpp>
#include <Poseidon/Graphics/Textures/ImageContainer.hpp>
#include <vector>
#include <string>
#include <cstdint>

namespace Poseidon
{

// Central data type for the texture pipeline: holds pixel data in any
// PixelFormat and converts between formats via an RGBA8888 intermediate.
class Image
{
  public:
    Image() = default;
    Image(int width, int height, PixelFormat format, std::vector<uint8_t> data);

    static Image FromRGBA(int width, int height, std::vector<uint8_t> rgba);
    static Image FromFile(const std::string& path);

    // Pixel format conversion (via RGBA8888 intermediate)
    Image ConvertTo(PixelFormat target) const;
    Image ToRGBA() const;

    // Container I/O
    bool Save(const std::string& path) const;
    bool Save(const std::string& path, ImageContainer ct) const;

    bool valid() const { return _width > 0 && _height > 0 && !_data.empty(); }
    int width() const { return _width; }
    int height() const { return _height; }
    PixelFormat format() const { return _format; }
    const std::vector<uint8_t>& data() const { return _data; }
    size_t dataSize() const { return _data.size(); }

  private:
    std::vector<uint8_t> _data;
    int _width = 0;
    int _height = 0;
    PixelFormat _format = PixelFormat::Unknown;
};

// Pixel format conversion functions (low-level, used by Image)
bool ConvertPixels(const void* src, void* dst, int width, int height, PixelFormat srcFmt, PixelFormat dstFmt);

// Compute expected data size for given dimensions and format
size_t ComputeDataSize(int width, int height, PixelFormat fmt);

} // namespace Poseidon
