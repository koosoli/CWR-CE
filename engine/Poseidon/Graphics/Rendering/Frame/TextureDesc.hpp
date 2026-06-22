#pragma once

#include <cstdint>

// Texture description for the frame layer.  Pure data + pure
// validation helpers; no GL, no asset-loader dependencies.
//
// Locks down three contracts that all share the shape "texture upload
// data layout violates a rule":
//
//   - format encodes source-alpha presence (else DXT1 transparency is
//     lost to a wrong RGB-vs-RGBA format),
//   - the mip chain is complete and dimensions follow the format's
//     block alignment (else the sampler hits black or wrong colour),
//   - upload row pitch matches the destination stride, computed
//     per-mip-per-format (else small DXT mips show black stripes).
//
// All three concern the texture *description* — independent of the
// backend that consumes it.  Pure helpers let unit tests pin them, so
// every backend's upload path uses the same source of truth.


namespace Poseidon
{
namespace render::frame
{

// Typed pixel format.  Each variant uniquely encodes:
//   - bytes per pixel (or per 4x4 block for compressed)
//   - whether alpha is present in the format
//   - whether the format is block-compressed
//
// Reusing one format for "RGB" and "RGB-with-alpha" loses source alpha
// silently; this enum makes the distinction structural.
enum class PixelFormat : std::uint8_t
{
    RGBA8,        // 4 bpp, alpha
    RGB8,         // 3 bpp, no alpha
    BGRA8,        // 4 bpp, alpha (D3D-style component order)
    R8,           // 1 bpp, no alpha
    DXT1,         // 8 bytes / 4x4 block, no alpha
    DXT1A,        // 8 bytes / 4x4 block, 1-bit alpha
    DXT3,         // 16 bytes / 4x4 block, 4-bit alpha
    DXT5,         // 16 bytes / 4x4 block, interpolated alpha
};

// Block dimensions in pixels.  Compressed formats are 4x4.
constexpr int BlockWidth(PixelFormat f)
{
    switch (f)
    {
        case PixelFormat::DXT1:
        case PixelFormat::DXT1A:
        case PixelFormat::DXT3:
        case PixelFormat::DXT5: return 4;
        default:                return 1;
    }
}

// Bytes per element (pixel for uncompressed, 4x4 block for compressed).
constexpr int BytesPerElement(PixelFormat f)
{
    switch (f)
    {
        case PixelFormat::RGBA8: return 4;
        case PixelFormat::RGB8:  return 3;
        case PixelFormat::BGRA8: return 4;
        case PixelFormat::R8:    return 1;
        case PixelFormat::DXT1:  return 8;   // 8 bytes per 4x4 block
        case PixelFormat::DXT1A: return 8;
        case PixelFormat::DXT3:  return 16;
        case PixelFormat::DXT5:  return 16;
    }
    return 0;
}

constexpr bool HasAlpha(PixelFormat f)
{
    switch (f)
    {
        case PixelFormat::RGBA8:
        case PixelFormat::BGRA8:
        case PixelFormat::DXT1A:
        case PixelFormat::DXT3:
        case PixelFormat::DXT5: return true;
        default:                return false;
    }
}

constexpr bool IsCompressed(PixelFormat f)
{
    return BlockWidth(f) > 1;
}

// Row stride in bytes for `widthPixels` of the given format.
// Compressed formats: 1 row of 4x4 blocks = ceil(width/4) * bytesPerBlock.
constexpr int RowStride(PixelFormat f, int widthPixels)
{
    if (widthPixels <= 0)
        return 0;
    const int bw     = BlockWidth(f);
    const int bpe    = BytesPerElement(f);
    const int blocks = (widthPixels + bw - 1) / bw;
    return blocks * bpe;
}

// Mip-level dimension: max(1, base >> level).  Clamps level into
// the range where the shift is defined (UB at level >= bit width
// of int).  Standard convention for both compressed and
// uncompressed.
constexpr int MipDim(int baseDim, int level)
{
    if (level <  0)  level = 0;
    if (level >= 31) return 1;          // any shift by >=31 → 0 or UB
    int d = baseDim >> level;
    return d < 1 ? 1 : d;
}

// Maximum well-formed mip count for a given base dimension.
// Stops at 1x1.  For width=64: 64,32,16,8,4,2,1 → 7 levels.
constexpr int MaxMipCount(int width, int height)
{
    int maxD = (width > height) ? width : height;
    int count = 1;
    while (maxD > 1)
    {
        maxD >>= 1;
        ++count;
    }
    return count;
}

// Full TextureDesc — backend-agnostic.  Backends consume this via
// Execute; tests construct it directly and exercise validation.
struct TextureDesc
{
    int          width      = 0;
    int          height     = 0;
    int          mipCount   = 1;    // number of stored mip levels
    PixelFormat  format     = PixelFormat::RGBA8;
    bool         sourceHasAlpha = false;  // ← input fact, not derived
};

// Validation result for TextureDesc.  Returns nullptr if valid,
// else a static C string explaining the violation.  Pure.
inline const char* ValidateTextureDesc(const TextureDesc& d)
{
    if (d.width  <= 0)                            return "width must be > 0";
    if (d.height <= 0)                            return "height must be > 0";
    if (d.mipCount < 1)                           return "mipCount must be >= 1";

    // Mip chain complete and within bounds.
    const int maxMips = MaxMipCount(d.width, d.height);
    if (d.mipCount > maxMips)
        return "mipCount exceeds max for base dimensions";

    // For compressed formats, base dimensions must be multiples of the
    // block size, otherwise the upload's row stride is meaningless.
    if (IsCompressed(d.format))
    {
        const int bw = BlockWidth(d.format);
        if (d.width  % bw != 0) return "compressed format: width not block-aligned";
        if (d.height % bw != 0) return "compressed format: height not block-aligned";
    }

    // Source alpha vs format alpha alignment.  An alpha-less format for
    // source-with-alpha drops data; the reverse wastes bytes but is
    // correct.
    if (d.sourceHasAlpha && !HasAlpha(d.format))
        return "format drops source alpha (B-014 class)";

    return nullptr;
}

} // namespace render::frame

} // namespace Poseidon
