#pragma once

#include <cstddef>
#include <cstdio>

namespace Poseidon
{

inline bool ShouldShowGroupUnitLeaderDebugSuffix()
{
#ifdef NDEBUG
    return false;
#else
    return true;
#endif
}

inline void FormatGroupUnitLabel(char* output, size_t outputSize, int id, int leader, bool showDebugLeaderSuffix)
{
    if (showDebugLeaderSuffix && leader >= 0)
    {
        std::snprintf(output, outputSize, "%d(%d)", id, leader);
    }
    else
    {
        std::snprintf(output, outputSize, "%d", id);
    }
}

} // namespace Poseidon
