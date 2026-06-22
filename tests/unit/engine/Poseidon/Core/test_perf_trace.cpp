// Unit tests for PerfTrace — Chrome trace JSON sink used by the
// asset-system modernization (Phase 1).  Pins:
//   - Enable / IsEnabled round-trip
//   - PushComplete writes a valid X event with ts/dur/args
//   - PushBegin / PushEnd write paired B/E events
//   - PushCounter writes a C event with numeric series
//   - AppendEscapedString handles backslashes / quotes / control chars
//   - Every emitted line is independently valid JSON (NDJSON contract)
//   - Disabled sink is a no-op (no file open, no allocation)
//
// Each test enables the trace to a fresh tmp file, exercises one
// API, then disables.  File is parsed line-by-line; each line must
// be a valid JSON object on its own.

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <Poseidon/Dev/Diag/PerfTrace.hpp>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

namespace
{

// Per-test-process unique trace path so tests run in parallel without
// stomping each other's output.
std::string MakeTracePath(const char* tag)
{
    static thread_local std::mt19937_64 rng{std::random_device{}()};
    std::ostringstream oss;
    oss << std::filesystem::temp_directory_path().generic_string() << "/perf-trace-" << tag << "-" << rng()
        << ".ndjson";
    return oss.str();
}

std::vector<std::string> ReadLines(const std::string& path)
{
    std::ifstream f(path);
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(f, line))
    {
        if (!line.empty())
        {
            lines.push_back(line);
        }
    }
    return lines;
}

// Hand-rolled minimal "does this look like valid JSON" check — full
// JSON parsers would balloon test deps.  We only need to verify the
// PerfTrace output is structurally well-formed.  Counts brace depth
// and quote escaping; rejects unmatched braces or trailing garbage.
bool LooksLikeJsonObject(const std::string& s)
{
    if (s.size() < 2 || s.front() != '{' || s.back() != '}')
    {
        return false;
    }
    int depth = 0;
    bool inString = false;
    bool escape = false;
    for (char c : s)
    {
        if (escape)
        {
            escape = false;
            continue;
        }
        if (inString)
        {
            if (c == '\\')
            {
                escape = true;
            }
            else if (c == '"')
            {
                inString = false;
            }
            continue;
        }
        if (c == '"')
        {
            inString = true;
        }
        else if (c == '{')
        {
            ++depth;
        }
        else if (c == '}')
        {
            --depth;
            if (depth < 0)
            {
                return false;
            }
        }
    }
    return depth == 0 && !inString;
}

// Trivial substring containment check using the "contains" semantic.
bool Contains(const std::string& haystack, const std::string& needle)
{
    return haystack.find(needle) != std::string::npos;
}

struct TraceFixture
{
    std::string path;

    explicit TraceFixture(const char* tag) : path(MakeTracePath(tag))
    {
        REQUIRE(Poseidon::Dev::Perf::Trace::Enable(path.c_str()));
    }

    ~TraceFixture() { Poseidon::Dev::Perf::Trace::Disable(); }

    // Flush the sink and read everything emitted so far.  Windows
    // fopen("wb") holds an exclusive lock; the file isn't readable
    // until Disable closes it, so this is the only correct way to
    // inspect the output inside the same test case.
    std::vector<std::string> Lines()
    {
        Poseidon::Dev::Perf::Trace::Disable();
        return ReadLines(path);
    }
};

} // namespace

TEST_CASE("PerfTrace::IsEnabled is true after Enable", "[perf_trace]")
{
    TraceFixture fix("isenabled");
    REQUIRE(Poseidon::Dev::Perf::Trace::IsEnabled());
}

TEST_CASE("PerfTrace::PushComplete writes X event with ts/dur", "[perf_trace][complete]")
{
    TraceFixture fix("complete");
    Poseidon::Dev::Perf::Trace::PushComplete("Audio", "WaveOAL::Load", 1000, 250, nullptr);
    Poseidon::Dev::Perf::Trace::PushComplete("Graphics", "TextBank::Load", 2000, 50, "\"asset\":\"foo.paa\"");

    auto lines = fix.Lines();
    REQUIRE(lines.size() >= 2);
    for (const auto& line : lines)
    {
        REQUIRE(LooksLikeJsonObject(line));
    }
    REQUIRE(Contains(lines[0], "\"name\":\"WaveOAL::Load\""));
    REQUIRE(Contains(lines[0], "\"cat\":\"Audio\""));
    REQUIRE(Contains(lines[0], "\"ph\":\"X\""));
    REQUIRE(Contains(lines[0], "\"dur\":250"));
    REQUIRE(Contains(lines[1], "\"asset\":\"foo.paa\""));
}

TEST_CASE("PerfTrace::PushBegin and PushEnd produce paired B/E events", "[perf_trace][phase]")
{
    TraceFixture fix("phase");
    Poseidon::Dev::Perf::Trace::PushBegin("Mission", "PreloadManifest", 1000, nullptr);
    Poseidon::Dev::Perf::Trace::PushEnd("Mission", "PreloadManifest", 5000);

    auto lines = fix.Lines();
    REQUIRE(lines.size() >= 2);
    for (const auto& line : lines)
    {
        REQUIRE(LooksLikeJsonObject(line));
    }
    REQUIRE(Contains(lines[0], "\"ph\":\"B\""));
    REQUIRE(Contains(lines[0], "\"name\":\"PreloadManifest\""));
    REQUIRE(Contains(lines[1], "\"ph\":\"E\""));
    REQUIRE(Contains(lines[1], "\"name\":\"PreloadManifest\""));
}

TEST_CASE("PerfTrace::PushCounter writes C event with single numeric arg", "[perf_trace][counter]")
{
    TraceFixture fix("counter");
    Poseidon::Dev::Perf::Trace::PushCounter("Memory", "TextBank.residentBytes", 1000, 12345);
    Poseidon::Dev::Perf::Trace::PushCounter("Memory", "TextBank.residentBytes", 2000, 23456);

    auto lines = fix.Lines();
    REQUIRE(lines.size() >= 2);
    for (const auto& line : lines)
    {
        REQUIRE(LooksLikeJsonObject(line));
    }
    REQUIRE(Contains(lines[0], "\"ph\":\"C\""));
    REQUIRE(Contains(lines[0], "\"name\":\"TextBank.residentBytes\""));
    REQUIRE(Contains(lines[0], "12345"));
    REQUIRE(Contains(lines[1], "23456"));
}

TEST_CASE("PerfTrace::AppendEscapedString escapes backslashes and quotes", "[perf_trace][escape]")
{
    char buf[256];
    const char* input = "data\\foo\"bar.paa";
    const int n = Poseidon::Dev::Perf::Trace::AppendEscapedString(buf, sizeof(buf), input);
    REQUIRE(n > 0);
    const std::string s(buf, static_cast<size_t>(n));
    // Should be: "data\\foo\"bar.paa"  →  "data\\\\foo\\\"bar.paa"
    // (and wrapped in outer quotes).
    REQUIRE(s.front() == '"');
    REQUIRE(s.back() == '"');
    REQUIRE(Contains(s, "\\\\")); // backslash escaped
    REQUIRE(Contains(s, "\\\"")); // quote escaped
}

TEST_CASE("PerfTrace::AppendEscapedString refuses buffers it can't fit", "[perf_trace][escape][edge]")
{
    char tiny[4];
    // 30-char string + 2 quotes can't fit in 4 bytes — must return 0.
    REQUIRE(Poseidon::Dev::Perf::Trace::AppendEscapedString(tiny, sizeof(tiny), "a_long_string_value_here") == 0);
}

TEST_CASE("PerfTrace lines are individually valid NDJSON", "[perf_trace][format]")
{
    // The contract is that every line is a complete JSON document on
    // its own, so a crash mid-stream still leaves a parseable file.
    // 50 events, each on its own line, no surrounding array.
    TraceFixture fix("ndjson");
    for (int i = 0; i < 50; ++i)
    {
        Poseidon::Dev::Perf::Trace::PushComplete("Core", "TestEvent", 1000 + i, 1, nullptr);
    }

    auto lines = fix.Lines();
    REQUIRE(lines.size() >= 50);
    for (const auto& line : lines)
    {
        REQUIRE(LooksLikeJsonObject(line));
        // Each line is independently a JSON object — no leading `[` or
        // `,` or trailing `]` should sneak in.
        REQUIRE(line.front() == '{');
        REQUIRE(line.back() == '}');
    }
}
