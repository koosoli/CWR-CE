#include <Poseidon/Foundation/Math/Interpol.hpp>

namespace Poseidon::Foundation
{
InterpolatorLinear::InterpolatorLinear(const Matrix4* values, const float* times, int n)
{
    _values = values;
    _times = times;
    _n = n;
}

Matrix4 InterpolatorLinear::operator()(float time) const
{
    return Lint(_times, _values, _n, time);
}

InterpolatorSpline::InterpolatorSpline(const Matrix4* values, const float* times, int n)
{
    _values = values;
    _times = times;
    _n = n;
    _sValues.Realloc(n + 1);
    Spline(times, values, n, _sValues.Data(), MZero);
}

Matrix4 InterpolatorSpline::operator()(float time) const
{
    return Splint(_times, _values, _sValues.Data(), _n, time);
}

} // namespace Poseidon::Foundation
