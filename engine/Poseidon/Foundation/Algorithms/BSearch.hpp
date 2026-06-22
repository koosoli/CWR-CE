#pragma once

#ifndef POSEIDON_BASE_ALGORITHMS_BSEARCH_HPP
#define POSEIDON_BASE_ALGORITHMS_BSEARCH_HPP

#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <cstddef>

// Binary search. Compare is a functor: int(KeyType key, Type value), neg/0/pos.
// Returns the index of some element equal to key, or -1 if none.

namespace Poseidon::Foundation
{
template <class Type, class KeyType, class Compare>
[[nodiscard]] int BSearch(const Type* data, size_t num, Compare compare, KeyType key)
{
    if (num == 0)
    {
        return -1;
    }

    int lo = 0;
    int hi = static_cast<int>(num) - 1;
    int mid;
    size_t half;
    int result;

    while (lo <= hi)
    {
        if ((half = num / 2))
        {
            mid = lo + (num & 1 ? half : (half - 1));
            if (!(result = compare(key, data[mid])))
            {
                return mid;
            }
            else if (result < 0)
            {
                hi = mid - 1;
                num = num & 1 ? half : half - 1;
            }
            else
            {
                lo = mid + 1;
                num = half;
            }
        }
        else if (num)
        {
            return compare(key, data[lo]) ? -1 : lo;
        }
        else
        {
            break;
        }
    }
    return -1;
}

// Like BSearch, but returns an insertion position: the index of some element
// equal to key, else the first element greater than key, else num.
template <class Type, class KeyType, class Compare>
[[nodiscard]] int BSearchPos(const Type* data, size_t num, Compare compare, KeyType key)
{
    if (num == 0)
    {
        return 0;
    }

    unsigned int lo = 0;
    unsigned int hi = static_cast<unsigned int>(num - 1);
    unsigned int mid;

    // always true:
    // key <= data[hi] or key>data[num-1]
    // key >= data[lo] or key<data[0]
    while (lo <= hi)
    {
        const size_t half = num / 2;
        if (half)
        {
            mid = lo + (num & 1 ? half : (half - 1));
            int result = compare(key, data[mid]);
            if (result == 0)
            {
                return mid;
            }
            else if (result < 0)
            {
                hi = mid - 1;
                num = num & 1 ? half : half - 1;
            }
            else
            {
                lo = mid + 1;
                num = half;
            }
        }
        else if (num)
        {
            // half==0 && num!=0 => num==1
            PoseidonAssert(hi == lo) int result = compare(key, data[lo]);
            if (result <= 0)
            {
                return lo;
            }
            return hi + 1;
        }
        else
        {
            return lo;
        }
    }
    return lo;
}

} // namespace Poseidon::Foundation

#endif
