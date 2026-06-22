#include <Poseidon/Foundation/Common/Win.h>

#include <Poseidon/Audio/Core/Format/DeltaPack.hpp>
#include <Poseidon/Foundation/Common/FltOpts.hpp>
#include <cmath>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>

namespace Poseidon
{

// Delta-4 fixed delta table
const int UnpackDelta4::_delta[MaxDeltaInv * 2 + 1] = {-8192, -4096, -2048, -1024, -512, -256, -64,
                                                       0, // 7
                                                       64,    256,   512,   1024,  2048, 4096, 8192};

// Saturate to 16-bit signed range
inline int Sat16S(int val)
{
    if (val > 0x7fff)
    {
        return 0x7fff;
    }
    if (val < -0x8000)
    {
        return -0x8000;
    }
    return val;
}

// UnpackDelta8 implementation

UnpackDelta8::UnpackDelta8()
{
    int i;
    double base = pow(static_cast<double>(MaxDelta), 1.0f / MaxDeltaInv);
    _delta[MaxDeltaInv] = 0;
    for (i = 1; i <= MaxDeltaInv; i++)
    {
        // Exponential delta encoding:
        //        / -base^-x  for x<0  (-127 to -1)
        // f(x)= {   0        for x=0  (0)
        //        \  base^x   for x>0  (1 to 127)
        int v = toInt(pow(base, static_cast<double>(i)));
        _delta[MaxDeltaInv + i] = +v;
        _delta[MaxDeltaInv - i] = -v;
    }
}

int UnpackDelta8::Unpack(short* dest, const char* src, int destSize, int startValue)
{
    PoseidonAssert((destSize & 1) == 0);
    destSize /= 2;
    int aVal = startValue;
    while (--destSize >= 0)
    {
        int idx = static_cast<signed char>(*src++) + MaxDeltaInv;
        // signed char reaches -128, giving idx -1 (the table only covers
        // -MaxDeltaInv..MaxDeltaInv); clamp so a malformed code can't read out of it.
        if (idx < 0)
            idx = 0;
        else if (idx > MaxDeltaInv * 2)
            idx = MaxDeltaInv * 2;
        int v = _delta[idx];
        aVal += v;
        *dest++ = static_cast<short>(Sat16S(aVal));
    }
    return aVal;
}

int UnpackDelta8::Skip(const char* src, int destSize, int startValue)
{
    PoseidonAssert((destSize & 1) == 0);
    destSize /= 2;
    int aVal = startValue;
    while (--destSize >= 0)
    {
        int idx = static_cast<signed char>(*src++) + MaxDeltaInv;
        // signed char reaches -128, giving idx -1 (the table only covers
        // -MaxDeltaInv..MaxDeltaInv); clamp so a malformed code can't read out of it.
        if (idx < 0)
            idx = 0;
        else if (idx > MaxDeltaInv * 2)
            idx = MaxDeltaInv * 2;
        int v = _delta[idx];
        aVal += v;
    }
    return aVal;
}

// UnpackDelta4 implementation

UnpackDelta4::UnpackDelta4() = default;

int UnpackDelta4::Unpack(short* dest, const char* src, int destSize, int startValue)
{
    // Two samples at a time (4 bits each)
    PoseidonAssert((destSize & 3) == 0);
    destSize /= 2;
    int aVal = startValue;
    while ((destSize -= 2) >= 0)
    {
        int v = *src++;
        int v1 = (v >> 4) & 0xf;
        int v2 = (v & 0xf);
        // The 4-bit code reaches 15 but the table has 2*MaxDeltaInv+1 entries
        // (0..14); clamp so a malformed nibble can't read past it.
        if (v1 > MaxDeltaInv * 2)
            v1 = MaxDeltaInv * 2;
        if (v2 > MaxDeltaInv * 2)
            v2 = MaxDeltaInv * 2;
        v1 = _delta[v1];
        v2 = _delta[v2];
        aVal += v1;
        *dest++ = static_cast<short>(Sat16S(aVal));
        aVal += v2;
        *dest++ = static_cast<short>(Sat16S(aVal));
    }
    return aVal;
}

int UnpackDelta4::Skip(const char* src, int destSize, int startValue)
{
    // Two samples at a time
    PoseidonAssert((destSize & 3) == 0);
    destSize /= 2;
    int aVal = startValue;
    while ((destSize -= 2) >= 0)
    {
        int v = *src++;
        int v1 = (v >> 4) & 0xf;
        int v2 = (v & 0xf);
        // The 4-bit code reaches 15 but the table has 2*MaxDeltaInv+1 entries
        // (0..14); clamp so a malformed nibble can't read past it.
        if (v1 > MaxDeltaInv * 2)
            v1 = MaxDeltaInv * 2;
        if (v2 > MaxDeltaInv * 2)
            v2 = MaxDeltaInv * 2;
        v1 = _delta[v1];
        v2 = _delta[v2];
        aVal += v1;
        aVal += v2;
    }
    return aVal;
}

// Global instances
UnpackDelta8 UnpackD8;
UnpackDelta4 UnpackD4;

} // namespace Poseidon
