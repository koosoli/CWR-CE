#include <Poseidon/Core/Application.hpp>
#include <Poseidon/Graphics/Dummy/TextureDummy.hpp>
#include <cstring>
#include <Poseidon/Foundation/Framework/AppFrame.hpp>
#include <Poseidon/Foundation/Framework/Log.hpp>
#include <Poseidon/Foundation/Types/Pointers.hpp>
#include <Poseidon/Foundation/platform.hpp>

namespace Poseidon
{

TextureDummy::TextureDummy() = default;

static PacFormat BasicFormat(const char* name)
{
    const char* ext = strrchr(name, '.');
    if (ext && !strcmpi(ext, ".paa"))
    {
        return PacARGB4444;
    }
    else
    {
        return PacARGB1555;
    }
}

static PacFormat DstFormat(PacFormat srcFormat, int dxt)
{
    // memory representation
    switch (srcFormat)
    {
        case PacARGB1555:
        case PacRGB565:
        case PacARGB4444:
            return srcFormat;
        case PacP8:
            return PacARGB1555;
        case PacAI88:
            return srcFormat;
        case PacDXT1:
            return srcFormat;
        default:
            LOG_DEBUG(Graphics, "Unsupported source format {}", (int)srcFormat);
            return srcFormat;
    }
}

int TextureDummy::Init()
{
    _glideEmulation = false; // no Glide backend
    _aRatio = 0;

    PacFormat format = BasicFormat(Name());

    ITextureSourceFactory* factory = SelectTextureSourceFactory(Name());
    if (!factory || !factory->Check(Name()))
    {
        _nMipmaps = 0;
        return -1;
    }
    _src = factory->Create(Name(), _mipmaps, MAX_MIPMAPS);

    if (_src)
    {
        // in.seekg(0,QIOS::beg);
        //  .paa should start with format marker

#if REPORT_ALLOC >= 20
        LOG_DEBUG(Graphics, "Init texture {}", (const char*)Name());
#endif
        format = _src->GetFormat();

        if (format == PacARGB4444 || format == PacAI88 || format == PacARGB8888)
        {
            _src->ForceAlpha();
        }

        int dxt = 0;
        PacFormat dFormat = DstFormat(format, dxt);

        int nMipmaps = _src->GetMipmapCount();
        int i;
        for (i = 0; i < nMipmaps; i++)
        {
            PacLevelMem& mip = _mipmaps[i];

            mip.SetDestFormat(dFormat, 8);

            // do not load too small mip-maps
            if (mip._w < 2)
            {
                break;
            }
            if (mip._h < 2)
            {
                break;
            }
        }

        _nMipmaps = i;

        _w = _mipmaps[0]._w;
        _h = _mipmaps[0]._h;

        int w = _w;
        int h = _h;
        if (h == w)
        {
            _aRatio = 0;
        }
        else if (h * 2 == w)
        {
            _aRatio = +1;
        }
        else if (h == w * 2)
        {
            _aRatio = -1;
        }
        else if (h * 4 == w)
        {
            _aRatio = +2;
        }
        else if (h == w * 4)
        {
            _aRatio = -2;
        }
        else if (h * 8 == w)
        {
            _aRatio = +3;
        }
        else if (h == w * 8)
        {
            _aRatio = -3;
        }
        else
        {
            Poseidon::Foundation::ErrorMessage("Invalid texture aspect ratio (%dx%d)", w, h);
        }

        return 0;
    }
    return -1;
}

// some APIs (Glide) require u,v conversion

float TextureDummy::UToPhysical(float u) const
{
    if (!_glideEmulation)
    {
        return u;
    }
    int uf, vf;
    int aRatio = _aRatio;
    if (aRatio > 0)
    {
        uf = 256, vf = 256 >> aRatio;
    }
    else
    {
        vf = 256, uf = 256 >> -aRatio;
    }
    return u * uf;
}

float TextureDummy::VToPhysical(float v) const
{
    if (!_glideEmulation)
    {
        return v;
    }
    int uf, vf;
    int aRatio = _aRatio;
    if (aRatio > 0)
    {
        uf = 256, vf = 256 >> aRatio;
    }
    else
    {
        vf = 256, uf = 256 >> -aRatio;
    }
    return v * vf;
}

float TextureDummy::UToLogical(float u) const
{
    if (!_glideEmulation)
    {
        return u;
    }
    int uif, vif;
    int aRatio = _aRatio;
    if (aRatio > 0)
    {
        uif = 1, vif = 1 << aRatio;
    }
    else
    {
        vif = 1, uif = 1 << -aRatio;
    }
    return u * (1.0f / 256) * uif;
}

float TextureDummy::VToLogical(float v) const
{
    if (!_glideEmulation)
    {
        return v;
    }
    int uif, vif;
    int aRatio = _aRatio;
    if (aRatio > 0)
    {
        uif = 1, vif = 1 << aRatio;
    }
    else
    {
        vif = 1, uif = 1 << -aRatio;
    }
    return v * (1.0f / 256) * vif;
}

} // namespace Poseidon
