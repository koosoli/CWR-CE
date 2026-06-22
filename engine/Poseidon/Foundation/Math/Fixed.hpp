#pragma once

#include <Poseidon/Foundation/platform.hpp>
#include <Poseidon/Foundation/Common/FltOpts.hpp>
#ifdef _DEBUG
#define FIXED_ENUM 0 // select implementation
#else
#define FIXED_ENUM 0 // select implementation
#endif

// enum is better optimized, class guarantees strict type checking

namespace Poseidon::Foundation
{
inline int fixed_mul(int a, int d)
{
    return ((int)(((long long)a * d) >> 16));
}

#if !FIXED_ENUM

// class implementation is syntactically safe, provides exact type checking,
// but a lot of constructors and operators is not well optimized

#define FixedVal const Fixed&

#ifdef __forceinline
#pragma message("__forceinline defined")
#endif

class Fixed
{
  protected:
    int fx;

  public:
    enum _directFixed
    {
        Fx
    };

  private:
  public:
    __forceinline Fixed() {}
    __forceinline ~Fixed() {}

    // no implicit conversions
    __forceinline Fixed(enum _directFixed, int i) { fx = i; } // constructors are non-portable - do not use directly
    __forceinline explicit Fixed(long l) { fx = static_cast<int>(static_cast<unsigned int>(l) << 16); }
    __forceinline explicit Fixed(int i) { fx = static_cast<int>(static_cast<unsigned int>(i) << 16); }
    __forceinline explicit Fixed(float f) { fx = toLargeInt(f * 65536.0f); }

    __forceinline Fixed(FixedVal a) { fx = a.fx; }
    __forceinline Fixed& operator=(FixedVal a)
    {
        fx = a.fx;
        return *this;
    }

    __forceinline friend int fxIntRaw(FixedVal x) { return x.fx; }

    __forceinline friend int fxToInt(FixedVal x) { return x.fx >> 16; }
    __forceinline friend int fxToIntFloor(FixedVal x) { return x.fx >> 16; }
    __forceinline friend int fxToIntCeil(FixedVal x) { return (int)(x.fx + 0xffff) >> 16; }
    __forceinline friend float fxToFloat(FixedVal x) { return (float)x.fx * (float)(1.0f / 65536.0f); }

    __forceinline Fixed operator+() const { return Fixed(Fx, fx); }
    __forceinline Fixed operator-() const { return Fixed(Fx, static_cast<int>(-static_cast<unsigned int>(fx))); }

    __forceinline Fixed operator+(FixedVal a) const { return Fixed(Fx, static_cast<int>(static_cast<unsigned int>(fx) + static_cast<unsigned int>(a.fx))); }
    __forceinline Fixed operator-(FixedVal a) const { return Fixed(Fx, static_cast<int>(static_cast<unsigned int>(fx) - static_cast<unsigned int>(a.fx))); }
    __forceinline Fixed operator*(FixedVal a) const { return Fixed(Fx, fixed_mul(fx, a.fx)); }

    __forceinline bool operator<(FixedVal a) const { return fx < a.fx; }
    __forceinline bool operator>(FixedVal a) const { return fx > a.fx; }
    __forceinline bool operator<=(FixedVal a) const { return fx <= a.fx; }
    __forceinline bool operator>=(FixedVal a) const { return fx >= a.fx; }
    __forceinline bool operator==(FixedVal a) const { return fx == a.fx; }
    __forceinline bool operator!=(FixedVal a) const { return fx != a.fx; }

    __forceinline Fixed& operator+=(FixedVal a)
    {
        fx = static_cast<int>(static_cast<unsigned int>(fx) + static_cast<unsigned int>(a.fx));
        return *this;
    }
    __forceinline Fixed& operator-=(FixedVal a)
    {
        fx = static_cast<int>(static_cast<unsigned int>(fx) - static_cast<unsigned int>(a.fx));
        return *this;
    }
    __forceinline Fixed& operator*=(FixedVal a)
    {
        fx = fixed_mul(fx, a.fx);
        return *this;
    }

    __forceinline Fixed operator<<(int a) const { return Fixed(Fx, static_cast<int>(static_cast<unsigned int>(fx) << a)); }
    __forceinline Fixed operator>>(int a) const { return Fixed(Fx, fx >> a); }

    __forceinline void operator<<=(int a) { fx = static_cast<int>(static_cast<unsigned int>(fx) << a); }
    __forceinline void operator>>=(int a) { fx >>= a; }

    // optimize mul/div by integer
    __forceinline Fixed operator*(int a) const { return Fixed(Fx, fx * a); }
    __forceinline Fixed operator/(int a) const { return Fixed(Fx, fx / a); }
    __forceinline Fixed& operator*=(int a)
    {
        fx *= a;
        return *this;
    }
    __forceinline Fixed& operator/=(int a)
    {
        fx /= a;
        return *this;
    }

    __forceinline Fixed operator*(float a) const { return Fixed(Fx, toLargeInt(fx * a)); }
};

#define FIXED_MAX Fixed(Fixed::Fx, 0x7fffffff)
#define FIXED_MIN Fixed(Fixed::Fx, -0x80000000)

// standard way to call explicit conversions:
__forceinline Fixed fixed(int x)
{
    return Fixed(x);
}
__forceinline Fixed fixed(float x)
{
    return Fixed(x);
}
__forceinline Fixed fixed(FixedVal x)
{
    return x;
}

static Fixed Fixed0(0);

#include "math.h"

#else

// enum implementation is very fast and simple

typedef enum _fixed
{
    Fixed0 = 0,
    FIXED_MAX = 0x7fffffffL,
    FIXED_MIN = -(0x80000000L),
} Fixed;

// no implicit conversions

__forceinline Fixed fixed(long l)
{
    return Fixed(l << 16);
}
__forceinline Fixed fixed(int i)
{
    return Fixed(i << 16);
}
__forceinline Fixed fixed(Fixed i)
{
    return i;
}
__forceinline Fixed fixed(double d)
{
    int fx = toLargeInt(d * 65536.0);
    return (Fixed)fx;
}
__forceinline Fixed fixed(float d)
{
    int fx = toLargeInt(d * (float)65536.0);
    return (Fixed)fx;
}

__forceinline int fxToInt(Fixed fx)
{
    return (int)fx >> 16;
}
__forceinline int fxToIntFloor(Fixed fx)
{
    return (int)fx >> 16;
}
__forceinline int fxToIntCeil(Fixed fx)
{
    return (int)(fx + 0xffff) >> 16;
}

__forceinline int fxIntRaw(Fixed fx)
{
    return fx;
}

__forceinline float toFloat(Fixed fx)
{
    return (float)(int)fx / (float)(1.0 / 65536.0);
}
__forceinline Fixed frac(Fixed fx)
{
    return Fixed((int)(unsigned short)fx);
}
__forceinline Fixed isFixed(int x)
{
    return Fixed(x);
}

__forceinline Fixed operator+(Fixed a)
{
    return a;
}
__forceinline Fixed operator-(Fixed a)
{
    return Fixed(-(int)a);
}

__forceinline Fixed operator+(Fixed a, Fixed b)
{
    return Fixed((int)a + (int)b);
}
__forceinline Fixed operator-(Fixed a, Fixed b)
{
    return Fixed((int)a - (int)b);
}
__forceinline Fixed operator*(Fixed a, Fixed b)
{
    return Fixed(fixed_mul((int)a, (int)b));
}

// usuall comparison operators use unsigned values

__forceinline bool operator<(Fixed a, Fixed b)
{
    return (int)a < (int)b;
}
__forceinline bool operator>(Fixed a, Fixed b)
{
    return (int)a > (int)b;
}
__forceinline bool operator<=(Fixed a, Fixed b)
{
    return (int)a <= (int)b;
}
__forceinline bool operator>=(Fixed a, Fixed b)
{
    return (int)a >= (int)b;
}
// operators == and != work fine (no difference between singed and unsigned version)

__forceinline Fixed& operator+=(Fixed& a, Fixed b)
{
    a = a + b;
    return a;
}
__forceinline Fixed& operator-=(Fixed& a, Fixed b)
{
    a = a - b;
    return a;
}
__forceinline Fixed& operator*=(Fixed& a, Fixed b)
{
    a = a * b;
    return a;
}

__forceinline Fixed operator<<(Fixed fx, int a)
{
    return Fixed((int)fx << a);
}
__forceinline Fixed operator>>(Fixed fx, int a)
{
    return Fixed((int)fx >> a);
}

__forceinline Fixed& operator<<=(Fixed& fx, int a)
{
    fx = Fixed((int)fx << a);
    return fx;
}
__forceinline Fixed& operator>>=(Fixed& fx, int a)
{
    fx = Fixed((int)fx >> a);
    return fx;
}

// optimize mul/div by integer
__forceinline Fixed operator*(Fixed fx, int a)
{
    return Fixed((int)fx * a);
}
__forceinline Fixed operator/(Fixed fx, int a)
{
    return Fixed((int)fx / a);
}
__forceinline Fixed& operator*=(Fixed& fx, int a)
{
    fx = Fixed((int)fx * a);
    return fx;
}
__forceinline Fixed& operator/=(Fixed& fx, int a)
{
    fx = Fixed((int)fx / a);
    return fx;
}

__forceinline Fixed operator*(Fixed fx, float a)
{
    return Fixed(toLargeInt((int)fx * a));
}

#endif

} // namespace Poseidon::Foundation

using ::Poseidon::Foundation::Fixed;
