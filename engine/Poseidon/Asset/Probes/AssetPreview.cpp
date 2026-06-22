#include <Poseidon/Asset/Probes/AssetPreview.hpp>
#include <Poseidon/Graphics/Textures/Image.hpp>
#include <Poseidon/Graphics/Textures/PAADecoder.hpp>
#include <Poseidon/Graphics/Textures/ModelRenderer.hpp>
#include <Poseidon/Graphics/Shared/PNGWriter.hpp>
#include <Poseidon/Graphics/Shared/BMPWriter.hpp>
#include <Poseidon/Graphics/Shared/TGAWriter.hpp>
#include <Poseidon/World/Model/ModelCache.hpp>
#include <Poseidon/World/Model/Model.hpp>
#include <Poseidon/World/Model/ShapeAdapter.hpp>
#include <Poseidon/Graphics/Rendering/Shape/Shape.hpp>
#include <Poseidon/Graphics/Rendering/Draw/FontData.hpp>
#include <Poseidon/World/Terrain/WrpReader.hpp>

#include <algorithm>
#include <filesystem>
#include <map>
#include <ctype.h>
#include <stdio.h>
#include <memory>
#include <set>

namespace Poseidon
{

// Save helpers

static std::string toLowerExt(const std::string& path)
{
    auto ext = std::filesystem::path(path).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext;
}

bool PreviewImage::saveToFile(const std::string& path) const
{
    if (!valid())
        return false;
    auto ext = toLowerExt(path);
    if (ext == ".png")
        return PNGWriter::WriteRGBA(path.c_str(), width, height, data.data());
    if (ext == ".bmp")
        return BMPWriter::WriteBMP(path.c_str(), width, height, data.data());
    if (ext == ".tga")
        return TGAWriter::WriteTGA(path.c_str(), width, height, 4, data.data());
    return false;
}

bool PreviewImageRGB::saveToFile(const std::string& path) const
{
    if (!valid())
        return false;
    auto ext = toLowerExt(path);
    if (ext == ".png")
        return PNGWriter::WriteRGB(path.c_str(), width, height, data.data());
    if (ext == ".bmp")
        return BMPWriter::WriteBMP(path.c_str(), width, height, data.data());
    if (ext == ".tga")
        return TGAWriter::WriteTGA(path.c_str(), width, height, 3, data.data());
    return false;
}

// Texture preview

PreviewImage PreviewTexture(const std::string& path)
{
    PreviewImage result;

    auto img = Image::FromFile(path);
    if (!img.valid())
        return result;

    auto rgba = img.ToRGBA();
    if (!rgba.valid())
        return result;

    result.width = rgba.width();
    result.height = rgba.height();
    result.data = rgba.data();
    return result;
}

PreviewImage PreviewTextureMip(const std::string& path, int mipLevel)
{
    if (mipLevel == 0)
        return PreviewTexture(path);

    PreviewImage result;
    auto decoded = DecodePAAFileMip(path, mipLevel);
    if (decoded.rgba.empty())
        return result;

    result.width = decoded.width;
    result.height = decoded.height;
    result.data = std::move(decoded.rgba);
    return result;
}

// Terrain heightmap preview

PreviewImage PreviewTerrain(const std::string& path)
{
    PreviewImage result;

    WrpReader reader;
    if (!reader.Load(path.c_str()))
        return result;

    int w = reader.GetGridX();
    int h = reader.GetGridZ();
    float minH = reader.GetMinHeight();
    float maxH = reader.GetMaxHeight();
    float range = maxH - minH;
    if (range < 0.001f)
        range = 1.0f;

    result.width = w;
    result.height = h;
    result.data.resize(w * h * 4);
    const float* heights = reader.GetHeightmapData();

    for (int z = 0; z < h; z++)
    {
        for (int x = 0; x < w; x++)
        {
            float t = (heights[(h - 1 - z) * w + x] - minH) / range;
            if (t < 0.0f)
                t = 0.0f;
            if (t > 1.0f)
                t = 1.0f;

            uint8_t r, g, b;
            if (t < 0.01f)
            {
                r = 30;
                g = 60;
                b = 180;
            }
            else if (t < 0.3f)
            {
                float s = (t - 0.01f) / 0.29f;
                r = (uint8_t)(30 + s * 100);
                g = (uint8_t)(120 + s * 60);
                b = (uint8_t)(30 + s * 20);
            }
            else if (t < 0.7f)
            {
                float s = (t - 0.3f) / 0.4f;
                r = (uint8_t)(130 + s * 60);
                g = (uint8_t)(180 - s * 60);
                b = (uint8_t)(50 - s * 20);
            }
            else
            {
                float s = (t - 0.7f) / 0.3f;
                r = (uint8_t)(190 + s * 65);
                g = (uint8_t)(120 + s * 135);
                b = (uint8_t)(30 + s * 225);
            }

            int idx = (z * w + x) * 4;
            result.data[idx + 0] = r;
            result.data[idx + 1] = g;
            result.data[idx + 2] = b;
            result.data[idx + 3] = 255;
        }
    }

    return result;
}

// Model wireframe preview

PreviewImageRGB PreviewModel(const std::string& path, const ModelPreviewOptions& opts)
{
    PreviewImageRGB result;

    // Use modern P3D loaders (x64-safe, supports both ODOL and MLOD)
    Poseidon::ModelCache cache;
    auto model = cache.load(path);
    if (!model)
        return result;

    // Convert the model to a Shape for the wireframe renderer
    auto* shape = Poseidon::Model::ShapeAdapter::convertToLODShape(*model);
    if (!shape)
        return result;

    if (opts.lodIndex >= shape->NLevels())
    {
        delete shape;
        return result;
    }

    Shape* lod = shape->Level(opts.lodIndex);
    if (!lod)
    {
        delete shape;
        return result;
    }

    auto rendered = RenderModelWireframe(lod, opts.width, opts.height, opts.view, opts.bgR, opts.bgG, opts.bgB);
    if (!rendered.valid())
    {
        delete shape;
        return result;
    }

    result.data = std::move(rendered.rgb);
    result.width = opts.width;
    result.height = opts.height;
    delete shape;
    return result;
}

// Font preview

// Core renderer: takes parsed FXY data and pre-loaded textures
static PreviewImage RenderFontGlyphs(const FXYData& fxy, const std::map<int, PreviewImage>& textures,
                                     const FontPreviewOptions& opts)
{
    PreviewImage result;

    if (textures.empty() || !fxy.valid())
        return result;

    // Generate text to render
    std::string text;
    if (opts.charmap)
    {
        // All 224 characters in rows of 32
        for (int i = 0; i < fxy.nChars; i++)
        {
            text += static_cast<char>(static_cast<unsigned char>(i + 32));
            if ((i + 1) % 32 == 0 && i + 1 < fxy.nChars)
                text += '\n';
        }
    }
    else
    {
        text = opts.sampleText;
    }
    int lineHeight = fxy.maxHeight + 2; // +2 for line spacing
    int curX = 0, maxLineW = 0;
    int numLines = 1;
    for (char c : text)
    {
        if (c == '\n')
        {
            if (curX > maxLineW)
                maxLineW = curX;
            curX = 0;
            numLines++;
            continue;
        }
        int idx = static_cast<unsigned char>(c) - 32; // StartChar=32
        if (idx >= 0 && idx < fxy.nChars)
            curX += fxy.glyphs[static_cast<size_t>(idx)].w + 1;
        else
            curX += fxy.maxWidth / 2; // fallback width for unmapped chars
    }
    if (curX > maxLineW)
        maxLineW = curX;

    int outW = maxLineW + 4; // small padding
    int outH = numLines * lineHeight + 4;
    if (outW <= 0 || outH <= 0)
        return result;

    // Create output buffer with background color
    result.width = outW;
    result.height = outH;
    result.data.resize(static_cast<size_t>(outW * outH * 4));
    for (int i = 0; i < outW * outH; i++)
    {
        result.data[static_cast<size_t>(i * 4 + 0)] = opts.bgR;
        result.data[static_cast<size_t>(i * 4 + 1)] = opts.bgG;
        result.data[static_cast<size_t>(i * 4 + 2)] = opts.bgB;
        result.data[static_cast<size_t>(i * 4 + 3)] = 255;
    }

    // Render sample text
    int drawX = 2, drawY = 2;
    for (char c : text)
    {
        if (c == '\n')
        {
            drawX = 2;
            drawY += lineHeight;
            continue;
        }

        int idx = static_cast<unsigned char>(c) - 32;
        if (idx < 0 || idx >= fxy.nChars)
        {
            drawX += fxy.maxWidth / 2;
            continue;
        }

        const FXYGlyph& glyph = fxy.glyphs[static_cast<size_t>(idx)];
        if (glyph.w <= 0 || glyph.h <= 0)
        {
            drawX += glyph.w > 0 ? glyph.w + 1 : fxy.maxWidth / 2;
            continue;
        }

        auto it = textures.find(glyph.setNum);
        if (it == textures.end())
        {
            drawX += glyph.w + 1;
            continue;
        }

        const PreviewImage& tex = it->second;

        // Blit glyph from texture atlas to output
        for (int gy = 0; gy < glyph.h && (drawY + gy) < outH; gy++)
        {
            int srcY = glyph.y + gy;
            if (srcY >= tex.height)
                break;
            for (int gx = 0; gx < glyph.w && (drawX + gx) < outW; gx++)
            {
                int srcX = glyph.x + gx;
                if (srcX >= tex.width)
                    break;

                size_t srcIdx = static_cast<size_t>((srcY * tex.width + srcX) * 4);
                // RGBA from PAA decoder — use alpha as glyph mask
                uint8_t alpha = tex.data[srcIdx + 3];
                if (alpha == 0)
                    continue;

                size_t dstIdx = static_cast<size_t>(((drawY + gy) * outW + (drawX + gx)) * 4);
                if (alpha >= 255)
                {
                    result.data[dstIdx + 0] = opts.fgR;
                    result.data[dstIdx + 1] = opts.fgG;
                    result.data[dstIdx + 2] = opts.fgB;
                    result.data[dstIdx + 3] = 255;
                }
                else
                {
                    // Alpha blend
                    uint8_t bgR = result.data[dstIdx + 0];
                    uint8_t bgG = result.data[dstIdx + 1];
                    uint8_t bgB = result.data[dstIdx + 2];
                    result.data[dstIdx + 0] = static_cast<uint8_t>((opts.fgR * alpha + bgR * (255 - alpha)) / 255);
                    result.data[dstIdx + 1] = static_cast<uint8_t>((opts.fgG * alpha + bgG * (255 - alpha)) / 255);
                    result.data[dstIdx + 2] = static_cast<uint8_t>((opts.fgB * alpha + bgB * (255 - alpha)) / 255);
                    result.data[dstIdx + 3] = 255;
                }
            }
        }

        drawX += glyph.w + 1;
    }

    return result;
}

PreviewImage PreviewFont(const std::string& fxyPath, const FontPreviewOptions& opts)
{
    PreviewImage result;

    // Parse FXY
    QIFStream stream;
    stream.open(fxyPath.c_str());
    if (stream.rest() <= 0)
        return result;

    std::filesystem::path p(fxyPath);
    std::string baseName = p.stem().string();
    FXYData fxy = ParseFXY(stream, baseName.c_str());
    if (!fxy.valid())
        return result;

    std::string dir = p.parent_path().string();
    if (!dir.empty())
        dir += "/";

    std::map<int, PreviewImage> textures;
    for (int setNum : fxy.textureSetNums)
    {
        char texName[256];
        // Use original stem case for texture names (Linux is case-sensitive)
        snprintf(texName, sizeof(texName), "%s-%02d.paa", baseName.c_str(), setNum);
        std::string texPath = dir + texName;
        auto tex = PreviewTexture(texPath);
        if (tex.valid())
            textures[setNum] = std::move(tex);
    }

    return RenderFontGlyphs(fxy, textures, opts);
}

PreviewImage PreviewFontFromData(const void* fxyData, size_t fxySize, const std::string& fontName,
                                 const std::vector<std::pair<int, std::pair<const void*, size_t>>>& textureSets,
                                 const FontPreviewOptions& opts)
{
    PreviewImage result;

    // Parse FXY from memory
    QIStream stream(static_cast<const char*>(fxyData), static_cast<int>(fxySize));
    FXYData fxy = ParseFXY(stream, fontName.c_str());
    if (!fxy.valid())
        return result;

    // Decode textures from memory buffers
    std::map<int, PreviewImage> textures;
    for (const auto& [setNum, bufInfo] : textureSets)
    {
        auto decoded = DecodePAABuffer(bufInfo.first, bufInfo.second, true);
        if (!decoded.rgba.empty())
        {
            PreviewImage tex;
            tex.width = decoded.width;
            tex.height = decoded.height;
            tex.data = std::move(decoded.rgba);
            textures[setNum] = std::move(tex);
        }
    }

    return RenderFontGlyphs(fxy, textures, opts);
}

} // namespace Poseidon
