#ifdef _MSC_VER
#pragma once
#endif

#ifndef _MATHOPT_HPP
#define _MATHOPT_HPP

#include <Poseidon/Foundation/Math/MathDefs.hpp>
#include <Poseidon/Foundation/platform.hpp>
#include <cmath>

namespace Poseidon::Foundation
{
inline constexpr double InvEps = 1e-4;
inline constexpr double InvSqrtEps = 1e-3;

#define INV_EPS (InvEps)
#define INVSQRT_EPS (InvSqrtEps)

[[nodiscard]] constexpr Coord HDegree(Coord angle)
{ // convert degree to radian
    return angle * coord(H_PI / 180);
}

[[nodiscard]] constexpr float fSign(float x)
{
    if (x > 0)
    {
        return +1.0f;
    }
    if (x < 0)
    {
        return -1.0f;
    }
    return 0.0f;
}

[[nodiscard]] constexpr int sign(float x)
{
    if (x > 0)
    {
        return +1;
    }
    if (x < 0)
    {
        return -1;
    }
    return 0;
}

#ifndef _KNI

[[nodiscard]] inline float Inv(float c)
{
    return 1.0f / c;
}

float InvSqrt(float c);

[[nodiscard]] constexpr float Square(float x)
{
    return x * x;
}

#else

// consider: NewtonRaphson Reciprocal
//   [2 * rcpps(x) - (x * rcpps(x) * rcpps(x))]
[[nodiscard]] inline float Inv(float c)
{
    return 1.0f / c;
}

//	NewtonRaphson Reciprocal Square Root
//  	0.5 * rsqrtps * (3 - x * rsqrtps(x) * rsqrtps(x))
float InvSqrt(float f);

[[nodiscard]] constexpr float Square(float x)
{
    return x * x;
}

#endif

} // namespace Poseidon::Foundation

using ::Poseidon::Foundation::fSign;
using ::Poseidon::Foundation::HDegree;
using ::Poseidon::Foundation::Inv;
using ::Poseidon::Foundation::InvEps;
using ::Poseidon::Foundation::InvSqrt;
using ::Poseidon::Foundation::InvSqrtEps;
using ::Poseidon::Foundation::sign;
using ::Poseidon::Foundation::Square;

#endif
