#pragma once

#include <Poseidon/Graphics/Rendering/Font/Pactext.hpp>
#include <Poseidon/Foundation/Strings/RString.hpp>

#include <memory>
#include <vector>

namespace Poseidon
{
// JPEG texture loader (stb_image). Decodes to RGBA8 at Init() time, caches the
// decoded pixels, and downsamples on demand in GetMipmapData(). Cross-platform,
// no external DLL dependency. Requires power-of-two source dimensions.
class TextureSourceJPEG : public ITextureSource
{
    RStringB _name;
    std::vector<uint8_t> _rgba0; //!< decoded mip 0 as RGBA8
    int _w = 0;
    int _h = 0;
    int _mipmaps = 0;
    PackedColor _avgColor{};

  public:
    TextureSourceJPEG();
    ~TextureSourceJPEG() override;

    bool Init(const char* name, PacLevelMem* mips, int maxMips) override;

    int GetMipmapCount() const override { return _mipmaps; }
    PacFormat GetFormat() const override { return PacARGB1555; }
    bool GetMipmapData(void* mem, const PacLevelMem& mip, int level) const override;

    PackedColor GetAverageColor() const override { return _avgColor; }

    bool IsAlpha() const override { return false; }
    bool IsTransparent() const override { return false; }
    void ForceAlpha() override {}

    // Testable entry point — decode from an in-memory JPEG buffer. Called by
    // Init() after it has loaded the file; exposed separately so unit tests
    // can exercise the decode path without initialising the file server.
    bool InitFromMemory(const uint8_t* data, size_t size, const char* nameForErrors);
};

class TextureSourceJPEGFactory : public ITextureSourceFactory
{
  public:
    bool Check(const char* name) override;
    void PreInit(const char* name) override;
    ITextureSource* Create(const char* name, PacLevelMem* mips, int maxMips) override;
};

extern TextureSourceJPEGFactory* GTextureSourceJPEGFactory;
} // namespace Poseidon
