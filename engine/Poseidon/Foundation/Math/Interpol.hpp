#pragma once

// numeric interpolators

// Spline interpolation. Parameters of Spline()/Splint():
//   x[]   - [0..n] independent variable, MUST be strictly increasing: x[0] < x[i] < x[n]
//   y[]   - [0..n] dependent variable
//   ypp[] - [0..n] second derivatives d2y/dx2 (filled by Spline, consumed by Splint)
//   n     - maximum index into x[]/y[]/ypp[]

#include <Poseidon/Foundation/Types/Pointers.hpp>
#include <Poseidon/Foundation/Common/FltOpts.hpp>
#include <Poseidon/Foundation/Math/Math3D.hpp>
namespace Poseidon::Foundation
{
template <class Numeric>
void Spline(const float x[], const Numeric y[], int n, Numeric* ypp, const Numeric& zero)
{
    n--;

    int i;
    Temp<float> a(n + 1), b(n + 1), c(n + 1), gamma(n + 1);
    Temp<Numeric> r1(n + 1), r2(n + 1);

    // Build a tri-diagonal system for the second-derivative values at the nodes.
    for (i = 1; i < n; i++)
    {
        a[i] = (x[i] - x[i - 1]) * (1 / 6.0);
        b[i] = (x[i + 1] - x[i - 1]) * (1 / 3.0);
        c[i] = (x[i + 1] - x[i]) * (1 / 6.0);
        r1[i] = (y[i + 1] - y[i]) * (1 / (x[i + 1] - x[i])) - (y[i] - y[i - 1]) * (1 / (x[i] - x[i - 1]));
    }

    b[0] = 1.0;
    c[0] = 0.0;
    r1[0] = zero;

    a[n] = 0.0;
    b[n] = 1.0;
    r1[n] = zero;

    // Solve via the Thomas algorithm.
    i = 0;
    gamma[i] = c[i] / b[i];
    r2[i] = r1[i] * (1 / b[i]);

    i = 1;
    while (i <= n - 1)
    {
        float betai = b[i] - a[i] * gamma[i - 1];
        gamma[i] = c[i] / betai;
        r2[i] = (r1[i] - r2[i - 1] * a[i]) * (1 / betai);
        i = i + 1;
    }

    i = n;
    float betai = b[i] - a[i] * gamma[i - 1];
    r2[i] = (r1[i] - r2[i - 1] * a[i]) * (1 / betai);

    ypp[i] = r2[i];

    i = n - 1;
    while (i >= 0)
    {
        ypp[i] = r2[i] - ypp[i + 1] * gamma[i];
        i = i - 1;
    }
}

template <class Numeric>
Numeric Splint(const float x[], const Numeric y[], const Numeric ypp[], int n, float xi)
{
    // xi is the x value where interpolation is needed; returns the interpolated y.
    int i_lo = 0;
    int i_hi = 0;
    while (i_hi < n && x[i_hi] < xi)
        i_hi++;
    if (i_hi <= 0)
        return y[0];
    if (i_hi >= n)
        return y[n - 1];
    i_lo = i_hi - 1;

    PoseidonAssert(x[i_hi] >= xi);
    PoseidonAssert(x[i_lo] <= xi);

    float invDenom = 1 / (x[i_hi] - x[i_lo]);
    float A = (x[i_hi] - xi) * invDenom;
    float B = (xi - x[i_lo]) * invDenom;
    PoseidonAssert(fabs(A + B - 1) < 1e-6);
    float C = (A * A * A - A) * Square(x[i_hi] - x[i_lo]) * (1 / 6.0);
    float D = (B * B * B - B) * Square(x[i_hi] - x[i_lo]) * (1 / 6.0);

    return y[i_lo] * A + y[i_hi] * B + ypp[i_lo] * C + ypp[i_hi] * D;
}

template <class Numeric>
Numeric Lint(const float x[], const Numeric y[], int n, float xi)
{
    int i_lo = 0;
    int i_hi = 0;
    while (i_hi < n && x[i_hi] < xi)
        i_hi++;
    if (i_hi <= 0)
        return y[0];
    if (i_hi >= n)
        return y[n - 1];
    i_lo = i_hi - 1;

    PoseidonAssert(x[i_hi] >= xi);
    PoseidonAssert(x[i_lo] <= xi);

    float invDenom = 1 / (x[i_hi] - x[i_lo]);
    float A = (x[i_hi] - xi) * invDenom;
    return y[i_lo] * A + y[i_hi] * (1 - A);
}

class M4Function
{
  public:
    virtual Matrix4 operator()(float time) const = 0;
    virtual ~M4Function() {}

    // query domain
    virtual float MinArg() const = 0;
    virtual float MaxArg() const = 0;
};

class InterpolatorLinear : public M4Function
{
    const Matrix4* _values;
    const float* _times;
    int _n;

  public:
    InterpolatorLinear(const Matrix4* values, const float* times, int n);
    Matrix4 operator()(float time) const override;

    float MinArg() const override { return _times[0]; }
    float MaxArg() const override { return _times[_n - 1]; }
};

class InterpolatorSpline : public M4Function
{
    const Matrix4* _values;
    const float* _times;
    int _n;
    Temp<Matrix4> _sValues;

  public:
    InterpolatorSpline(const Matrix4* values, const float* times, int n);
    Matrix4 operator()(float time) const override;

    float MinArg() const override { return _times[0]; }
    float MaxArg() const override { return _times[_n - 1]; }
};

} // namespace Poseidon::Foundation

using ::Poseidon::Foundation::InterpolatorLinear;
using ::Poseidon::Foundation::InterpolatorSpline;
using ::Poseidon::Foundation::Lint;
using ::Poseidon::Foundation::M4Function;
