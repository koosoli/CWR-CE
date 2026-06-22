// Unit tests for ScopedTimer — pins:
//   - PERF log fires at LOG_DEBUG level on the right category
//   - Threshold filter silences sub-threshold timings
//   - Manual `Now()` / `ElapsedMs()` pair returns plausible numbers
//
// The capture sink pattern is borrowed from test_logging.cpp so the
// timing log lines can be inspected without going to stdout.

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <Poseidon/Dev/Diag/ScopedTimer.hpp>
#include <Poseidon/Foundation/Framework/Log.hpp>
#include <spdlog/sinks/callback_sink.h>

#include <chrono>
#include <string>
#include <thread>
#include <vector>
#include <spdlog/common.h>
#include <spdlog/details/log_msg.h>
#include <spdlog/logger.h>
#include <spdlog/spdlog.h>
#include <stddef.h>
#include <algorithm>
#include <catch2/catch_message.hpp>
#include <memory>

namespace
{

struct PerfCapture
{
    struct Entry
    {
        spdlog::level::level_enum level;
        std::string logger;
        std::string message;
    };
    std::vector<Entry> entries;
    std::shared_ptr<spdlog::sinks::callback_sink_mt> sink;

    void Install()
    {
        sink = std::make_shared<spdlog::sinks::callback_sink_mt>(
            [this](const spdlog::details::log_msg& msg)
            {
                entries.push_back({msg.level, std::string(msg.logger_name.data(), msg.logger_name.size()),
                                   std::string(msg.payload.data(), msg.payload.size())});
            });
        sink->set_level(spdlog::level::trace);
        // Push the sink onto every category logger that's been
        // initialised, or onto the default if logging hasn't been
        // bootstrapped yet (the test runner may or may not have).
        bool any = false;
        for (int i = 0; i < static_cast<int>(Poseidon::Foundation::LogCategory::_Count); ++i)
        {
            if (LogDetail::g_loggers[i])
            {
                LogDetail::g_loggers[i]->sinks().push_back(sink);
                LogDetail::g_loggers[i]->set_level(spdlog::level::trace);
                any = true;
            }
        }
        if (!any)
        {
            spdlog::default_logger()->sinks().push_back(sink);
            spdlog::default_logger()->set_level(spdlog::level::trace);
        }
    }

    void Uninstall()
    {
        auto remove = [this](spdlog::logger* l)
        {
            auto& sinks = l->sinks();
            sinks.erase(std::remove(sinks.begin(), sinks.end(), sink), sinks.end());
        };
        for (int i = 0; i < static_cast<int>(Poseidon::Foundation::LogCategory::_Count); ++i)
            if (LogDetail::g_loggers[i])
                remove(LogDetail::g_loggers[i]);
        remove(spdlog::default_logger_raw());
        sink.reset();
    }

    size_t Count() const { return entries.size(); }
};

} // namespace

TEST_CASE("ScopedTimer emits PERF log at DEBUG on destruction", "[Core][ScopedTimer]")
{
    PerfCapture cap;
    cap.Install();
    {
        SCOPED_PERF_TIMER(Audio, "ScopedTimer-smoke");
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    cap.Uninstall();

    // Find the entry our timer produced.  Other tests may inject log
    // noise via the same sink so search rather than assert count.
    bool found = false;
    for (const auto& e : cap.entries)
    {
        if (e.message.find("PERF: ScopedTimer-smoke took") != std::string::npos)
        {
            found = true;
            CHECK(e.level == spdlog::level::debug);
        }
    }
    REQUIRE(found);
}

TEST_CASE("ScopedTimer with threshold filters fast paths", "[Core][ScopedTimer]")
{
    PerfCapture cap;
    cap.Install();
    {
        // 100 ms threshold; the block exits in microseconds → no log.
        SCOPED_PERF_TIMER_THRESHOLD(Audio, "ScopedTimer-below-threshold", 100.0);
    }
    cap.Uninstall();

    for (const auto& e : cap.entries)
    {
        INFO("unexpected entry: " << e.message);
        CHECK(e.message.find("ScopedTimer-below-threshold") == std::string::npos);
    }
}

TEST_CASE("ScopedTimer with threshold logs slow paths", "[Core][ScopedTimer]")
{
    PerfCapture cap;
    cap.Install();
    {
        // 0.5 ms threshold; a 5 ms sleep clears it comfortably.
        SCOPED_PERF_TIMER_THRESHOLD(Audio, "ScopedTimer-above-threshold", 0.5);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    cap.Uninstall();

    bool found = false;
    for (const auto& e : cap.entries)
        if (e.message.find("PERF: ScopedTimer-above-threshold took") != std::string::npos)
            found = true;
    REQUIRE(found);
}

TEST_CASE("ScopedTimer free-function pair returns plausible elapsed", "[Core][ScopedTimer]")
{
    const auto t0 = ::Poseidon::Dev::Perf::Now();
    std::this_thread::sleep_for(std::chrono::milliseconds(3));
    const double ms = ::Poseidon::Dev::Perf::ElapsedMs(t0);
    // sleep_for is "at least" — be generous on the upper bound to
    // tolerate scheduler noise on CI runners.
    CHECK(ms >= 2.0);
    CHECK(ms < 200.0);
}

TEST_CASE("Two ScopedTimers in the same scope don't collide on name", "[Core][ScopedTimer]")
{
    // The macro expands to a __LINE__-suffixed variable; two timers on
    // adjacent lines must coexist without a redeclaration error.
    PerfCapture cap;
    cap.Install();
    {
        SCOPED_PERF_TIMER(Audio, "first");
        SCOPED_PERF_TIMER(Audio, "second");
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    cap.Uninstall();

    bool firstFound = false, secondFound = false;
    for (const auto& e : cap.entries)
    {
        if (e.message.find("PERF: first took") != std::string::npos)
            firstFound = true;
        if (e.message.find("PERF: second took") != std::string::npos)
            secondFound = true;
    }
    CHECK(firstFound);
    CHECK(secondFound);
}
