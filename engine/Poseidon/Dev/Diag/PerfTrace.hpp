// Chrome trace-event JSON writer for performance instrumentation.
//
// When enabled, ScopedTimer (and any other call site that opts in)
// emits one "X" complete event per measurement.  The output loads
// directly into https://ui.perfetto.dev/ or chrome://tracing for a
// timeline view, and into DuckDB via `read_json_auto` for tabular
// analysis.
//
// Enable from main once `AppConfig` is parsed:
//
//   if (const auto& p = AppConfig::Instance().GetPerfTracePath(); !p.empty())
//       Poseidon::Dev::Perf::Trace::Enable(p.c_str());
//
// Shutdown is flush-on-process-exit via `atexit`; the trailing `]` is
// always written so the file is valid JSON even if the process dies
// later.  Thread-safe — push from any thread.

#pragma once

#include <cstdint>

namespace Poseidon::Dev::Perf::Trace
{

bool IsEnabled();
bool Enable(const char* path);

// Flush + close the trace file.  Called automatically by atexit;
// explicitly callable for tests that need to re-enable with a fresh
// path, or for tools that want a deterministic shutdown boundary.
void Disable();

// Push a complete (X) event.  `args` is optional raw JSON body
// (e.g. `"asset":"foo.ogg","bytes":12345`) — keep it short.  Any
// string values inside `args` must already be JSON-escaped (use
// AppendEscapedString to build them).  `cat` is the LOG category
// name ("Audio", etc.).
void PushComplete(const char* cat, const char* name, int64_t tsUs, int64_t durationUs, const char* args = nullptr);

// Push a begin (B) event for a long-running async span that can
// stack across threads.  Pair every PushBegin with one PushEnd that
// uses the same `name`.  Use for "Phase A measurement", "Mission
// preload", "Decode batch" — spans where the start and end are far
// apart in time and a wrapping ScopedTimer doesn't fit naturally.
void PushBegin(const char* cat, const char* name, int64_t tsUs, const char* args = nullptr);
void PushEnd  (const char* cat, const char* name, int64_t tsUs);

// Push a counter (C) sample.  Renders as a sparkline on the Perfetto
// timeline.  Single numeric series per call; the value lands under
// `args.value` in the emitted JSON.  Use for residency bytes, cache
// hit rate, in-flight decode count, etc.
void PushCounter(const char* cat, const char* name, int64_t tsUs, int64_t value);

// Append a JSON-escaped, quote-wrapped string to a fixed buffer.
// Used by call sites that build their own `args` JSON body and
// need to embed a path (backslashes, quotes, control chars).
// Returns the number of bytes written, or 0 if the input would
// overflow `bufSize`.
int AppendEscapedString(char* buf, int bufSize, const char* s);

} // namespace Poseidon::Dev::Perf::Trace
