#include <Poseidon/Core/Global.hpp>

#include <Poseidon/Foundation/Time/Time.hpp>

namespace Poseidon::Foundation
{
float AbstractTime::Diff(const AbstractTime& x) const
{
    // Widen to 64-bit before subtracting: the plain operator- overflowed and caused
    // hard-to-find bugs. This form cannot overflow.
    return 1e-3 * ((__int64)_time - (__int64)x._time);
}

#ifndef NDEBUG

void UITime::operator+=(float diff)
{
    __int64 result = _time;
    result += ::toLargeInt(1e3 * diff);
    PoseidonAssert(result >= -INT_MAX);
    if (result < -INT_MAX)
    {
        result = -INT_MAX;
    }
    PoseidonAssert(result <= INT_MAX);
    if (result > INT_MAX)
    {
        result = INT_MAX;
    }
    _time = result;
}

void UITime::operator-=(float diff)
{
    __int64 result = _time;
    result -= ::toLargeInt(1e3 * diff);
    PoseidonAssert(result >= -INT_MAX);
    if (result < -INT_MAX)
    {
        result = -INT_MAX;
    }
    PoseidonAssert(result <= INT_MAX);
    if (result > INT_MAX)
    {
        result = INT_MAX;
    }
    _time = result;
}

float UITime::operator-(UITimeVal src) const
{
    __int64 result = _time;
    result -= src._time;
    PoseidonAssert(result >= -INT_MAX);
    if (result < -INT_MAX)
    {
        result = -INT_MAX;
    }
    PoseidonAssert(result <= INT_MAX);
    if (result > INT_MAX)
    {
        result = INT_MAX;
    }
    return 1e-3 * result;
}

UITime UITime::operator-(float diff) const
{
    UITime ret = *this;
    ret -= diff;
    return ret;
}

UITime UITime::operator+(float diff) const
{
    UITime ret = *this;
    ret += diff;
    return ret;
}

void Time::operator+=(float diff)
{
    __int64 result = _time;
    result += ::toLargeInt(1e3 * diff);
    PoseidonAssert(result >= -INT_MAX);
    if (result < -INT_MAX)
    {
        result = -INT_MAX;
    }
    PoseidonAssert(result <= INT_MAX);
    if (result > INT_MAX)
    {
        result = INT_MAX;
    }
    _time = result;
}

void Time::operator-=(float diff)
{
    __int64 result = _time;
    result -= ::toLargeInt(1e3 * diff);
    PoseidonAssert(result >= -INT_MAX);
    if (result < -INT_MAX)
    {
        result = -INT_MAX;
    }
    PoseidonAssert(result <= INT_MAX);
    if (result > INT_MAX)
    {
        result = INT_MAX;
    }
    _time = result;
}

float Time::operator-(TimeVal src) const
{
    __int64 result = _time;
    result -= src._time;
    PoseidonAssert(result >= -INT_MAX);
    if (result < -INT_MAX)
    {
        result = -INT_MAX;
    }
    PoseidonAssert(result <= INT_MAX);
    if (result > INT_MAX)
    {
        result = INT_MAX;
    }
    return 1e-3 * result;
}

Time Time::operator-(float diff) const
{
    Time ret = *this;
    ret -= diff;
    return ret;
}

Time Time::operator+(float diff) const
{
    Time ret = *this;
    ret += diff;
    return ret;
}

#endif

} // namespace Poseidon::Foundation
