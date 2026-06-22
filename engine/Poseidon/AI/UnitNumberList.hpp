#pragma once

#include <cstdio>

#include <Poseidon/Foundation/Containers/BoolArray.hpp>
#include <Poseidon/Foundation/Strings/RString.hpp>

// Join the 1-based positions of the set bits in `units` (scanning indices
// 0..count-1) into a comma-separated list: units {2,3,6,7,8} render as
// "2, 3, 6, 7, 8". Returns an empty string when no bit is set. Building the
// list as an RString keeps every id separated regardless of the running
// length, so adjacent ids never run together.

namespace Poseidon
{

inline RString JoinUnitNumbers(const PackedBoolArray& units, int count)
{
    RString result;
    for (int i = 0; i < count; i++)
    {
        if (!units.Get(i))
        {
            continue;
        }
        char number[16];
        snprintf(number, sizeof(number), "%d", i + 1);
        result = result.GetLength() > 0 ? result + ", " + number : RString(number);
    }
    return result;
}

} // namespace Poseidon
