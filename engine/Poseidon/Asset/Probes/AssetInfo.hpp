#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace Poseidon
{

struct TextureInfo
{
    std::string path;
    bool        isPaa              = false;
    std::string typeName;
    std::string formatName;
    uint16_t    magic              = 0;
    int         width              = 0;
    int         height             = 0;
    int         mipmapCount        = 0;
    int         paletteColors      = 0;
    bool        hasTransparentBlocks = false;
    bool        valid              = false;
};

TextureInfo InspectTexture(const std::string& path);

struct SoundInfo
{
    std::string path;
    std::string format;
    int         channels          = 0;
    int         sampleRate        = 0;
    int         bitDepth          = 0;
    double      duration          = 0.0;
    int         uncompressedSize  = 0;
    int64_t     fileSize          = 0;
    double      compressionRatio  = 0.0;
    bool        valid             = false;
};

SoundInfo InspectSound(const std::string& path);

struct ModelSectionInfo
{
    int         index         = 0;
    uint32_t    materialIndex = 0;
    std::string textureName;
    uint32_t    startTriangle = 0;
    uint32_t    triangleCount = 0;
    uint32_t    hints         = 0;
    std::string hintsStr;
};

struct ModelLodInfo
{
    int                                          index      = 0;
    float                                        resolution = 0.0f;
    std::string                                  name;
    int                                          points     = 0;
    int                                          faces      = 0;
    int                                          textures   = 0;
    int                                          selections = 0;
    std::vector<std::string>                     textureNames;
    std::vector<std::pair<std::string, int>>     selectionNames;
    std::vector<ModelSectionInfo>                sectionInfos;
};

struct ModelInfo
{
    std::string              path;
    std::string              format;
    int                      version     = 0;
    bool                     isSupported = true;
    std::string              errorMessage;
    int                      lodCount    = 0;
    std::vector<ModelLodInfo> lods;
    bool                     valid       = false;
};

ModelInfo InspectModel(const std::string& path);

struct TerrainInfo
{
    std::string              path;
    std::string              formatName;
    int                      gridX         = 0;
    int                      gridZ         = 0;
    float                    terrainX      = 0.0f;
    float                    terrainZ      = 0.0f;
    int                      heightmapSize = 0;
    float                    minHeight     = 0.0f;
    float                    maxHeight     = 0.0f;
    int                      textureCount  = 0;
    int                      objectCount   = 0;
    int                      objectNameCount = 0;
    std::vector<std::string> textureNames;
    bool                     valid         = false;
};

TerrainInfo InspectTerrain(const std::string& path);

struct PboFileEntry
{
    std::string name;
    long        length           = 0;
    long        time             = 0;
    bool        compressed       = false;
    int         uncompressedSize = 0;
};

struct PboInfo
{
    std::string              path;
    std::vector<PboFileEntry> entries;
    long                     totalSize   = 0;
    long                     totalStored = 0;
    bool                     valid       = false;
};

PboInfo InspectPbo(const std::string& path);

struct FontInfo
{
    std::string              path;
    std::string              name;
    int                      glyphCount      = 0;
    int                      maxHeight       = 0;
    int                      maxWidth        = 0;
    int                      textureSetCount = 0;
    int                      charCodeMin     = 0;
    int                      charCodeMax     = 0;
    std::vector<std::string> textureNames;
    std::vector<bool>        texturesExist;
    bool                     valid           = false;
};

FontInfo InspectFont(const std::string& path);

struct AnimationInfo
{
    std::string path;
    std::string format;
    int         boneCount  = 0;
    int         phaseCount = 0;
    float       stepX = 0, stepY = 0, stepZ = 0;
    bool        valid      = false;
};

AnimationInfo InspectAnimation(const std::string& path);

struct ConfigInfo
{
    std::string path;
    bool        isBinarized = false;
    int         version     = 0;
    int64_t     fileSize    = 0;
    bool        valid       = false;
};

ConfigInfo InspectConfig(const std::string& path);

struct AssetFileInfo
{
    std::string path;
    std::string name;
    int64_t     size         = 0;
    int64_t     modifiedTime = 0;
    std::string extension;
    bool        valid        = false;
};

AssetFileInfo InspectFile(const std::string& path);

std::string FormatTime(long timestamp);
std::string FormatSize(long size);

} // namespace Poseidon

using Poseidon::TextureInfo;
using Poseidon::SoundInfo;
using Poseidon::ModelInfo;
using Poseidon::TerrainInfo;
using Poseidon::PboFileEntry;
using Poseidon::PboInfo;
using Poseidon::FontInfo;
using Poseidon::AnimationInfo;
using Poseidon::ConfigInfo;
using Poseidon::AssetFileInfo;
using Poseidon::InspectTexture;
using Poseidon::InspectSound;
using Poseidon::InspectModel;
using Poseidon::InspectTerrain;
using Poseidon::InspectPbo;
using Poseidon::InspectFont;
using Poseidon::InspectAnimation;
using Poseidon::InspectConfig;
using Poseidon::InspectFile;
using Poseidon::FormatTime;
using Poseidon::FormatSize;
