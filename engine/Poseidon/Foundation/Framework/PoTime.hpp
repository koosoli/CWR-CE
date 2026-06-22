#pragma once

#include <cstdint>
#include <Poseidon/Foundation/Common/Global.hpp>

namespace Poseidon::Foundation
{
// Initialize the system timer (high-performance timer on Win32).
extern void startSystemTime();

// System clock (high-resolution real-time clock) frequency in Hz.
extern unsigned getClockFrequency();

// Actual system time, in microseconds.
extern unsigned64 getSystemTime();

// Section timing (high-resolution elapsed-time measurement).
typedef int64_t SectionTimeHandle;

SectionTimeHandle StartSectionTime();
float GetSectionTime(SectionTimeHandle section);
bool CompareSectionTimeGE(SectionTimeHandle section, float time);
int64_t GetSectionResolution();

#ifdef _WIN32

#define SLEEP_MS(t) Sleep(t)

#else

#define SLEEP_MS(t) ::Poseidon::Foundation::sleepMs(t)

extern void sleepMs(unsigned ms);

#endif

} // namespace Poseidon::Foundation

