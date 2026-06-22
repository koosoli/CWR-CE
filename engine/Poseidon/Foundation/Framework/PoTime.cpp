#include <Poseidon/Foundation/Framework/PoTime.hpp>
#ifndef _WIN32
#include <sys/select.h>
#endif
#include <Poseidon/Foundation/platform.hpp>

#ifdef _WIN32

#include <Poseidon/Foundation/Common/Win.h>
#if _MSC_VER < 1300
#include <largeint.h>
#endif
#include <Poseidon/Foundation/Common/Global.hpp>

namespace Poseidon::Foundation
{
static LARGE_INTEGER hpcFrequency;

static bool isHpc = false;

// Meyer's singleton pattern - initialization on first use
static bool& GetIsHpcInitialized()
{
    static bool initialized = false;
    return initialized;
}

void startSystemTime()
// check the high-performance counter
{
    if (GetIsHpcInitialized())
    {
        return;
    }
    LARGE_INTEGER frequency;
    isHpc = (QueryPerformanceFrequency(&frequency) != 0);
    if (isHpc)
    {
        hpcFrequency = frequency;
    }
    GetIsHpcInitialized() = true;
}

unsigned getClockFrequency()
// clock frequency in Hz
{
    startSystemTime(); // Ensure initialization
    return (isHpc ? (hpcFrequency.HighPart ? UINT_MAX : hpcFrequency.LowPart) : 100);
}

unsigned64 getSystemTime()
// returns actual system time in micro-seconds
{
    startSystemTime(); // Ensure initialization
    if (isHpc)
    {
        LARGE_INTEGER count;
        if (QueryPerformanceCounter(&count))
        {
#if _MSC_VER >= 1300
            LARGE_INTEGER sec;
            LARGE_INTEGER remainder;
            sec.QuadPart = count.QuadPart / hpcFrequency.QuadPart;
            remainder.QuadPart = count.QuadPart % hpcFrequency.QuadPart;
#else
            LARGE_INTEGER remainder;
            LARGE_INTEGER sec = LargeIntegerDivide(count, hpcFrequency, &remainder);
#endif
            // time = sec + (remainder/hpcFrequency)
            return ((unsigned64)sec.QuadPart * 1000000U +
                    ((unsigned64)remainder.QuadPart * 1000000U) / (unsigned64)hpcFrequency.QuadPart);
        }
    }
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    return (((((unsigned64)ft.dwHighDateTime) << 32) + ft.dwLowDateTime + 5) / 10);
}

// Section timing — uses QPC directly for maximum precision

SectionTimeHandle StartSectionTime()
{
    LARGE_INTEGER count;
    QueryPerformanceCounter(&count);
    return count.QuadPart;
}

int64_t GetSectionResolution()
{
    startSystemTime();
    return hpcFrequency.QuadPart;
}

float GetSectionTime(SectionTimeHandle section)
{
    startSystemTime();
    LARGE_INTEGER count;
    QueryPerformanceCounter(&count);
    return static_cast<float>(count.QuadPart - section) / static_cast<float>(hpcFrequency.QuadPart);
}

bool CompareSectionTimeGE(SectionTimeHandle section, float time)
{
    startSystemTime();
    LARGE_INTEGER count;
    QueryPerformanceCounter(&count);
    return count.QuadPart >= section + static_cast<int64_t>(time * static_cast<float>(hpcFrequency.QuadPart));
}

#else

#include <sys/time.h>
#include <time.h>
#include <Poseidon/Foundation/Common/Global.hpp>

namespace Poseidon::Foundation
{

void startSystemTime() {}

unsigned getClockFrequency()
// clock frequency in Hz
{
    return 100;
}

#ifdef HAS_CLOCK_GETTIME

// "clock_gettime()" is defined:

unsigned64 getSystemTime()
// returns actual system time in micro-seconds
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (ts.tv_sec * 1000000L + (ts.tv_nsec + 500L) / 1000L);
}

#elif defined(HAS_GETTIMEOFDAY)

// "gettimeofday()" is defined:

unsigned64 getSystemTime()
// returns actual system time in micro-seconds
{
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return ((unsigned64)tv.tv_sec * 1000000LL + tv.tv_usec);
}

#else

// neither "clock_gettime()" nor "gettimeofday()" defined:

#include <sys/timeb.h>

unsigned64 getSystemTime()
// returns actual system time in micro-seconds
{
    struct timeb tb;
    ftime(&tb);
    return (1000 * (tb.millitm + 1000 * (unsigned64)tb.time));
}

#endif

void sleepMs(unsigned ms)
{
    struct timeval timeout = {ms / 1000L, (ms % 1000L) * 1000L}; // seconds, micro-seconds
    select(0, nullptr, nullptr, nullptr, &timeout);
}

// Section timing — uses CLOCK_MONOTONIC for elapsed time measurement

static int64_t GetMonotonicNanos()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<int64_t>(ts.tv_sec) * 1000000000LL + ts.tv_nsec;
}

static const int64_t kNsPerSec = 1000000000LL;

SectionTimeHandle StartSectionTime()
{
    return GetMonotonicNanos();
}

int64_t GetSectionResolution()
{
    return kNsPerSec;
}

float GetSectionTime(SectionTimeHandle section)
{
    return static_cast<float>(GetMonotonicNanos() - section) / static_cast<float>(kNsPerSec);
}

bool CompareSectionTimeGE(SectionTimeHandle section, float time)
{
    return GetMonotonicNanos() >= section + static_cast<int64_t>(time * kNsPerSec);
}

#endif

} // namespace Poseidon::Foundation
