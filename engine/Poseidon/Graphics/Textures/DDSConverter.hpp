#pragma once

#include <Poseidon/Graphics/Textures/PixelFormat.hpp>
#include <cstdint>
#include <string>
#include <vector>

namespace Poseidon
{

struct DDSMipLevel
{
    int width = 0;
    int height = 0;
    std::vector<uint8_t> data;
};

struct DDSFile
{
    PixelFormat format = PixelFormat::Unknown;
    int width = 0;
    int height = 0;
    std::vector<DDSMipLevel> mipmaps; // [0] = largest

    bool valid() const { return width > 0 && height > 0 && !mipmaps.empty(); }
};

// PAA<->DDS: DXT1/DXT5 blocks transfer verbatim (lossless); other formats
// round-trip through an RGBA8888 intermediate.
namespace DDSConverter
{
// Read DDS file from disk
DDSFile ReadDDS(const std::string& path);

// Read DDS from memory buffer
DDSFile ReadDDSBuffer(const void* data, size_t size);

// Write DDS file to disk
bool WriteDDS(const std::string& path, const DDSFile& dds);

// Write DDS to memory buffer
std::vector<uint8_t> WriteDDSBuffer(const DDSFile& dds);

// Convert PAA/PAC file to DDS (raw block copy for DXT, decode+re-store for others)
DDSFile PAAFileToDDS(const std::string& paaPath);

} // namespace DDSConverter

} // namespace Poseidon
