#include <Poseidon/Graphics/Textures/DDSConverter.hpp>
#include <Poseidon/Graphics/Textures/PAADecoder.hpp>
#include <Poseidon/Graphics/Textures/DXTCompressor.hpp>
#include <fstream>
#include <cstring>
#include <algorithm>
#include <utility>

namespace Poseidon
{

// DDS structures (Microsoft spec)
static constexpr uint32_t DDS_MAGIC = 0x20534444; // "DDS "

struct DDS_PIXELFORMAT
{
    uint32_t size;
    uint32_t flags;
    uint32_t fourCC;
    uint32_t rgbBitCount;
    uint32_t rBitMask;
    uint32_t gBitMask;
    uint32_t bBitMask;
    uint32_t aBitMask;
};

static constexpr uint32_t DDPF_ALPHAPIXELS = 0x1;
static constexpr uint32_t DDPF_FOURCC = 0x4;
static constexpr uint32_t DDPF_RGB = 0x40;

struct DDS_HEADER
{
    uint32_t size;
    uint32_t flags;
    uint32_t height;
    uint32_t width;
    uint32_t pitchOrLinearSize;
    uint32_t depth;
    uint32_t mipMapCount;
    uint32_t reserved1[11];
    DDS_PIXELFORMAT ddspf;
    uint32_t caps;
    uint32_t caps2;
    uint32_t caps3;
    uint32_t caps4;
    uint32_t reserved2;
};

static constexpr uint32_t DDSD_CAPS = 0x1;
static constexpr uint32_t DDSD_HEIGHT = 0x2;
static constexpr uint32_t DDSD_WIDTH = 0x4;
static constexpr uint32_t DDSD_PIXELFORMAT = 0x1000;
static constexpr uint32_t DDSD_MIPMAPCOUNT = 0x20000;
static constexpr uint32_t DDSD_LINEARSIZE = 0x80000;
static constexpr uint32_t DDSD_PITCH = 0x8;

static constexpr uint32_t DDSCAPS_COMPLEX = 0x8;
static constexpr uint32_t DDSCAPS_MIPMAP = 0x400000;
static constexpr uint32_t DDSCAPS_TEXTURE = 0x1000;

static uint32_t makeFourCC(char a, char b, char c, char d)
{
    return static_cast<uint32_t>(a) | (static_cast<uint32_t>(b) << 8) | (static_cast<uint32_t>(c) << 16) |
           (static_cast<uint32_t>(d) << 24);
}

static bool isDXTFormat(PixelFormat fmt)
{
    return fmt == PixelFormat::DXT1 || fmt == PixelFormat::DXT3 || fmt == PixelFormat::DXT5;
}

static size_t computeMipSize(int w, int h, PixelFormat fmt)
{
    if (fmt == PixelFormat::DXT1)
        return static_cast<size_t>(((w + 3) / 4)) * ((h + 3) / 4) * 8;
    if (fmt == PixelFormat::DXT3 || fmt == PixelFormat::DXT5)
        return static_cast<size_t>(((w + 3) / 4)) * ((h + 3) / 4) * 16;
    if (fmt == PixelFormat::RGBA8888 || fmt == PixelFormat::ARGB8888)
        return static_cast<size_t>(w) * h * 4;
    if (fmt == PixelFormat::ARGB4444 || fmt == PixelFormat::ARGB1555 || fmt == PixelFormat::AI88 ||
        fmt == PixelFormat::RGB565)
        return static_cast<size_t>(w) * h * 2;
    return 0;
}

static PixelFormat fourCCToPixelFormat(uint32_t fourCC)
{
    if (fourCC == makeFourCC('D', 'X', 'T', '1'))
        return PixelFormat::DXT1;
    if (fourCC == makeFourCC('D', 'X', 'T', '3'))
        return PixelFormat::DXT3;
    if (fourCC == makeFourCC('D', 'X', 'T', '5'))
        return PixelFormat::DXT5;
    return PixelFormat::Unknown;
}

static DDS_PIXELFORMAT makePixelFormat(PixelFormat fmt)
{
    DDS_PIXELFORMAT pf{};
    pf.size = sizeof(DDS_PIXELFORMAT);
    if (isDXTFormat(fmt))
    {
        pf.flags = DDPF_FOURCC;
        if (fmt == PixelFormat::DXT1)
            pf.fourCC = makeFourCC('D', 'X', 'T', '1');
        else if (fmt == PixelFormat::DXT3)
            pf.fourCC = makeFourCC('D', 'X', 'T', '3');
        else
            pf.fourCC = makeFourCC('D', 'X', 'T', '5');
    }
    else
    {
        // Uncompressed RGBA8888
        pf.flags = DDPF_RGB | DDPF_ALPHAPIXELS;
        pf.rgbBitCount = 32;
        pf.rBitMask = 0x000000FF;
        pf.gBitMask = 0x0000FF00;
        pf.bBitMask = 0x00FF0000;
        pf.aBitMask = 0xFF000000;
    }
    return pf;
}

// --- Reading ---

DDSFile DDSConverter::ReadDDSBuffer(const void* data, size_t size)
{
    DDSFile result;
    if (size < 4 + sizeof(DDS_HEADER))
        return result;

    const uint8_t* ptr = static_cast<const uint8_t*>(data);

    uint32_t magic;
    std::memcpy(&magic, ptr, 4);
    if (magic != DDS_MAGIC)
        return result;
    ptr += 4;

    DDS_HEADER hdr;
    std::memcpy(&hdr, ptr, sizeof(DDS_HEADER));
    ptr += sizeof(DDS_HEADER);

    if (hdr.size != sizeof(DDS_HEADER))
        return result;

    result.width = static_cast<int>(hdr.width);
    result.height = static_cast<int>(hdr.height);
    int mipCount = (hdr.flags & DDSD_MIPMAPCOUNT) ? std::max(1u, hdr.mipMapCount) : 1;

    // Determine pixel format
    if (hdr.ddspf.flags & DDPF_FOURCC)
    {
        result.format = fourCCToPixelFormat(hdr.ddspf.fourCC);
    }
    else if ((hdr.ddspf.flags & DDPF_RGB) && hdr.ddspf.rgbBitCount == 32)
    {
        result.format = PixelFormat::RGBA8888;
    }

    if (result.format == PixelFormat::Unknown)
        return result;

    // Read mipmap data
    const uint8_t* end = static_cast<const uint8_t*>(data) + size;
    int w = result.width, h = result.height;
    for (int i = 0; i < mipCount && w >= 1 && h >= 1; ++i)
    {
        size_t mipSz = computeMipSize(w, h, result.format);
        if (ptr + mipSz > end)
            break;

        DDSMipLevel level;
        level.width = w;
        level.height = h;
        level.data.assign(ptr, ptr + mipSz);
        result.mipmaps.push_back(std::move(level));

        ptr += mipSz;
        w = std::max(w / 2, 1);
        h = std::max(h / 2, 1);
    }

    if (result.mipmaps.empty())
    {
        result.format = PixelFormat::Unknown;
        result.width = result.height = 0;
    }

    return result;
}

DDSFile DDSConverter::ReadDDS(const std::string& path)
{
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f.good())
        return {};
    auto sz = f.tellg();
    f.seekg(0);
    std::vector<uint8_t> buf(static_cast<size_t>(sz));
    f.read(reinterpret_cast<char*>(buf.data()), sz);
    return ReadDDSBuffer(buf.data(), buf.size());
}

// --- Writing ---

std::vector<uint8_t> DDSConverter::WriteDDSBuffer(const DDSFile& dds)
{
    if (!dds.valid())
        return {};

    // Calculate total size
    size_t totalData = 0;
    for (auto& m : dds.mipmaps)
        totalData += m.data.size();

    std::vector<uint8_t> buf;
    buf.reserve(4 + sizeof(DDS_HEADER) + totalData);

    // Magic
    buf.resize(4);
    std::memcpy(buf.data(), &DDS_MAGIC, 4);

    // Header
    DDS_HEADER hdr{};
    hdr.size = sizeof(DDS_HEADER);
    hdr.flags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    hdr.height = static_cast<uint32_t>(dds.height);
    hdr.width = static_cast<uint32_t>(dds.width);
    hdr.ddspf = makePixelFormat(dds.format);
    hdr.caps = DDSCAPS_TEXTURE;

    if (dds.mipmaps.size() > 1)
    {
        hdr.flags |= DDSD_MIPMAPCOUNT;
        hdr.mipMapCount = static_cast<uint32_t>(dds.mipmaps.size());
        hdr.caps |= DDSCAPS_COMPLEX | DDSCAPS_MIPMAP;
    }

    if (isDXTFormat(dds.format))
    {
        hdr.flags |= DDSD_LINEARSIZE;
        hdr.pitchOrLinearSize = static_cast<uint32_t>(dds.mipmaps[0].data.size());
    }
    else
    {
        hdr.flags |= DDSD_PITCH;
        hdr.pitchOrLinearSize = static_cast<uint32_t>(dds.width * 4); // RGBA row pitch
    }

    size_t hdrOffset = buf.size();
    buf.resize(hdrOffset + sizeof(DDS_HEADER));
    std::memcpy(buf.data() + hdrOffset, &hdr, sizeof(DDS_HEADER));

    // Mipmap data
    for (auto& m : dds.mipmaps)
    {
        size_t off = buf.size();
        buf.resize(off + m.data.size());
        std::memcpy(buf.data() + off, m.data.data(), m.data.size());
    }

    return buf;
}

bool DDSConverter::WriteDDS(const std::string& path, const DDSFile& dds)
{
    auto buf = WriteDDSBuffer(dds);
    if (buf.empty())
        return false;
    std::ofstream f(path, std::ios::binary);
    if (!f.good())
        return false;
    f.write(reinterpret_cast<const char*>(buf.data()), buf.size());
    return f.good();
}

// --- PAA → DDS conversion (raw block copy for DXT) ---

DDSFile DDSConverter::PAAFileToDDS(const std::string& paaPath)
{
    DDSFile result;

    // Read PAA metadata
    PAAInfo info;
    if (!ReadPAAInfo(paaPath, info) || !info.formatName)
        return result;

    // Map PAA format → PixelFormat
    PixelFormat pf = PixelFormat::Unknown;
    switch (info.magic)
    {
        case 0xFF01:
            pf = PixelFormat::DXT1;
            break;
        case 0xFF03:
            pf = PixelFormat::DXT3;
            break;
        case 0xFF05:
            pf = PixelFormat::DXT5;
            break;
        default:
            break;
    }

    if (pf != PixelFormat::Unknown)
    {
        // DXT: extract raw compressed blocks from PAA mipmaps (no re-encoding)
        std::ifstream f(paaPath, std::ios::binary);
        if (!f.good())
            return result;

        // Skip magic word
        uint16_t magic;
        f.read(reinterpret_cast<char*>(&magic), 2);

        // Skip TAGGs
        while (f.good())
        {
            uint32_t tag;
            f.read(reinterpret_cast<char*>(&tag), 4);
            if (tag != 0x54414747)
            {
                f.seekg(-4, std::ios::cur);
                break;
            }
            uint32_t tagName;
            f.read(reinterpret_cast<char*>(&tagName), 4);
            uint32_t tagSize;
            f.read(reinterpret_cast<char*>(&tagSize), 4);
            f.seekg(tagSize, std::ios::cur);
        }

        // Skip palette
        uint16_t palCount;
        f.read(reinterpret_cast<char*>(&palCount), 2);
        if (palCount > 0)
            f.seekg(palCount * 3, std::ios::cur);

        // Read mipmap levels — raw DXT blocks
        result.format = pf;
        result.width = info.width;
        result.height = info.height;

        while (f.good())
        {
            uint16_t w, h;
            f.read(reinterpret_cast<char*>(&w), 2);
            f.read(reinterpret_cast<char*>(&h), 2);
            if (w == 0 && h == 0)
                break;
            if (!f.good())
                break;

            int rw = w, rh = h;
            if (w == 1234 && h == 8765)
            {
                f.read(reinterpret_cast<char*>(&w), 2);
                f.read(reinterpret_cast<char*>(&h), 2);
                rw = w;
                rh = h;
            }

            uint32_t dataSize = 0;
            f.read(reinterpret_cast<char*>(&dataSize), 3);
            dataSize &= 0xFFFFFF;

            DDSMipLevel level;
            level.width = rw;
            level.height = rh;
            level.data.resize(dataSize);
            f.read(reinterpret_cast<char*>(level.data.data()), dataSize);

            result.mipmaps.push_back(std::move(level));
        }

        if (result.mipmaps.empty())
            result = DDSFile{};
    }
    else
    {
        // Non-DXT (ARGB4444, ARGB1555, AI88): decode to RGBA, store as uncompressed DDS
        result.format = PixelFormat::RGBA8888;
        result.width = info.width;
        result.height = info.height;

        // Decode each mipmap level
        for (int m = 0; m < info.mipmapCount; ++m)
        {
            auto decoded = DecodePAAFileMip(paaPath, m);
            if (!decoded.valid())
                break;

            DDSMipLevel level;
            level.width = decoded.width;
            level.height = decoded.height;
            level.data = std::move(decoded.rgba);
            result.mipmaps.push_back(std::move(level));
        }

        if (result.mipmaps.empty())
            result = DDSFile{};
    }

    return result;
}

} // namespace Poseidon
