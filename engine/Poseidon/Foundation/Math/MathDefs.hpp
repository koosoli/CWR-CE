#ifdef _MSC_VER
#pragma once
#endif

#ifndef _MATHDEFS_HPP
#define _MATHDEFS_HPP

#include <cfloat>

// common KNI/P declarations

// enum placeholders for the various matrix/vector constructors

namespace Poseidon::Foundation
{
enum _noInit
{
    NoInit
};
enum _vRotate
{
    VRotate
};
enum _vFastTransform
{
    VFastTransform
};
enum _vFastTransformA
{
    VFastTransformA
};
enum _vPerspective
{
    VPerspective
};
enum _vMultiply
{
    VMultiply
};
enum _vMultiplyLeft
{
    VMultiplyLeft
};

enum _mTranslation
{
    MTranslation
};
enum _mTilda
{
    MTilda
};
enum _mRotationX
{
    MRotationX
};
enum _mRotationY
{
    MRotationY
};
enum _mRotationZ
{
    MRotationZ
};
enum _mScale
{
    MScale
};
enum _mPerspective
{
    MPerspective
};
enum _mDirection
{
    MDirection
};
enum _mUpAndDirection
{
    MUpAndDirection
};
enum _mView
{
    MView
};
enum _mMultiply
{
    MMultiply
};
enum _mInverseRotation
{
    MInverseRotation
};
enum _mInverseScaled
{
    MInverseScaled
};
enum _mInverseGeneral
{
    MInverseGeneral
};
enum _mNormalTransform
{
    MNormalTransform
};

using Coord = float;

inline constexpr Coord CoordMin = -FLT_MAX;
inline constexpr Coord CoordMax = +FLT_MAX;

template <typename T>
constexpr Coord ToCoord(T value)
{
    return static_cast<Coord>(value);
}

inline constexpr Coord HPi = 3.14159265358979323846f;
inline constexpr Coord HSqrt2 = 1.4142135624f;
inline constexpr Coord HInvSqrt2 = 0.70710678119f;

#define COORD_MIN (CoordMin)
#define COORD_MAX (CoordMax)
#define coord(x) ToCoord(x)

#define H_PI (HPi)
#define H_SQRT2 (HSqrt2)
#define H_INVSQRT2 (HInvSqrt2)

} // namespace Poseidon::Foundation

using ::Poseidon::Foundation::_mDirection;
using ::Poseidon::Foundation::_mInverseGeneral;
using ::Poseidon::Foundation::_mInverseRotation;
using ::Poseidon::Foundation::_mInverseScaled;
using ::Poseidon::Foundation::_mMultiply;
using ::Poseidon::Foundation::_mNormalTransform;
using ::Poseidon::Foundation::_mPerspective;
using ::Poseidon::Foundation::_mRotationX;
using ::Poseidon::Foundation::_mRotationY;
using ::Poseidon::Foundation::_mRotationZ;
using ::Poseidon::Foundation::_mScale;
using ::Poseidon::Foundation::_mTilda;
using ::Poseidon::Foundation::_mTranslation;
using ::Poseidon::Foundation::_mUpAndDirection;
using ::Poseidon::Foundation::_mView;
using ::Poseidon::Foundation::_noInit;
using ::Poseidon::Foundation::_vFastTransform;
using ::Poseidon::Foundation::_vFastTransformA;
using ::Poseidon::Foundation::_vMultiply;
using ::Poseidon::Foundation::_vMultiplyLeft;
using ::Poseidon::Foundation::_vPerspective;
using ::Poseidon::Foundation::_vRotate;
using ::Poseidon::Foundation::Coord;
using ::Poseidon::Foundation::CoordMax;
using ::Poseidon::Foundation::CoordMin;
using ::Poseidon::Foundation::HInvSqrt2;
using ::Poseidon::Foundation::HPi;
using ::Poseidon::Foundation::HSqrt2;
using ::Poseidon::Foundation::MDirection;
using ::Poseidon::Foundation::MInverseGeneral;
using ::Poseidon::Foundation::MInverseRotation;
using ::Poseidon::Foundation::MInverseScaled;
using ::Poseidon::Foundation::MMultiply;
using ::Poseidon::Foundation::MNormalTransform;
using ::Poseidon::Foundation::MPerspective;
using ::Poseidon::Foundation::MRotationX;
using ::Poseidon::Foundation::MRotationY;
using ::Poseidon::Foundation::MRotationZ;
using ::Poseidon::Foundation::MScale;
using ::Poseidon::Foundation::MTilda;
using ::Poseidon::Foundation::MTranslation;
using ::Poseidon::Foundation::MUpAndDirection;
using ::Poseidon::Foundation::MView;
using ::Poseidon::Foundation::NoInit;
using ::Poseidon::Foundation::ToCoord;
using ::Poseidon::Foundation::VFastTransform;
using ::Poseidon::Foundation::VFastTransformA;
using ::Poseidon::Foundation::VMultiply;
using ::Poseidon::Foundation::VMultiplyLeft;
using ::Poseidon::Foundation::VPerspective;
using ::Poseidon::Foundation::VRotate;

#endif
