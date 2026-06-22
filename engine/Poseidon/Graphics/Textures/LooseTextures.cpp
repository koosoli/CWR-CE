#include <Poseidon/Graphics/Textures/LooseTextures.hpp>

#include <Poseidon/IO/Streams/QBStream.hpp>
#include <Poseidon/Foundation/Platform/AppConfig.hpp>

#include <cstring>
#include <Poseidon/Foundation/Strings/RString.hpp>

namespace Poseidon
{
namespace Graphics
{

static bool HasExtCi(const char* name, const char* ext)
{
    size_t nlen = strlen(name);
    size_t elen = strlen(ext);
    if (nlen < elen)
        return false;
    for (size_t i = 0; i < elen; ++i)
    {
        char a = name[nlen - elen + i];
        char b = ext[i];
        if (a >= 'A' && a <= 'Z')
            a = static_cast<char>(a - 'A' + 'a');
        if (b >= 'A' && b <= 'Z')
            b = static_cast<char>(b - 'A' + 'a');
        if (a != b)
            return false;
    }
    return true;
}

static RString SwapExt(const char* name, const char* newExt)
{
    size_t nlen = strlen(name);
    // Find last '.' after the last path separator.
    size_t dot = nlen;
    for (size_t i = nlen; i-- > 0;)
    {
        char c = name[i];
        if (c == '/' || c == '\\')
            break;
        if (c == '.')
        {
            dot = i;
            break;
        }
    }
    if (dot == nlen)
        return RString(name);
    char buf[512];
    size_t copy = dot < sizeof(buf) ? dot : sizeof(buf) - 1;
    memcpy(buf, name, copy);
    buf[copy] = 0;
    size_t bufLen = strlen(buf);
    strncat(buf, newExt, sizeof(buf) - bufLen - 1);
    return RString(buf);
}

RString ResolveLooseTexturePath(const char* name)
{
    if (!name || !name[0])
        return RString();
    if (!AppConfig::Instance().LooseTextures())
        return RString(name);

    // Override the engine's baked texture extensions only.  PNG / TGA
    // direct refs pass through unchanged.
    const bool baked =
        HasExtCi(name, ".paa") || HasExtCi(name, ".pac") || HasExtCi(name, ".jpg") || HasExtCi(name, ".jpeg");
    if (!baked)
        return RString(name);

    // Fallback order: .png → .tga → .jpg → .jpeg → original.
    RString png = SwapExt(name, ".png");
    if (QIFStreamB::FileExist(png))
        return png;

    RString tga = SwapExt(name, ".tga");
    if (QIFStreamB::FileExist(tga))
        return tga;

    RString jpg = SwapExt(name, ".jpg");
    if (QIFStreamB::FileExist(jpg))
        return jpg;

    RString jpeg = SwapExt(name, ".jpeg");
    if (QIFStreamB::FileExist(jpeg))
        return jpeg;

    return RString(name);
}

} // namespace Graphics
} // namespace Poseidon
