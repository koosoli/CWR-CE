#include <Poseidon/Foundation/Containers/Bitmask.hpp>
#include <string.h>
#include <Poseidon/Foundation/Common/Global.hpp>
#include <Poseidon/Foundation/Memory/CheckMem.hpp>

namespace Poseidon::Foundation
{

const int BitMask::END = 0x7FFFFFFF;

BitMask::BitMask()
{
    mask = nullptr;
    allocMask = min = max = 0;
}

BitMask::BitMask(const BitMask& b)
{
    mask = nullptr;
    this->operator=(b);
}

unsigned32* BitMask::newArray(int size)
{
    return (size ? new unsigned32[size] : (unsigned32*)nullptr);
}

void BitMask::deleteArray(unsigned32*& array)
{
    if (array)
    {
        delete[] array;
        array = nullptr;
    }
}

BitMask& BitMask::operator=(const BitMask& b)
{
    if (this == &b)
    {
        return *this;
    }
    empty();
    min = b.min;
    max = b.max;
    // NOLINTNEXTLINE(bugprone-assignment-in-if-condition) - Intentional: assign and test in one statement
    if ((allocMask = b.allocMask) != 0)
    {
        mask = newArray(allocMask);
        memcpy(mask, b.mask, allocMask << 2);
    }
    return *this;
}

void BitMask::empty()
{
    deleteArray(mask);
    allocMask = min = max = 0;
}

BitMask::~BitMask()
{
    empty();
}

void BitMask::reserve(int value)
{
    int add;
    unsigned32* newMask;
    if (!mask)
    {
        mask = newArray(allocMask = 1);
        mask[0] = 0;
        min = (value & -32);
        max = min + 32;
        return;
    }
    if (value < min)
    {
        add = (min - value + 31) >> 5;
        newMask = newArray(allocMask + add);
        memset(newMask, 0, add << 2);
        memcpy(newMask + add, mask, allocMask << 2);
        deleteArray(mask);
        mask = newMask;
        allocMask += add;
        min -= add << 5;
        return;
    }
    if (value >= max)
    {
        add = (value - min + 32) >> 5;
        if (add <= allocMask)
        {
            return;
        }
        newMask = newArray(add);
        memcpy(newMask, mask, allocMask << 2);
        deleteArray(mask);
        memset(newMask + allocMask, 0, (add - allocMask) << 2);
        mask = newMask;
        allocMask = add;
        max = min + (add << 5);
    }
}

void BitMask::compact()
{
    int mMin = getFirst();
    if (mMin == END)
    {
        empty();
        return;
    }
    mMin &= -32;
    int mMax = (getLast() + 32) & -32;
    int newAlloc = (mMax - mMin) >> 5;
    if (newAlloc == allocMask)
    {
        return;
    }
    unsigned32* newMask = newArray(newAlloc);
    memcpy(newMask, mask + ((mMin - min) >> 5), newAlloc << 2);
    deleteArray(mask);
    mask = newMask;
    allocMask = newAlloc;
    min = mMin;
    max = mMax;
}

void BitMask::growOptimize(bool up, int anchor)
{
    int m, words, shift;
    if (up)
    { // shift all 1-s to the left
        m = getFirst();
        if (m == END)
        { // the mask is empty..
            if (anchor != END)
            { // anchor the array
                shift = (anchor & -32) - min;
                min += shift;
                max += shift;
            }
        }
        else // the mask is non-empty => shift all 1-s to the left
            // NOLINTNEXTLINE(bugprone-assignment-in-if-condition) - Intentional: assign and test in one statement
            if ((shift = m - min) >= 32)
            { // any free space for the shift?
                shift &= -32;
                words = (shift >> 5);
                memmove(mask, mask + words, (allocMask - words) << 2);
                memset(mask + allocMask - words, 0, words << 2);
                min += shift;
                max += shift;
            }
    }
    else
    { // shift all 1-s to the right
        m = getLast();
        if (m == END)
        { // the mask is empty..
            if (anchor != END)
            { // anchor the array
                shift = ((anchor + 31) & -32) - max;
                min += shift;
                max += shift;
            }
        }
        else // the mask is non-empty => shift all 1-s to the right
            // NOLINTNEXTLINE(bugprone-assignment-in-if-condition) - Intentional: assign and test in one statement
            if ((shift = max - m + 1) >= 32)
            { // any free space for the shift?
                shift &= -32;
                words = (shift >> 5);
                memmove(mask + words, mask, (allocMask - words) << 2);
                memset(mask, 0, words << 2);
                min -= shift;
                max -= shift;
            }
    }
}

void BitMask::emptyOptimize(bool up, int origin)
{
    if (!allocMask)
    {
        return; // nothing to do..
    }
    if (max - min > 32)
    {
        if (up)
        {
            if (origin == END)
            {
                min = max - 32;
            }
            else
            {
                min = (origin & -32);
            }
            max = min + (allocMask << 5);
        }
        else
        {
            if (origin == END)
            {
                max = min + 32;
            }
            else
            {
                max = (origin & -32) + 32;
            }
            min = max - (allocMask << 5);
        }
    }
    memset(mask, 0, allocMask << 2);
}

void BitMask::set(int value, bool flag)
{
    if (flag)
    {
        reserve(value);
        value -= min;
        MaskHigh(value) |= MaskLow(value);
        return;
    }
    if (value < min || value >= max)
    {
        return;
    }
    value -= min;
    MaskHigh(value) &= ~MaskLow(value);
}

void BitMask::on(int value)
{
    reserve(value);
    value -= min;
    MaskHigh(value) |= MaskLow(value);
}

void BitMask::off(int value)
{
    if (value < min || value >= max)
    {
        return;
    }
    value -= min;
    MaskHigh(value) &= ~MaskLow(value);
}

void BitMask::range(int from, int len, bool flag)
{
    if (len <= 0)
    {
        return;
    }
    if (flag)
    {
        reserve(from);
        reserve(from + len - 1);
    }
    else
    {
        if (from < min)
        {
            len -= min - from;
            from = min;
        }
        if (from + len > max)
        {
            len = max - from;
        }
    }
    if (len >= 64)
    { // optimized solution for large ranges
        while (from & 31)
        {
            setUnsafe(from++, flag);
            len--;
        }
        unsigned32 val = flag ? 0xffffffff : 0;
        from -= min;
        do
        {
            MaskHigh(from) = val;
            from += 32;
        } while ((len -= 32) >= 32);
        from += min;
    }
    while (len-- > 0)
    {
        setUnsafe(from++, flag);
    }
}

int BitMask::card() const
{
    if (!allocMask)
    {
        return 0;
    }
    int count = 0;
    unsigned32* pm = mask;
    for (int i = 0; i++ < allocMask;)
    {
        unsigned32 act = *pm++;
        // NOLINTNEXTLINE(bugprone-infinite-loop) - False positive: act is modified by right shift below
        while (act)
        {
            count += (act & 1);
            act >>= 1;
        }
    }
    return count;
}

int BitMask::getFirst() const
{
    if (!mask)
    {
        return END;
    }
    return getNext(min - 1);
}

int BitMask::getNext(int i) const
{
    int ii;
    unsigned32 b;
    while (++i < max)
    {
        if ((b = MaskHigh(ii = i - min)))
        {
            if (b & MaskLow(ii))
            {
                return i;
            }
        }
        else
        {
            i += 32 - (ii & 31);
            ii >>= 5;
            while (i < max && !mask[++ii])
            {
                i += 32;
            }
            i--;
        }
    }
    return END;
}

int BitMask::getLast() const
{
    if (!mask)
    {
        return END;
    }
    int ii = ((max - min) >> 5) - 1;
    while (ii >= 0 && !mask[ii])
    {
        ii--;
    }
    if (ii < 0)
    {
        return END;
    }
    unsigned32 b = mask[ii];
    ii = (ii << 5) + min + 31;
    // NOLINTNEXTLINE(bugprone-infinite-loop) - False positive: b is modified by left shift below
    while (!(b & 0x80000000))
    {
        b += b;
        ii--;
    }
    return ii;
}

BitMask& BitMask::operator|=(const BitMask& b)
{
    int bFirst = b.getFirst();
    if (bFirst == END)
    {
        return *this;
    }
    int bLast = b.getLast();
    reserve(bFirst);
    reserve(bLast);
    bFirst &= -32;                      // round the value down to unsigned32 boundary
    bLast = (bLast - bFirst + 32) >> 5; // processed length in mask[] items
    const unsigned32* bMask = b.mask + ((bFirst - b.min) >> 5);
    unsigned32* myMask = mask + ((bFirst - min) >> 5);
    do
    {
        *myMask++ |= *bMask++;
    } while (--bLast);
    return *this;
}

BitMask& BitMask::operator&=(const BitMask& b)
{
    if (!mask)
    {
        return *this;
    }
    int bFirst = b.getFirst();
    if (bFirst == END)
    {
        empty();
        return *this;
    }
    int bLast = b.getLast();
    reserve(bFirst);
    reserve(bLast);
    if (bFirst > min)
    {
        range(min, bFirst - min, false);
    }
    if (bLast < max - 1)
    {
        range(bLast + 1, max - bLast - 1, false);
    }
    bFirst &= -32;                      // round the value down to unsigned32 boundary
    bLast = (bLast - bFirst + 32) >> 5; // processed length in mask[] items
    const unsigned32* bMask = b.mask + ((bFirst - b.min) >> 5);
    unsigned32* myMask = mask + ((bFirst - min) >> 5);
    do
    {
        *myMask++ &= *bMask++;
    } while (--bLast);
    return *this;
}

BitMask& BitMask::operator^=(const BitMask& b)
{
    int bFirst = b.getFirst();
    if (bFirst == END)
    {
        return *this;
    }
    int bLast = b.getLast();
    reserve(bFirst);
    reserve(bLast);
    bFirst &= -32;                      // round the value down to unsigned32 boundary
    bLast = (bLast - bFirst + 32) >> 5; // processed length in mask[] items
    const unsigned32* bMask = b.mask + ((bFirst - b.min) >> 5);
    unsigned32* myMask = mask + ((bFirst - min) >> 5);
    do
    {
        *myMask++ ^= *bMask++;
    } while (--bLast);
    return *this;
}

BitMask& BitMask::operator-=(const BitMask& b)
{
    int bFirst = b.getFirst();
    if (bFirst == END)
    {
        return *this;
    }
    int bLast = b.getLast();
    reserve(bFirst);
    reserve(bLast);
    bFirst &= -32;                      // round the value down to unsigned32 boundary
    bLast = (bLast - bFirst + 32) >> 5; // processed length in mask[] items
    const unsigned32* bMask = b.mask + ((bFirst - b.min) >> 5);
    unsigned32* myMask = mask + ((bFirst - min) >> 5);
    do
    {
        *myMask++ &= ~(*bMask++);
    } while (--bLast);
    return *this;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - Parameter names (minimum/maximum) are self-explanatory
void BitMask::getStat(int& minimum, int& maximum)
{
    minimum = min;
    maximum = max;
}

BitMaskMTS::BitMaskMTS() = default;

BitMaskMTS::BitMaskMTS(const BitMaskMTS& b)
{
    mask = nullptr;
    this->operator=(b);
}

BitMaskMTS::BitMaskMTS(const BitMask& b)
{
    mask = nullptr;
    this->operator=(b);
}

unsigned32* BitMaskMTS::newArray(int size)
{
    return (size ? (unsigned32*)safeNew(size << 2) : (unsigned32*)nullptr);
}

void BitMaskMTS::deleteArray(unsigned32*& array)
{
    if (array)
    {
        safeDelete(array);
        array = nullptr;
    }
}

BitMaskMTS& BitMaskMTS::operator=(const BitMaskMTS& b)
{
    return operator=(static_cast<const BitMask&>(b));
}

BitMaskMTS& BitMaskMTS::operator=(const BitMask& b)
{
    empty();
    min = b.min;
    max = b.max;
    // NOLINTNEXTLINE(bugprone-assignment-in-if-condition) - Intentional: assign and test in one statement
    if ((allocMask = b.allocMask) != 0)
    {
        mask = newArray(allocMask);
        memcpy(mask, b.mask, allocMask << 2);
    }
    return *this;
}

BitMaskMTS::~BitMaskMTS()
{
    deleteArray(mask);
    allocMask = min = max = 0;
}

} // namespace Poseidon::Foundation
