#pragma once

#include <vector>
#include <cstdint>
#include <string>

namespace Poseidon
{

struct DecodedImage
{
    std::vector<uint8_t> rgba;
    int width = 0;
    int height = 0;
    bool valid() const { return width > 0 && height > 0 && !rgba.empty(); }
};

struct PAAInfo
{
    std::string path;
    uint16_t magic = 0;
    bool isPaa = false;
    int width = 0;
    int height = 0;
    int mipmapCount = 0;
    int paletteColors = 0;
    const char* formatName = nullptr;
    bool hasTransparentBlocks = false; // DXT1: any blocks with c0 <= c1 (1-bit alpha mode)
};

// Read metadata without decoding pixel data
bool ReadPAAInfo(const std::string& path, PAAInfo& info);

// Decode PAA/PAC file to RGBA8888 pixel buffer
DecodedImage DecodePAAFile(const std::string& path);

// Decode specific mipmap level
DecodedImage DecodePAAFileMip(const std::string& path, int mipLevel);

// Decode from memory buffer (for embedded/archive use)
DecodedImage DecodePAABuffer(const void* data, size_t size, bool isPaa);

// Three-way alpha classification of a decoded RGBA8888 buffer. This is the
// per-texture signal a section-sort renderer needs (ArmA1-style): only a Blend
// texture (partial-alpha texels present) must be deferred to the back-to-front
// pass; Opaque and Cutout occlude (write depth, alpha-test the holes).
struct AlphaStats
{
    enum Kind
    {
        Opaque, // alpha essentially all 255
        Cutout, // fully-transparent holes (a=0), no meaningful partial alpha
        Blend   // partial-alpha texels (0 < a < 255) present
    };
    Kind kind = Opaque;
    int aMin = 255, aMax = 255, aMean = 255;
    double pctClear = 0.0, pctOpaque = 100.0, pctPartial = 0.0; // % of texels a=0 / a=255 / 0<a<255
};

// Classify the alpha channel of an RGBA8888 buffer (4 bytes/texel, alpha = byte 3).
// `partialThreshold`/`clearThreshold` are the percentages above which partial / clear
// texels are considered significant (default 2.0%, tolerating AA-edge noise).
AlphaStats ClassifyAlpha(const uint8_t* rgba, size_t pixelCount, double partialThreshold = 2.0,
                         double clearThreshold = 2.0);

const char* AlphaKindName(AlphaStats::Kind kind);

// Decide a texture's alpha class from its cheap header flags + format, deferring
// to a full decoded-alpha scan only when the format can carry partial alpha. This
// is the tiering policy a section-sort renderer uses at texture-load time:
//   - no alpha channel        -> Cutout if chroma-keyed, else Opaque   (no decode)
//   - 1-bit-alpha format       -> Cutout (punch-through holes only)      (no decode)
//   - multi-bit-alpha format   -> `decoded` verdict (caller must decode + ClassifyAlpha)
// `decoded` is null when the caller has not (or could not) decode; the safe fallback
// is Opaque (preserves occlusion / batching — never routes an unknown to the blend pass).
AlphaStats::Kind ClassifyTextureAlpha(bool hasAlpha, bool isChromaKey, bool oneBitAlphaFormat,
                                      const AlphaStats* decoded);

} // namespace Poseidon
