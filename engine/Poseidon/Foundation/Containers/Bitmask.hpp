#pragma once

#include <Poseidon/Foundation/Types/Pointers.hpp>
#include <Poseidon/Foundation/Common/Global.hpp>


namespace Poseidon::Foundation
{
class BitMaskMTS;

#define MaskHigh(addr) mask[(addr) >> 5]

#define MaskLow(addr) (1 << ((addr) & 31))

class BitMask : public RefCount
{
  protected:
    // One bit per value in one continuous /32-bit aligned/ memory segment.
    unsigned32* mask;

    // Number of allocated mask[] items (in 32-bit words).
    int allocMask;

    // Invariants: (max-min)/32 <= allocMask, (max-min) == k * 32
    int min, max;

    virtual unsigned32* newArray(int size);

    virtual void deleteArray(unsigned32*& array);

    friend class BitMaskMTS;

  public:
    // Sentinel returned by getFirst()/getNext() when no bit is set.
    static const int END;

    BitMask();

    BitMask(const BitMask& b);

    BitMask& operator=(const BitMask& b);

    ~BitMask() override;

    void set(int value, bool flag);

    void on(int value);

    void off(int value);

    void reserve(int value);

    void compact();

    void growOptimize(bool up, int anchor = END);

    inline void setUnsafe(int value, bool flag)
    {
        value -= min;
        if (flag)
        {
            MaskHigh(value) |= MaskLow(value);
        }
        else
        {
            MaskHigh(value) &= ~MaskLow(value);
        }
    }

    void range(int from, int len, bool flag);

    inline bool get(int value) const
    {
        if (value < min || value >= max)
        {
            return false;
        }
        value -= min;
        return ((MaskHigh(value) & MaskLow(value)) != 0);
    }

    inline bool getUnsafe(int value) const
    {
        value -= min;
        return ((MaskHigh(value) & MaskLow(value)) != 0);
    }

    // Resets to empty, freeing all memory.
    void empty();

    void emptyOptimize(bool up, int origin = END);

    int card() const;

    int getFirst() const;

    int getNext(int i) const;

    int getLast() const;

    BitMask& operator|=(const BitMask& b);

    BitMask& operator&=(const BitMask& b);

    BitMask& operator^=(const BitMask& b);

    BitMask& operator-=(const BitMask& b);

    void getStat(int& minimum, int& maximum);
};

// Variable-size bit-mask with multi-threaded safety.
class BitMaskMTS : public BitMask
{
  protected:
    unsigned32* newArray(int size) override;

    void deleteArray(unsigned32*& array) override;

  public:
    BitMaskMTS();

    BitMaskMTS(const BitMaskMTS& b);

    BitMaskMTS(const BitMask& b);

    BitMaskMTS& operator=(const BitMaskMTS& b);

    BitMaskMTS& operator=(const BitMask& b);

    ~BitMaskMTS() override;
};

} // namespace Poseidon::Foundation


using ::Poseidon::Foundation::BitMask;
using ::Poseidon::Foundation::BitMaskMTS;
