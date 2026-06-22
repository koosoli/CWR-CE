#pragma once

#include <cstdint>

namespace Poseidon
{

// File envelope/container format, independent of pixel format (DXT1, RGBA, …).
enum class ImageContainer : uint8_t
{
    Unknown = 0,
    PAA, // BI PAA texture (DXT/ARGB/AI88)
    PAC, // BI PAC texture (P8 palette, DXT)
    PNG, // Standard PNG
    BMP, // Standard BMP
    TGA, // Standard TGA
    DDS, // DirectDraw Surface (DXT1/3/5, uncompressed RGBA)
    Count
};

struct ImageContainerInfo
{
    ImageContainer container;
    const char* name;
    const char* extension; // e.g. ".paa"
    const char* description;
    bool canRead;
    bool canWrite;
};

class ImageContainerRegistry
{
  public:
    static const ImageContainerInfo& Get(ImageContainer ct);
    static const ImageContainerInfo* FindByExtension(const char* ext);
    static const ImageContainerInfo* AllContainers();
    static int ContainerCount();
};

} // namespace Poseidon
