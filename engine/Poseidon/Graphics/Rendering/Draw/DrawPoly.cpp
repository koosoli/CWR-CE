#include <Poseidon/Core/Config/EngineConfig.hpp>

#include <Poseidon/Graphics/Rendering/Primitives/Poly.hpp>
#include <Poseidon/Graphics/Core/TLVertex.hpp>
#include <Poseidon/Graphics/Core/Engine.hpp>
#include <Poseidon/Core/Global.hpp>

#include <Poseidon/World/Scene/Scene.hpp>
#include <Poseidon/World/Scene/Camera/Camera.hpp>
#include <limits.h>
#include <Poseidon/Foundation/Common/FltOpts.hpp>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Poseidon/Foundation/Math/Math3D.hpp>
#include <Poseidon/Foundation/Math/Math3DP.hpp>
#include <Poseidon/Foundation/Math/MathOpt.hpp>

namespace Poseidon
{

void PrepareTexture(Texture* texture, float z2, int special, float areaOTex)
{
    if (!texture)
    {
        return;
    }
    texture->LoadHeaders();
    int nMipmaps = texture->ANMipmaps();
    int texW = texture->AWidth();
    int texH = texture->AHeight();
    // if not predefined, use object settings
    int level = nMipmaps - 1;
    int mipTop = level;
    // do not calculate mipmaps for non-transparent shadows
    if (!(special & IsShadow) || texture->IsTransparent())
    {
        if (special & BestMipmap)
        {
            level = mipTop = 0;
        }
        else
        {
            // one screen pixel contains textArea/scrArea _texture pixels (average)
            // select mip-map level based on polygon distance

            // estimate screen area from z and area
            saturateMax(z2, Square(0.1));

            Camera& camera = *GScene->GetCamera();
            // apply perspective on position and size
            Matrix4Val project = camera.Projection();

            // perspective screen size
            // fast inv interpolation (6b precision is enough)
            float invZ = InvSqrt(z2);
            float sizeMin = project(0, 0) * invZ * camera.InvLeft();
            // select mipmaps based on areaOTex

            float scrAreaOTex = areaOTex * Square(sizeMin);
            float hw = texH * texW;

            // the higher scrAreaOTex, the better mipmap should be used
            for (level = 0; level < nMipmaps - 1; level++, scrAreaOTex *= 4)
            {
                if (scrAreaOTex >= hw)
                {
                    break;
                }
            }
        }
    }

    int txtSize = texW;
    saturateMax(txtSize, texH);
    txtSize >>= level;
    mipTop = level;
    // no limit for merged textures
    int maxSize = texture->AMaxSize();

    float dropdown = Glob.dropDown;
    if (special & NoDropdown)
    {
        saturateMax(maxSize, ENGINE_CONFIG.maxCockText);
        dropdown = 0;
    }
    while (txtSize > maxSize && mipTop < nMipmaps - 1)
    {
        mipTop++, txtSize >>= 1;
    }
    saturateMax(dropdown, Glob.fullDropDown);
    level += toInt(dropdown * ENGINE_CONFIG.autoDropText);

    // try to reload _texture data
    if (level > nMipmaps - 1)
    {
        level = nMipmaps - 1;
    }
    if (mipTop > nMipmaps - 1)
    {
        mipTop = nMipmaps - 1;
    }
    if (mipTop < 0)
    {
        mipTop = 0;
    }
    if (level < mipTop)
    {
        level = mipTop;
    }

    texture->PrepareMipmap(level, mipTop);
    // if texture is animated prepare all animation phases
    if (texture->IsAnimated())
    {
        int n = texture->AnimationLength();
        for (int i = 0; i < n; i++)
        {
            Texture* anim = texture->GetAnimation(i);
            anim->PrepareMipmap(level, mipTop);
        }
    }
}

void PolyProperties::Prepare(Texture* texture, int special)
{
    PoseidonAssert(texture != (Texture*)-1);

    // some triangles have mipmap level predefined (e.g. landscape)
    AbstractTextBank* bank = GEngine->TextBank();
    int level = INT_MAX, mipTop = INT_MAX;
    if (special & BestMipmap)
    {
        level = mipTop = 0;
    }

    MipInfo mip = bank->UseMipmap(texture, level, mipTop);

    // check current texture state

    if (texture)
    {
        PoseidonAssert(mip.IsOK());

        if (!mip.IsOK())
        {
            return;
        }
    }

    GEngine->PrepareTriangle(mip, special);
}

void PolyProperties::PrepareTL() const
{
    PoseidonAssert(_texture != (Texture*)-1);

    // some triangles have mipmap level predefined (e.g. landscape)
    int level = INT_MAX, mipTop = INT_MAX;
    if (_special & BestMipmap)
    {
        level = mipTop = 0;
    }

    AbstractTextBank* bank = GEngine->TextBank();
    MipInfo mip = bank->UseMipmap(_texture, level, mipTop);

    // check current texture state
    if (_texture)
    {
        PoseidonAssert(mip.IsOK());
        if (!mip.IsOK())
        {
            return;
        }
    }

    GEngine->PrepareTriangleTL(mip, render::SplitLegacy(_special));
}

void PolyProperties::PrepareTL(Texture* texture, int special)
{
    PoseidonAssert(texture != (Texture*)-1);

    // some triangles have mipmap level predefined (e.g. landscape)
    int level = INT_MAX, mipTop = INT_MAX;
    if (special & BestMipmap)
    {
        level = mipTop = 0;
    }

    AbstractTextBank* bank = GEngine->TextBank();
    MipInfo mip = bank->UseMipmap(texture, level, mipTop);

    // check current texture state
    if (texture)
    {
        PoseidonAssert(mip.IsOK());
        if (!mip.IsOK())
        {
            return;
        }
    }

    GEngine->PrepareTriangleTL(mip, render::SplitLegacy(special));
}
} // namespace Poseidon
