#pragma once

#include <Poseidon/Foundation/platform.hpp>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <cmath>
#include <limits.h>
#include <math.h>
#include <utility>

// fast float helpers

inline float floatMax(float a, float b)
{
    return (a > b ? a : b);
}
inline float floatMin(float a, float b)
{
    return (a < b ? a : b);
}

#define saturateAbove saturateMin
#define saturateBelow saturateMax

inline void saturateMin(float& a, float b)
{
    if (a > b)
    {
        a = b;
    }
}
inline void saturateMax(float& a, float b)
{
    if (a < b)
    {
        a = b;
    }
}

inline void saturateMin(int& a, int b)
{
    if (a > b)
    {
        a = b;
    }
}
inline void saturateMax(int& a, int b)
{
    if (a < b)
    {
        a = b;
    }
}

// Note: Square() is defined in Poseidon/Foundation/Math/MathOpt.hpp but needed here for splineEqD.hpp
// This is a temporary workaround - should refactor algorithm dependencies
template <class T>
inline T Square(T x)
{
    return x * x;
}

#ifdef __ICL

#define floatMaxFast floatMax
#define floatMinFast floatMin

inline void saturate(float& a, float min, float max)
{
    if (a < min)
        a = min;
    if (a > max)
        a = max;
}

#define saturateFast saturate

inline void saturate(int& a, int min, int max)
{
    if (a < min)
        a = min;
    if (a > max)
        a = max;
}

#else

inline float floatMaxFast(double a, double b)
{
    return float((a + b + fabs(a - b)) * 0.5);
}
inline float floatMinFast(double a, double b)
{
    return float((a + b - fabs(a - b)) * 0.5);
}

inline void saturate(float& a, float min, float max)
{
    if (a < min)
    {
        a = min;
    }
    else if (a > max)
    {
        a = max;
    }
}

inline void saturateFast(float& a, float min, float max)
{
    a = floatMaxFast(a, min);
    a = floatMinFast(a, max);
}

inline void saturate(int& a, int min, int max)
{
    if (a < min)
    {
        a = min;
    }
    else if (a > max)
    {
        a = max;
    }
}

#endif

static const float Inv2 = 0.5;

inline float fastRound(float x)
{
    // Round-to-nearest, ties-to-even.  `(int)x` would truncate toward zero,
    // which silently breaks `fastFloor(x) = fastRound(x - 0.5)` for positive
    // non-integer inputs whose fractional part is < 0.5 (returns
    // `floor(x) - 1` instead of `floor(x)`).  That off-by-one surfaced as
    // Czech lip animation terminating ~0.4s into every voice line:
    // `GetPhase`'s `floor` landed one frame behind, `dif` overflowed
    // `_frame`, the interpolation `(frame - dif)*oldPhase` flipped negative,
    // and `Head::Animate` cleared `_lipInfo`.
    return std::nearbyint(x);
}

inline int toLargeInt(float f)
{
    // Round to nearest, ties-to-even - the original x86 x87 `fistp` behaviour
    // (the DX8 reference); `(int)f` would truncate.
    // Clamp before cast: x87 fistp silently saturated on overflow; C++ UB.
    float r = std::nearbyint(f);
    if (r >= 2147483648.0f)  return 2147483647;   // INT_MAX
    if (r < -2147483648.0f)  return -2147483648;  // INT_MIN
    return static_cast<int>(r);
}

inline __int64 to64bInt(float f)
{
    return (__int64)std::nearbyint(f);
}

inline int toInt(float fval)
{
    // Round to nearest, ties-to-even - the x86 FPU default that the DX8
    // reference uses.  nearbyint honours the current rounding mode and lowers
    // to a single cvtss2si.
    float r = std::nearbyint(fval);
    if (r >= 2147483648.0f)  return 2147483647;
    if (r < -2147483648.0f)  return -2147483648;
    return static_cast<int>(r);
}

inline int toInt(double f)
{
    return toInt(float(f));
}

#define toIntFloor(x) toInt((x) - Inv2)
#define toIntCeil(x) toInt((x) + Inv2)

#define fastCeil(x) fastRound((x) + Inv2)
#define fastFloor(x) fastRound((x) - Inv2)

inline float fastFmod(float x, const float n)
{ // n is often constant expression
    x *= 1 / n;
    x -= toIntFloor(x); // nearest int
    return x * n;
}

#if _MSC_VER > 1300
// MSVC-only overloads to disambiguate int→float propagation
__forceinline int abs(unsigned x)
{
    return abs(int(x));
}

__forceinline float fabs(int x)
{
    return static_cast<float>(fabs(float(x)));
} // Explicit cast: int→float→double→float
__forceinline float log(int x)
{
    return static_cast<float>(log(float(x)));
} // Explicit cast: int→float→double→float
__forceinline float sqrt(int x)
{
    return static_cast<float>(sqrt(float(x)));
} // Explicit cast: int→float→double→float

#endif

#define FP_BITS(fp) (*(DWORD*)&(fp))
#define FP_ABS_BITS(fp) (FP_BITS(fp) & 0x7FFFFFFF)
#define FP_SIGN_BIT(fp) (FP_BITS(fp) & 0x80000000)
#define FP_ONE_BITS 0x3F800000

// fast reciprocal approximation
inline float FastInv(float p)
{
    // about 6b precision?
    int _i = 2 * FP_ONE_BITS - *(int*)&(p);
    float r = *(float*)&_i;
    return r * (2.0f - (p)*r);
}

// Disables copying: declares (but does not define) the copy constructor and operator.
// Use at the end of a class definition.
#define NoCopy(type)   \
  private:             \
    type(const type&); \
    type& operator=(const type&);

using std::swap;

inline char myLower(char c)
{
    c += char(CHAR_MIN - 'A');
    if (c <= CHAR_MIN + 'Z' - 'A')
    {
        c += 'a' - 'A';
    }
    c -= char(CHAR_MIN - 'A');
    return c;
}

inline char myUpper(char c)
{
    c += char(CHAR_MIN - 'a');
    if (c <= CHAR_MIN + 'z' - 'a')
    {
        c += 'A' - 'a';
    }
    c -= char(CHAR_MIN - 'a');
    return c;
}

template <int Size>
class SizedMemory
{
    char _data[Size];
};

#define FastCpy(dst, src, size) *(SizedMemory<size>*)(dst) = *(SizedMemory<size>*)(src)

// Prefetch hints. off must be < 0x80; adr is a pointer or reference.
// Uses SSE intrinsics (works on x86 and x64).
#if defined(_M_IX86) || defined(_M_X64)
#include <xmmintrin.h> // SSE intrinsics

// Non-temporal prefetch (streaming data)
#define PrefetchNTA(adr) _mm_prefetch((const char*)(adr), _MM_HINT_NTA)
#define PrefetchNTAOff(adr, off) _mm_prefetch((const char*)(adr) + (off), _MM_HINT_NTA)

// L1 cache prefetch (hot data)
#define PrefetchT0(adr) _mm_prefetch((const char*)(adr), _MM_HINT_T0)
#define PrefetchT0Off(adr, off) _mm_prefetch((const char*)(adr) + (off), _MM_HINT_T0)
#else
// No prefetch on unsupported architectures
#define PrefetchNTA(adr)
#define PrefetchNTAOff(adr, off)
#define PrefetchT0(adr)
#define PrefetchT0Off(adr, off)
#endif

