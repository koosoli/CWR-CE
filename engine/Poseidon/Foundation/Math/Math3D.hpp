#ifdef _MSC_VER
#pragma once
#endif

#ifndef _MATH3D_HPP
#define _MATH3D_HPP

#if defined _KNI
#include <Poseidon/Foundation/Math/Math3DK.hpp>
// force K types
#define Vector3 Vector3K
#define Point3 Vector3K
#define Vector3Val Vector3KVal
#define Vector3Par Vector3KPar
#define Matrix3 Matrix3K
#define Matrix4 Matrix4K
#define Matrix3Val const Matrix3K&
#define Matrix4Val const Matrix4K&
#define Matrix3Par const Matrix3K&
#define Matrix4Par const Matrix4K&
#define ConvertToM(x) ConvertPToK(x)
#define ConvertToP(x) ConvertKToP(x)
#define VZero VZeroK
#define VUp VUpK
#define VForward VForwardK
#define VAside VAsideK

#define MIdentity M4IdentityK
#define MZero M4ZeroK
#define M4Identity M4IdentityK
#define M4Zero M4ZeroK
#define M3Identity M3IdentityK
#define M3Zero M3ZeroK
#else
#include <Poseidon/Foundation/Math/Math3DP.hpp>
// force P types
#define Vector3 Vector3P
#define Point3 Vector3P
#define Matrix3 Matrix3P
#define Matrix4 Matrix4P

#define Vector3Val Vector3PVal
#define Vector3Par Vector3PPar
#define Matrix3Val Matrix3PVal
#define Matrix3Par Matrix3PPar
#define Matrix4Val Matrix4PVal
#define Matrix4Par Matrix4PPar

#define ConvertToM(x) (x)
#define ConvertToP(x) (x)

#define VZero VZeroP
#define VUp VUpP
#define VForward VForwardP
#define VAside VAsideP
#define MIdentity M4IdentityP
#define MZero M4ZeroP
#define M4Identity M4IdentityP
#define M4Zero M4ZeroP
#define M3Identity M3IdentityP
#define M3Zero M3ZeroP
#endif

#endif

namespace Poseidon::Foundation
{
} // namespace Poseidon::Foundation
