#include <Poseidon/Graphics/Core/MatrixConversion.hpp>
#include <Poseidon/Graphics/Core/Engine.hpp>
#include <Poseidon/Core/Global.hpp>
#include <Poseidon/Foundation/Math/Math3D.hpp>

namespace Poseidon
{

void ConvertMatrix(GfxMatrix& mat, Matrix4Val src)
{
    mat._11 = src.DirectionAside()[0];
    mat._12 = src.DirectionAside()[1];
    mat._13 = src.DirectionAside()[2];

    mat._21 = src.DirectionUp()[0];
    mat._22 = src.DirectionUp()[1];
    mat._23 = src.DirectionUp()[2];

    mat._31 = src.Direction()[0];
    mat._32 = src.Direction()[1];
    mat._33 = src.Direction()[2];

    mat._41 = src.Position()[0];
    mat._42 = src.Position()[1];
    mat._43 = src.Position()[2];

    mat._14 = 0;
    mat._24 = 0;
    mat._34 = 0;
    mat._44 = 1;
}

void ConvertMatrixTransposed(GfxMatrix& mat, Matrix4Val src)
{
    mat._11 = src.DirectionAside()[0];
    mat._21 = src.DirectionAside()[1];
    mat._31 = src.DirectionAside()[2];
    mat._41 = 0;

    mat._12 = src.DirectionUp()[0];
    mat._22 = src.DirectionUp()[1];
    mat._32 = src.DirectionUp()[2];
    mat._42 = 0;

    mat._13 = src.Direction()[0];
    mat._23 = src.Direction()[1];
    mat._33 = src.Direction()[2];
    mat._43 = 0;

    mat._14 = src.Position()[0];
    mat._24 = src.Position()[1];
    mat._34 = src.Position()[2];
    mat._44 = 1;
}

void ConvertProjectionMatrix(GfxMatrix& mat, Matrix4Val src, float zBias)
{
    mat._11 = src(0, 0);
    mat._12 = 0;
    mat._13 = 0;
    mat._14 = 0;

    mat._21 = 0;
    mat._22 = src(1, 1);
    mat._23 = 0;
    mat._24 = 0;

    float c = src(2, 2);
    float d = src.Position()[2];
    if (zBias > 0)
    {
        if (GEngine->GetTL() && GEngine->HasWBuffer() && GEngine->IsWBuffer())
        {
            float n = -d / c;
            float f = d / (1 - c);
            float add = 0;
            float mult = 1 + zBias * 1e-5f;

            f *= mult;
            n *= mult;
            n += add;

            c = f / (f - n);
            d = -c * n;
        }
        else
        {
            float zMult = 1.0f - zBias * 1e-7f;
            float zAdd = -zBias * 1e-6f;

            c = src(2, 2) * zMult + zAdd;
            d = src.Position()[2] * zMult;
        }
    }

    mat._31 = 0;
    mat._32 = 0;
    mat._33 = c;
    mat._34 = 1;

    mat._41 = 0;
    mat._42 = 0;
    mat._43 = d;
    mat._44 = 0;
}

} // namespace Poseidon
