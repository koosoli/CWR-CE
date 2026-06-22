#include <Poseidon/Dev/Diag/PerfTrace.hpp>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <mutex>

namespace Poseidon::Dev::Perf::Trace
{

namespace
{

std::mutex g_mtx;
std::FILE* g_file = nullptr;
bool g_firstEvent = true;
int64_t g_baseUs = 0;
std::atomic<bool> g_enabled{false};

int64_t NowUs()
{
    using namespace std::chrono;
    return duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count();
}

// Each thread is given a small monotonic integer the first time it
// emits an event.  This stays stable across the trace and renders
// cleanly in Perfetto's lane view (one row per tid).  The first
// thread to call this — always the main thread in production, where
// Enable runs — gets tid=1, matching the pre-Phase-3d hardcoded
// value so old traces line up on the same lane.
uint32_t ThisThreadTid()
{
    static std::atomic<uint32_t> g_nextTid{1};
    static thread_local uint32_t s_tid = 0;
    if (s_tid == 0)
    {
        s_tid = g_nextTid.fetch_add(1, std::memory_order_relaxed);
    }
    return s_tid;
}

std::FILE* OpenForWrite(const char* path)
{
#ifdef _MSC_VER
    std::FILE* f = nullptr;
    if (fopen_s(&f, path, "wb") != 0)
    {
        return nullptr;
    }
    return f;
#else
    return std::fopen(path, "wb");
#endif
}

void WriteEscapedString(std::FILE* f, const char* s)
{
    std::fputc('"', f);
    for (const char* p = s; *p; ++p)
    {
        const unsigned char c = static_cast<unsigned char>(*p);
        switch (c)
        {
            case '"':
                std::fputs("\\\"", f);
                break;
            case '\\':
                std::fputs("\\\\", f);
                break;
            case '\n':
                std::fputs("\\n", f);
                break;
            case '\r':
                std::fputs("\\r", f);
                break;
            case '\t':
                std::fputs("\\t", f);
                break;
            default:
                if (c < 0x20)
                {
                    std::fprintf(f, "\\u%04x", c);
                }
                else
                {
                    std::fputc(c, f);
                }
        }
    }
    std::fputc('"', f);
}

void FlushOnExit()
{
    std::lock_guard<std::mutex> lock(g_mtx);
    if (g_file)
    {
        std::fclose(g_file);
        g_file = nullptr;
        g_enabled.store(false, std::memory_order_release);
    }
}

} // namespace

bool IsEnabled()
{
    return g_enabled.load(std::memory_order_acquire);
}

void Disable()
{
    std::lock_guard<std::mutex> lock(g_mtx);
    if (g_file)
    {
        std::fclose(g_file);
        g_file = nullptr;
        g_enabled.store(false, std::memory_order_release);
    }
}

bool Enable(const char* path)
{
    std::lock_guard<std::mutex> lock(g_mtx);
    if (g_file)
    {
        // Re-enable with a new path: close the existing file first so
        // the second Enable's events land where the caller asked.
        // (Original semantics — "second call is a no-op" — broke unit
        // tests that exercise Enable per-test-case, and was never a
        // documented user-facing guarantee.)
        std::fclose(g_file);
        g_file = nullptr;
        g_enabled.store(false, std::memory_order_release);
    }
    g_file = OpenForWrite(path);
    if (!g_file)
    {
        return false;
    }
    g_baseUs = NowUs();
    g_firstEvent = true;
    // Claim tid=1 for the calling (main) thread before any worker
    // thread can touch ThisThreadTid().  Phase 3d's TaskPool workers
    // emit Audio::WaveOAL::Decode events from worker threads; this
    // ordering ensures the main thread reads as tid=1 in the trace.
    (void)ThisThreadTid();
    g_enabled.store(true, std::memory_order_release);
    std::atexit(&FlushOnExit);
    return true;
}

int AppendEscapedString(char* buf, int bufSize, const char* s)
{
    if (bufSize < 3)
    {
        return 0;
    }
    int n = 0;
    buf[n++] = '"';
    for (const unsigned char* p = reinterpret_cast<const unsigned char*>(s); *p; ++p)
    {
        const unsigned char c = *p;
        char tmp[8];
        int len = 0;
        switch (c)
        {
            case '"':
                tmp[0] = '\\';
                tmp[1] = '"';
                len = 2;
                break;
            case '\\':
                tmp[0] = '\\';
                tmp[1] = '\\';
                len = 2;
                break;
            case '\n':
                tmp[0] = '\\';
                tmp[1] = 'n';
                len = 2;
                break;
            case '\r':
                tmp[0] = '\\';
                tmp[1] = 'r';
                len = 2;
                break;
            case '\t':
                tmp[0] = '\\';
                tmp[1] = 't';
                len = 2;
                break;
            default:
                if (c < 0x20)
                {
                    len = std::snprintf(tmp, sizeof(tmp), "\\u%04x", c);
                }
                else
                {
                    tmp[0] = static_cast<char>(c);
                    len = 1;
                }
        }
        if (n + len + 2 > bufSize) // reserve room for closing '"' + NUL
        {
            return 0;
        }
        for (int i = 0; i < len; ++i)
        {
            buf[n++] = tmp[i];
        }
    }
    buf[n++] = '"';
    buf[n] = '\0';
    return n;
}

// Shared writer for every event phase.  Newline-delimited JSON —
// one event per line.  Survives process crash / atexit-not-firing
// because each flushed line is already a valid JSON document.
// DuckDB reads via `read_json_auto('trace.ndjson',
// format='newline_delimited')`; Chrome trace conversion via
// `scripts/perf-analyze.ps1 -ToChromeTrace`.
//
// Caller is expected to hold g_mtx.  `extraJson` is appended verbatim
// after the common header (e.g. `,"dur":250` for X events).
namespace
{
void WriteEventLocked(const char* cat, const char* name, char phase, int64_t tsUs, const char* extraJson,
                      const char* args)
{
    std::fputs("{\"name\":", g_file);
    WriteEscapedString(g_file, name);
    std::fputs(",\"cat\":", g_file);
    WriteEscapedString(g_file, cat);
    std::fprintf(g_file, ",\"ph\":\"%c\",\"pid\":1,\"tid\":%u,\"ts\":%lld", phase,
                 static_cast<unsigned>(ThisThreadTid()), static_cast<long long>(tsUs - g_baseUs));
    if (extraJson && *extraJson)
    {
        std::fputs(extraJson, g_file);
    }
    if (args && *args)
    {
        std::fputs(",\"args\":{", g_file);
        std::fputs(args, g_file);
        std::fputc('}', g_file);
    }
    std::fputs("}\n", g_file);
    std::fflush(g_file);
    g_firstEvent = false;
}
} // namespace

void PushComplete(const char* cat, const char* name, int64_t tsUs, int64_t durationUs, const char* args)
{
    if (!g_enabled.load(std::memory_order_acquire))
    {
        return;
    }
    std::lock_guard<std::mutex> lock(g_mtx);
    if (!g_file)
    {
        return;
    }
    char dur[48];
    std::snprintf(dur, sizeof(dur), ",\"dur\":%lld", static_cast<long long>(durationUs));
    WriteEventLocked(cat, name, 'X', tsUs, dur, args);
}

void PushBegin(const char* cat, const char* name, int64_t tsUs, const char* args)
{
    if (!g_enabled.load(std::memory_order_acquire))
    {
        return;
    }
    std::lock_guard<std::mutex> lock(g_mtx);
    if (!g_file)
    {
        return;
    }
    WriteEventLocked(cat, name, 'B', tsUs, nullptr, args);
}

void PushEnd(const char* cat, const char* name, int64_t tsUs)
{
    if (!g_enabled.load(std::memory_order_acquire))
    {
        return;
    }
    std::lock_guard<std::mutex> lock(g_mtx);
    if (!g_file)
    {
        return;
    }
    WriteEventLocked(cat, name, 'E', tsUs, nullptr, nullptr);
}

void PushCounter(const char* cat, const char* name, int64_t tsUs, int64_t value)
{
    if (!g_enabled.load(std::memory_order_acquire))
    {
        return;
    }
    std::lock_guard<std::mutex> lock(g_mtx);
    if (!g_file)
    {
        return;
    }
    char args[64];
    std::snprintf(args, sizeof(args), "\"value\":%lld", static_cast<long long>(value));
    WriteEventLocked(cat, name, 'C', tsUs, nullptr, args);
}

} // namespace Poseidon::Dev::Perf::Trace
