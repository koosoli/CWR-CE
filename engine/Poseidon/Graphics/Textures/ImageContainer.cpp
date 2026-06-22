#include <Poseidon/Graphics/Textures/ImageContainer.hpp>
#include <cstring>
#include <cctype>

namespace Poseidon
{

// clang-format off
static constexpr ImageContainerInfo kContainers[] = {
    {ImageContainer::PAA, "PAA", ".paa", "BI PAA texture (DXT1/3/5, ARGB4444/1555, AI88)",  true,  true},
    {ImageContainer::PAC, "PAC", ".pac", "BI PAC texture (P8 palette, read-only: needs color quantization to write)", true, false},
    {ImageContainer::PNG, "PNG", ".png", "PNG image (always RGBA8888)",  true,  true},
    {ImageContainer::BMP, "BMP", ".bmp", "BMP image (always RGBA8888)",  true,  true},
    {ImageContainer::TGA, "TGA", ".tga", "TGA image (always RGBA8888)",  true,  true},
    {ImageContainer::DDS, "DDS", ".dds", "DDS texture (DXT1/3/5, RGBA8888)", true, true},
};
// clang-format on

static constexpr int kContainerCount = static_cast<int>(sizeof(kContainers) / sizeof(kContainers[0]));

static const ImageContainerInfo kUnknown = {ImageContainer::Unknown, "Unknown", "", "Unknown container", false, false};

static bool iequals(const char* a, const char* b)
{
    for (; *a && *b; ++a, ++b)
    {
        if (std::tolower(static_cast<unsigned char>(*a)) != std::tolower(static_cast<unsigned char>(*b)))
            return false;
    }
    return *a == *b;
}

const ImageContainerInfo& ImageContainerRegistry::Get(ImageContainer ct)
{
    for (int i = 0; i < kContainerCount; ++i)
    {
        if (kContainers[i].container == ct)
            return kContainers[i];
    }
    return kUnknown;
}

const ImageContainerInfo* ImageContainerRegistry::FindByExtension(const char* ext)
{
    if (!ext)
        return nullptr;
    for (int i = 0; i < kContainerCount; ++i)
    {
        if (iequals(kContainers[i].extension, ext))
            return &kContainers[i];
    }
    return nullptr;
}

const ImageContainerInfo* ImageContainerRegistry::AllContainers()
{
    return kContainers;
}

int ImageContainerRegistry::ContainerCount()
{
    return kContainerCount;
}

} // namespace Poseidon
