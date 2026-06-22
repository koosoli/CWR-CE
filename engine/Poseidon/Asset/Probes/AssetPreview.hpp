#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace Poseidon
{

struct PreviewImage
{
    std::vector<uint8_t> data;
    int                  width    = 0;
    int                  height   = 0;
    int                  channels = 4; // RGBA

    bool valid() const { return !data.empty() && width > 0 && height > 0; }

    bool saveToFile(const std::string& path) const;
};

PreviewImage PreviewTexture(const std::string& path);
PreviewImage PreviewTextureMip(const std::string& path, int mipLevel);
PreviewImage PreviewTerrain(const std::string& path);

struct ModelPreviewOptions
{
    int         width  = 512;
    int         height = 512;
    int         lodIndex = 0;
    std::string view   = "front";
    uint8_t     bgR = 24, bgG = 24, bgB = 24;
};

struct PreviewImageRGB
{
    std::vector<uint8_t> data;
    int                  width    = 0;
    int                  height   = 0;
    int                  channels = 3; // RGB

    bool valid() const { return !data.empty() && width > 0 && height > 0; }

    bool saveToFile(const std::string& path) const;
};

PreviewImageRGB PreviewModel(const std::string& path, const ModelPreviewOptions& opts = {});

struct FontPreviewOptions
{
    std::string sampleText = "ABCDEFGHIJ\nabcdefghij\n0123456789\n!@#$%^&*()";
    bool        charmap    = false;
    uint8_t     bgR = 24, bgG = 24, bgB = 24;
    uint8_t     fgR = 255, fgG = 255, fgB = 255;
};

PreviewImage PreviewFont(const std::string& fxyPath, const FontPreviewOptions& opts = {});
PreviewImage PreviewFontFromData(const void* fxyData, size_t fxySize, const std::string& fontName,
                                 const std::vector<std::pair<int, std::pair<const void*, size_t>>>& textureSets,
                                 const FontPreviewOptions& opts = {});

} // namespace Poseidon
