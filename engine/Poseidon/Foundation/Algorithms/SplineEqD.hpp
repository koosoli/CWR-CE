#pragma once

#ifndef POSEIDON_BASE_ALGORITHMS_SPLINEEQD_HPP
#define POSEIDON_BASE_ALGORITHMS_SPLINEEQD_HPP

#include <Poseidon/Foundation/Common/FltOpts.hpp>
#include <Poseidon/Foundation/Containers/TypeOpts.hpp>
#include <Poseidon/Foundation/Types/Pointers.hpp>
#include <cmath>

// Cubic spline interpolation.

/* Spline: fill ypp[0..n] with d2y/dx2 for samples y[0..n].
   x ranges over [0,1]; x==1 corresponds to index n-1. */


namespace Poseidon::Foundation
{
template <class Numeric>
void Spline(const Numeric y[], int n, Numeric* ypp, const Numeric& zero)
{
    const float sampleCount = static_cast<float>(n);
    const float invSampleCount = 1.0f / sampleCount;

    n--;

    int i;
    Temp<float> a(n + 1), b(n + 1), c(n + 1), gamma(n + 1);
    Temp<Numeric> r1(n + 1), r2(n + 1);

    // Build the tri-diagonal system for the second derivatives at the nodes.

    for (i = 1; i < n; i++)
    {
        a[i] = invSampleCount * (1 / 6.0);
        b[i] = (invSampleCount * 2) * (1 / 3.0);
        c[i] = invSampleCount * (1 / 6.0);
        r1[i] = (y[i + 1] - y[i]) * sampleCount - (y[i] - y[i - 1]) * sampleCount;
    }

    b[0] = 1.0;
    c[0] = 0.0;
    r1[0] = zero;

    a[n] = 0.0;
    b[n] = 1.0;
    r1[n] = zero;

    // Solve the system with the Thomas algorithm.
    i = 0;
    gamma[i] = c[i] / b[i];
    r2[i] = r1[i] * (1 / b[i]);

    i = 1;
    while (i <= n - 1)
    {
        float betai = b[i] - a[i] * gamma[i - 1];
        gamma[i] = c[i] / betai;
        r2[i] = (r1[i] - r2[i - 1] * a[i]) * (1 / betai);
        ++i;
    }

    i = n;
    float betai = b[i] - a[i] * gamma[i - 1];
    r2[i] = (r1[i] - r2[i - 1] * a[i]) * (1 / betai);

    ypp[i] = r2[i];

    i = n - 1;
    while (i >= 0)
    {
        ypp[i] = r2[i] - ypp[i + 1] * gamma[i];
        --i;
    }
}

template <class Numeric>
Numeric Splint(const Numeric y[], const Numeric ypp[], int n, float xi)
{
    /* Splint: interpolate y at xi using the spline second derivatives ypp[0..n].
       x ranges over [0,1); x==1 corresponds to index n. */

    // table lookup: x in [0,1] maps to index [0,n]

    const float invN = 1.0f / n;

    int i_lo = toIntFloor(xi * n);
    int i_hi = i_lo + 1;
    if (i_hi <= 0)
    {
        return y[0];
    }
    if (i_hi >= n)
    {
        return y[n - 1];
    }

    const float x_lo = i_lo * invN;
    const float x_hi = i_hi * invN;
    PoseidonAssert(x_hi >= xi);
    PoseidonAssert(x_lo <= xi);

    const float invDenom = n;
    // 1/(x_hi - x_lo) == n, since nodes are 1/n apart
    const float A = (x_hi - xi) * invDenom;
    const float B = (xi - x_lo) * invDenom;
    PoseidonAssert(std::fabs(A + B - 1) < 1e-6f);
    const float C = (A * A * A - A) * Square(invN) * (1 / 6.0);
    const float D = (B * B * B - B) * Square(invN) * (1 / 6.0);

    return y[i_lo] * A + y[i_hi] * B + ypp[i_lo] * C + ypp[i_hi] * D;
}

} // namespace Poseidon::Foundation

#endif
