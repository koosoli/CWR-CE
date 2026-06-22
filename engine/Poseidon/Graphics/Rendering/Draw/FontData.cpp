// Shared FXY font data parser; used by the engine and the asset tools.

#include <Poseidon/Graphics/Rendering/Draw/FontData.hpp>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <ctype.h>

namespace Poseidon
{

static int getiw(QIStream& f)
{
    int low = (unsigned char)f.get();
    int high = (unsigned char)f.get();
    return low | (high << 8);
}

static int nextPow2(int v)
{
    int p = 1;
    while (p < v)
        p <<= 1;
    return p;
}

static void lowerName(const char* name, char* out, int outSize)
{
    snprintf(out, outSize, "%s", name);
    for (char* p = out; *p; ++p)
        *p = static_cast<char>(tolower(static_cast<unsigned char>(*p)));
}

FXYData ParseFXY(QIStream& stream, const char* name)
{
    FXYData result;

    char lowName[256];
    lowerName(name, lowName, sizeof(lowName));
    result.name = lowName;

    if (stream.rest() <= 0)
        return result;

    result.nChars = stream.rest() / (6 * sizeof(short));
    result.glyphs.resize(static_cast<size_t>(result.nChars));

    int maxW = 0;
    result.maxHeight = 0;

    while (stream.rest() > 0)
    {
        int c = getiw(stream);
        int setNum = getiw(stream);
        int x = getiw(stream);
        int y = getiw(stream);
        int w = getiw(stream);
        int h = getiw(stream);

        char texName[256];
        snprintf(texName, sizeof(texName), "%s-%02d.paa", lowName, setNum);

        if (c >= 0 && c < result.nChars)
        {
            FXYGlyph& glyph = result.glyphs[static_cast<size_t>(c)];
            glyph.charCode = c;
            glyph.setNum = setNum;
            glyph.x = x;
            glyph.y = y;
            glyph.w = w - 1;
            glyph.h = h - 1;
            glyph.wTex = nextPow2(w);
            glyph.hTex = nextPow2(h);
            glyph.textureName = texName;
        }

        result.textureSetNums.insert(setNum);

        if (w - 1 > maxW)
            maxW = w - 1;
        if (h - 1 > result.maxHeight)
            result.maxHeight = h - 1;
    }

    result.maxWidth = maxW;

    // Space char width = maxW * 3/4 (same as Font::Load)
    if (result.nChars > 0)
        result.glyphs[0].w = maxW * 3 / 4;

    return result;
}

std::vector<std::string> FXYData::GetTextureNames() const
{
    std::vector<std::string> names;
    for (int setNum : textureSetNums)
    {
        char texName[256];
        snprintf(texName, sizeof(texName), "%s-%02d.paa", name.c_str(), setNum);
        names.emplace_back(texName);
    }
    return names;
}

} // namespace Poseidon
