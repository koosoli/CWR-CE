// Scoped wall-clock timer for performance instrumentation.
//
// Drops a `LOG_DEBUG(<Category>, "PERF: <label> took N.NNms")` on
// destruction, or only when elapsed >= a threshold (filters noise
// during steady-state gameplay).  The category is the same token the
// LOG_* macros take (Audio, Graphics, etc.), so the call site reads
// the same way:
//
//   SCOPED_PERF_TIMER(Audio, "WaveOAL::Load");
//   SCOPED_PERF_TIMER_THRESHOLD(Graphics, "TextBank::Load", 1.0);
//
// For cases that need to append details after the work (file size,
// row count, etc.), use the manual `Now()` / `ElapsedMs()` pair
// instead and write the LOG_DEBUG yourself:
//
//   const auto t0 = ::Poseidon::Dev::Perf::Now();
//   ... work ...
//   LOG_DEBUG(Audio, "PERF: WaveOAL::Load {} ({} bytes) took {:.2f}ms",
//             name, sizeBytes, ::Poseidon::Dev::Perf::ElapsedMs(t0));
//
// Output stays under the existing log-level / log-categories filters,
// so a `--log-level info` run sees nothing extra; a `--log-level debug
// --log-categories Audio,Graphics` run captures the timing stream.

#pragma once

#include <Poseidon/Foundation/Framework/Log.hpp>
#include <Poseidon/Dev/Diag/PerfTrace.hpp>

#include <chrono>
#include <cstdio>
#include <cstring>

namespace Poseidon::Dev::Perf
{

using Clock = std::chrono::steady_clock;
using Point = Clock::time_point;

inline Point Now()
{
    return Clock::now();
}

inline double ElapsedMs(Point start)
{
    return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}

inline int64_t PointToUs(Point p)
{
    using namespace std::chrono;
    return duration_cast<microseconds>(p.time_since_epoch()).count();
}

class ScopedTimer
{
  public:
    // `thresholdMs == 0` always logs; positive values gate noise from
    // sub-millisecond paths the user doesn't want to see.  When the
    // Chrome-trace sink is enabled, every scope emits one trace event
    // regardless of threshold — the threshold only gates the LOG line.
    explicit ScopedTimer(Foundation::LogCategory category, const char* label, double thresholdMs = 0.0)
        : _category(category), _label(label), _thresholdMs(thresholdMs), _start(Now())
    {
    }

    ~ScopedTimer()
    {
        const Point end = Now();
        const double ms = std::chrono::duration<double, std::milli>(end - _start).count();
        if (ms >= _thresholdMs)
        {
            LogDetail::Get(_category)->log(spdlog::source_loc{}, spdlog::level::debug, "PERF: {} took {:.2f}ms", _label,
                                           ms);
        }
        if (Trace::IsEnabled())
        {
            Trace::PushComplete(Foundation::LogCategoryTag(_category), _label, PointToUs(_start),
                                PointToUs(end) - PointToUs(_start));
        }
    }

    // Non-copyable, non-movable — the only sensible use is RAII at a
    // block's start.
    ScopedTimer(const ScopedTimer&) = delete;
    ScopedTimer& operator=(const ScopedTimer&) = delete;
    ScopedTimer(ScopedTimer&&) = delete;
    ScopedTimer& operator=(ScopedTimer&&) = delete;

  private:
    Foundation::LogCategory _category;
    const char* _label;
    double _thresholdMs;
    Point _start;
};

// Emit a Chrome-trace event for a manual `Now()` / `ElapsedMs()`
// measurement.  Use this next to the LOG_DEBUG that already reports
// the timing — it's a no-op when the trace sink isn't enabled, so
// it's cheap to leave in production hot paths.
//
//   const auto t0 = ::Poseidon::Dev::Perf::Now();
//   ... work ...
//   LOG_DEBUG(Audio, "PERF: WaveOAL::Load {} ({} bytes) took {:.2f}ms",
//             name, bytes, ::Poseidon::Dev::Perf::ElapsedMs(t0));
//   ::Poseidon::Dev::Perf::EmitTraceEvent(Foundation::LogCategory::Audio, "WaveOAL::Load",
//                                    t0, "\"asset\":\"<escaped>\",\"bytes\":12345");
inline void EmitTraceEvent(Foundation::LogCategory category, const char* label, Point start, const char* argsJson = nullptr)
{
    if (!Trace::IsEnabled())
    {
        return;
    }
    const int64_t startUs = PointToUs(start);
    const int64_t endUs = PointToUs(Now());
    Trace::PushComplete(Foundation::LogCategoryTag(category), label, startUs, endUs - startUs, argsJson);
}

// Emit a trace event with a single string-valued `asset` arg.
// Handles JSON escaping internally so callers can pass raw paths
// (backslashes, etc.).  No-op when the trace sink is disabled.
inline void EmitTraceEventAsset(Foundation::LogCategory category, const char* label, Point start, const char* asset)
{
    if (!Trace::IsEnabled())
    {
        return;
    }
    char args[768];
    constexpr int prefixLen = 8; // strlen("\"asset\":")
    std::memcpy(args, "\"asset\":", prefixLen);
    const int wrote = Trace::AppendEscapedString(args + prefixLen, sizeof(args) - prefixLen, asset);
    if (wrote <= 0)
    {
        EmitTraceEvent(category, label, start);
        return;
    }
    const int64_t startUs = PointToUs(start);
    const int64_t endUs = PointToUs(Now());
    Trace::PushComplete(Foundation::LogCategoryTag(category), label, startUs, endUs - startUs, args);
}

// Emit a trace event with an `asset` string + one numeric arg.
inline void EmitTraceEventAssetNum(Foundation::LogCategory category, const char* label, Point start, const char* asset,
                                   const char* numericKey, long long numericValue)
{
    if (!Trace::IsEnabled())
    {
        return;
    }
    char args[768];
    constexpr int prefixLen = 8;
    std::memcpy(args, "\"asset\":", prefixLen);
    int n = prefixLen + Trace::AppendEscapedString(args + prefixLen, sizeof(args) - prefixLen, asset);
    if (n <= prefixLen)
    {
        EmitTraceEvent(category, label, start);
        return;
    }
    const int written = std::snprintf(args + n, sizeof(args) - n, ",\"%s\":%lld", numericKey, numericValue);
    if (written <= 0 || written >= static_cast<int>(sizeof(args) - n))
    {
        EmitTraceEvent(category, label, start);
        return;
    }
    const int64_t startUs = PointToUs(start);
    const int64_t endUs = PointToUs(Now());
    Trace::PushComplete(Foundation::LogCategoryTag(category), label, startUs, endUs - startUs, args);
}

} // namespace Poseidon::Dev::Perf

// Token-paste helpers so SCOPED_PERF_TIMER can be used multiple times
// in the same scope without colliding on the variable name.
#define POSEIDON_PERF_TIMER_CAT2(a, b) a##b
#define POSEIDON_PERF_TIMER_CAT(a, b) POSEIDON_PERF_TIMER_CAT2(a, b)
#define POSEIDON_PERF_TIMER_NAME() POSEIDON_PERF_TIMER_CAT(_poseidon_perf_timer_, __LINE__)

#define SCOPED_PERF_TIMER(Category, Label) \
    ::Poseidon::Dev::Perf::ScopedTimer POSEIDON_PERF_TIMER_NAME()(::Poseidon::Foundation::LogCategory::Category, Label)

#define SCOPED_PERF_TIMER_THRESHOLD(Category, Label, ThresholdMs) \
    ::Poseidon::Dev::Perf::ScopedTimer POSEIDON_PERF_TIMER_NAME()(::Poseidon::Foundation::LogCategory::Category, Label, ThresholdMs)
