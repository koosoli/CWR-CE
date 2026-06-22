#include <Poseidon/Foundation/Platform/AppFrameExt.hpp>
#include <SDL3/SDL.h>

#include <ctime>
#include <stdint.h>
#include <Poseidon/Foundation/Types/Memtype.h>

namespace Poseidon::Foundation
{
bool interrupted = false;

void handleInt(int /*sig*/)
{
    interrupted = true;
}

DWORD CWRFrameFunctions::TickCount()
{
    static int64_t offset = []()
    {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return (int64_t)ts.tv_sec * 1000000000LL + ts.tv_nsec;
    }();
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    int64_t now = (int64_t)ts.tv_sec * 1000000000LL + ts.tv_nsec;
    return (DWORD)((now - offset) / 1000000LL);
}

} // namespace Poseidon::Foundation
