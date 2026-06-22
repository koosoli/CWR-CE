#pragma once

#include <cstdint>

namespace Poseidon
{

namespace RateLimit
{

// Monotonic token bucket. Refills at `ratePerSec` tokens, capped at `burst`; each
// tryConsume(nowMs) first credits the time elapsed since the last call, then takes
// one token if any remain. Timestamps are unsigned milliseconds and differenced
// wrap-aware, so a 32-bit tick-count rollover is handled. Pure and clock-injected
// so it can be unit-tested without a real timer.
//
// Used to cap the rate of reflected enum replies so a dedicated server can't be
// abused as a UDP reflection/amplification source (N-SEC-09): a spoofed-source
// enum request can still be answered, but only up to `burst` in a flood and
// `ratePerSec` sustained — far below any useful amplification, far above the
// one-request-per-refresh a legitimate server browser sends.
struct TokenBucket
{
    double tokens = 0.0;
    double ratePerSec = 0.0;
    double burst = 0.0;
    uint32_t lastMs = 0;
    bool initialized = false;

    void configure(double rate, double burstCap)
    {
        ratePerSec = rate;
        burst = burstCap;
        tokens = burstCap;
        lastMs = 0;
        initialized = false;
    }

    bool tryConsume(uint32_t nowMs)
    {
        if (!initialized)
        {
            initialized = true;
            lastMs = nowMs;
            tokens = burst;
        }
        const uint32_t elapsedMs = nowMs - lastMs; // wrap-aware unsigned difference
        lastMs = nowMs;
        tokens += static_cast<double>(elapsedMs) / 1000.0 * ratePerSec;
        if (tokens > burst)
        {
            tokens = burst;
        }
        if (tokens >= 1.0)
        {
            tokens -= 1.0;
            return true;
        }
        return false;
    }
};

} // namespace RateLimit

} // namespace Poseidon
