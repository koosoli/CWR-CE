// test_logging.cpp - Tests for unified LOG_* macros and Poseidon::Foundation::LoggingSystem utilities

#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Core/Application.hpp>
#include <Poseidon/Foundation/Logging/Logging.hpp>
#include <Poseidon/Foundation/Framework/Log.hpp>
#include <spdlog/sinks/callback_sink.h>
#include <spdlog/common.h>
#include <spdlog/details/log_msg.h>
#include <spdlog/logger.h>
#include <spdlog/spdlog.h>
#include <stddef.h>
#include <algorithm>
#include <memory>
#include <string>
#include <vector>

// Helper: captures spdlog messages via callback sink installed on category loggers
struct TestLogCapture
{
    struct Entry
    {
        spdlog::level::level_enum level;
        std::string logger_name;
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
        // Install on each category logger if initialized, otherwise on default
        bool any = false;
        for (int i = 0; i < static_cast<int>(Poseidon::Foundation::LogCategory::_Count); ++i)
        {
            if (LogDetail::g_loggers[i])
            {
                LogDetail::g_loggers[i]->sinks().push_back(sink);
                any = true;
            }
        }
        if (!any)
            spdlog::default_logger()->sinks().push_back(sink);
    }

    void Uninstall()
    {
        auto removeSink = [this](spdlog::logger* l)
        {
            auto& sinks = l->sinks();
            sinks.erase(std::remove(sinks.begin(), sinks.end(), sink), sinks.end());
        };
        for (int i = 0; i < static_cast<int>(Poseidon::Foundation::LogCategory::_Count); ++i)
            if (LogDetail::g_loggers[i])
                removeSink(LogDetail::g_loggers[i]);
        removeSink(spdlog::default_logger_raw());
        sink.reset();
    }

    size_t Count() const { return entries.size(); }

    size_t CountByLevel(spdlog::level::level_enum level) const
    {
        size_t n = 0;
        for (auto& e : entries)
            if (e.level == level)
                ++n;
        return n;
    }

    bool HasMessage(const std::string& substr) const
    {
        for (auto& e : entries)
            if (e.message.find(substr) != std::string::npos)
                return true;
        return false;
    }

    bool HasCategory(const std::string& cat) const
    {
        for (auto& e : entries)
            if (e.logger_name == cat)
                return true;
        return false;
    }

    void Clear() { entries.clear(); }
};

TEST_CASE("LOG_* macros produce structured spdlog output with category tags", "[logging][sink]")
{
    // Ensure category loggers exist (Initialize creates them)
    Poseidon::Foundation::LoggingSystem logSys;
    logSys.Initialize("debug");

    TestLogCapture capture;
    capture.Install();

    LOG_INFO(Core, "hello sink");
    LOG_WARN(Audio, "audio warning {}", 42);
    LOG_ERROR(Graphics, "gl error");

    REQUIRE(capture.Count() == 3);
    CHECK(capture.entries[0].level == spdlog::level::info);
    CHECK(capture.entries[0].logger_name == "Core");
    CHECK(capture.entries[0].message.find("hello sink") != std::string::npos);
    CHECK(capture.entries[1].level == spdlog::level::warn);
    CHECK(capture.entries[1].logger_name == "Audio");
    CHECK(capture.entries[1].message.find("audio warning 42") != std::string::npos);
    CHECK(capture.entries[2].level == spdlog::level::err);
    CHECK(capture.entries[2].logger_name == "Graphics");

    capture.Uninstall();
    logSys.Shutdown();
}

TEST_CASE("LOG_* category tags and levels are correct", "[logging][sink]")
{
    // Lower level on all loggers to capture debug messages
    auto prevLevel = spdlog::default_logger()->level();
    auto setAll = [](spdlog::level::level_enum lvl)
    {
        spdlog::default_logger()->set_level(lvl);
        for (int i = 0; i < static_cast<int>(Poseidon::Foundation::LogCategory::_Count); ++i)
            if (LogDetail::g_loggers[i])
                LogDetail::g_loggers[i]->set_level(lvl);
    };
    setAll(spdlog::level::debug);

    TestLogCapture capture;
    capture.Install();

    LOG_INFO(Core, "init complete");
    LOG_DEBUG(Audio, "loaded sound");
    LOG_WARN(Core, "fallback used");
    LOG_INFO(Audio, "audio init");

    CHECK(capture.CountByLevel(spdlog::level::info) == 2);
    CHECK(capture.CountByLevel(spdlog::level::warn) == 1);
    CHECK(capture.HasMessage("loaded sound"));
    CHECK_FALSE(capture.HasMessage("nonexistent"));

    capture.Clear();
    CHECK(capture.Count() == 0);
    capture.Uninstall();

    setAll(prevLevel);
}

TEST_CASE("Removing spdlog sink stops capturing", "[logging][sink]")
{
    Poseidon::Foundation::LoggingSystem logSys;
    logSys.Initialize("debug");

    TestLogCapture capture;
    capture.Install();

    LOG_INFO(Core, "before remove");
    REQUIRE(capture.Count() == 1);

    capture.Uninstall();

    LOG_INFO(Core, "after remove");
    CHECK(capture.Count() == 1); // should not grow

    logSys.Shutdown();
}

TEST_CASE("GetLevelName returns correct strings", "[logging]")
{
    CHECK(std::string(Poseidon::Foundation::LoggingSystem::GetLevelName(spdlog::level::trace)) == "trace");
    CHECK(std::string(Poseidon::Foundation::LoggingSystem::GetLevelName(spdlog::level::debug)) == "debug");
    CHECK(std::string(Poseidon::Foundation::LoggingSystem::GetLevelName(spdlog::level::info)) == "info");
    CHECK(std::string(Poseidon::Foundation::LoggingSystem::GetLevelName(spdlog::level::warn)) == "warn");
    CHECK(std::string(Poseidon::Foundation::LoggingSystem::GetLevelName(spdlog::level::err)) == "error");
    CHECK(std::string(Poseidon::Foundation::LoggingSystem::GetLevelName(spdlog::level::critical)) == "critical");
}

TEST_CASE("GetCategoryName returns correct strings", "[logging]")
{
    CHECK(std::string(Poseidon::Foundation::LoggingSystem::GetCategoryName(
              Poseidon::Foundation::LoggingSystem::Category::Core)) == "CORE");
    CHECK(std::string(Poseidon::Foundation::LoggingSystem::GetCategoryName(
              Poseidon::Foundation::LoggingSystem::Category::Audio)) == "AUDIO");
    CHECK(std::string(Poseidon::Foundation::LoggingSystem::GetCategoryName(
              Poseidon::Foundation::LoggingSystem::Category::Graphics)) == "GRAPHICS");
}

TEST_CASE("LOG_* messages contain no ANSI codes", "[logging]")
{
    Poseidon::Foundation::LoggingSystem logSys;
    logSys.Initialize("debug");

    TestLogCapture capture;
    capture.Install();

    LOG_INFO(Core, "test message with special chars");
    LOG_WARN(Audio, "value={}", 42);

    REQUIRE(capture.Count() == 2);
    for (auto& e : capture.entries)
        CHECK(e.message.find("\033[") == std::string::npos);
    CHECK(capture.HasMessage("special chars"));
    CHECK(capture.HasMessage("value=42"));

    capture.Uninstall();
    logSys.Shutdown();
}

TEST_CASE("strict mode: err-level log latches the trip; warn/info do not", "[logging][strict]")
{
    // The --strict path: ErrorCountingSink latches StrictTripped() on the first
    // err-level message while strict mode is on. GameApplication's main loop polls
    // it and turns it into a clean non-zero exit. Tested here at the mechanism
    // level (no main loop / GApp needed — latching is harmless without a poller).
    using LS = Poseidon::Foundation::LoggingSystem;
    LS logSys;
    logSys.Initialize("trace"); // attaches ErrorCountingSink to the category loggers

    // Strict OFF: an error bumps the count but must not latch the trip.
    LS::SetStrictMode(false);
    LS::ResetErrorCount();
    LOG_ERROR(Core, "strict-off error");
    CHECK(LS::GetErrorCount() >= 1);
    CHECK_FALSE(LS::StrictTripped());

    // Strict ON: info/warn do not trip; the first err-level message does.
    LS::SetStrictMode(true);
    LS::ResetErrorCount();
    CHECK_FALSE(LS::StrictTripped());
    LOG_INFO(Core, "info line");
    LOG_WARN(Core, "warn line");
    CHECK_FALSE(LS::StrictTripped());
    LOG_ERROR(Core, "strict-on error");
    CHECK(LS::StrictTripped());

    // ResetErrorCount clears the latch so tests can re-baseline at a quiet moment.
    LS::ResetErrorCount();
    CHECK_FALSE(LS::StrictTripped());

    LS::SetStrictMode(false); // don't leak strict into sibling tests in this binary
}
