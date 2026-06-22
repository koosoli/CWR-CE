#pragma once

// Dev-only command registry — one source of truth for cheats / tools that
// are exposed from multiple front-ends: the ImGui dev panel buttons, the
// dev-console text input, and per-action tri verbs (triEndMission,
// triGod, …).  Each command provides:
//   - name + help text (used by console and panel tooltips)
//   - available() predicate (gates UI disabled state + console errors)
//   - invoke()  — the actual work, called by UI/console/tri
//
// Tri verbs do NOT go through the string parser — they call the typed
// Cmd_X::Invoke() directly so argument types stay strong.  The registry
// exists so the console's free-form input can dispatch by name.
//
// Gated by AppConfig::DevMode() at the call sites (panel + console only
// render under --dev); tri verbs are dev-only by virtue of triBin not
// shipping with release.

#include <string>
#include <string_view>
#include <vector>

namespace Poseidon::Dev {
namespace DebugCommands
{

struct Command
{
    const char* name;
    const char* help;
    bool (*available)();
    // String form used by the console.  Args is the raw text after the
    // command name (may be empty).  Output goes to `out` for echo into
    // the console scrollback.
    void (*invoke)(std::string_view args, std::string& out);
};

void Register(const Command& cmd);
const Command* Find(std::string_view name);
const std::vector<Command>& All();

// Parse `line` as "<name> <rest>" and dispatch.  Used by the console
// tab.  Returns true on dispatch (regardless of command result); false
// if the command was not found.  Echoes either the command's output or
// an error string into `out`.
bool Run(std::string_view line, std::string& out);

} // namespace DebugCommands
} // namespace Poseidon::Dev
