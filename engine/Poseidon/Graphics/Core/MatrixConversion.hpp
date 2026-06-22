#pragma once

#ifndef __MATRIX_CONVERSION_HPP
#define __MATRIX_CONVERSION_HPP

#include <Poseidon/Foundation/Math/Math3D.hpp>

// GfxMatrix: portable 4x4 float matrix with the same memory layout as D3DMATRIX.
// On Windows this is ABI-compatible with D3DMATRIX (both are 16 contiguous floats
// with identical named members). We define our own struct to avoid pulling in
// d3d9types.h (which requires <windows.h>).

namespace Poseidon
{
struct GfxMatrix
{
    union
    {
        struct
        {
            float _11, _12, _13, _14;
            float _21, _22, _23, _24;
            float _31, _32, _33, _34;
            float _41, _42, _43, _44;
        };
        float m[4][4];
    };
};

// Row-major; maps the OFP coordinate system into the graphics matrix.
void ConvertMatrix(GfxMatrix& mat, Matrix4Val src);
void ConvertMatrixTransposed(GfxMatrix& mat, Matrix4Val src);
void ConvertProjectionMatrix(GfxMatrix& mat, Matrix4Val src, float zBias);

} // namespace Poseidon
#endif
