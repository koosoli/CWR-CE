#pragma once

#include <Poseidon/Foundation/Framework/DebugLog.hpp>

// Optional bounds-check hook for landscape access; defined empty (no checking).
#define ASSERT_RANGE(x, y)


namespace Poseidon::Foundation
{
template <class Type, int dimX, int dimY>
class BigArrayNormal
{
    Type _array[dimY][dimX];

  public:
    Type& operator()(int x, int y)
    {
        ASSERT_RANGE(x, y);
        return _array[y][x];
    }
    const Type& operator()(int x, int y) const
    {
        ASSERT_RANGE(x, y);
        return _array[y][x];
    }
    void GetFour(Type res[2][2], int x, int y) const;
};

template <class Type, int dimX, int dimY>
void BigArrayNormal<Type, dimX, dimY>::GetFour(Type res[2][2], int x, int y) const
{
    ASSERT_RANGE(x, y);
    ASSERT_RANGE(x + 1, y + 1);
    res[0][0] = _array[y][x];
    res[0][1] = _array[y][x + 1];
    res[1][0] = _array[y + 1][x];
    res[1][1] = _array[y + 1][x + 1];
}

const int elemLog = 2;

template <class Type, int dimX, int dimY>
class BigArray
{
    // note: dimX, dimY must be power of 2
    enum
    {
        elem = 1 << elemLog,
        elemMask = elem - 1,
        bigX = (dimX + elemMask) >> elemLog,
        bigY = (dimY + elemMask) >> elemLog
    };

    Type _array[bigY][bigX][elem][elem];
    typedef Type SArray[elem][elem];

  public:
    Type& operator()(int x, int y)
    {
        int bx = x >> elemLog, by = y >> elemLog;
        return _array[by][bx][y & elemMask][x & elemMask];
    }
    const Type& operator()(int x, int y) const
    {
        int bx = x >> elemLog, by = y >> elemLog;
        return _array[by][bx][y & elemMask][x & elemMask];
    }
    void GetFour(Type res[2][2], int x, int y) const;
};

template <class Type, int dimX, int dimY>
void BigArray<Type, dimX, dimY>::GetFour(Type res[2][2], int x, int y) const
{ // note: all four corners must be in range
    AssertDebug(x + 1 < dimX);
    AssertDebug(y + 1 < dimY);
    const SArray* yLine;
    int bx = x >> elemLog, sx = x & elemMask;
    int by = y >> elemLog, sy = y & elemMask;
    x++, y++;
    yLine = _array[by];
    res[0][0] = yLine[bx][sy][sx];
    int bx1 = x >> elemLog, sx1 = x & elemMask;
    res[0][1] = yLine[bx1][sy][sx1];
    by = y >> elemLog, sy = y & elemMask;
    yLine = _array[by];
    res[1][0] = yLine[bx][sy][sx];
    res[1][1] = yLine[bx1][sy][sx1];
}

} // namespace Poseidon::Foundation

using ::Poseidon::Foundation::BigArray;
using ::Poseidon::Foundation::BigArrayNormal;
