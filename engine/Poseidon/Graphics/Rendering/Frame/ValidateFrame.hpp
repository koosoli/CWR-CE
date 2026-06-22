#pragma once

#include <Poseidon/Graphics/Rendering/Frame/Frame.hpp>

#include <string>
#include <vector>

// Pure validation of a Frame against renderer invariants.  Runs in
// debug builds before Execute; in release builds returns immediately.
// Tests drive it directly without any GL context.


namespace Poseidon
{
namespace render::frame
{

struct Violation
{
    const char* ruleId;     // "I-01", "I-09", etc. — refs invariants doc
    std::string detail;     // human-readable specifics
};

struct ValidationResult
{
    std::vector<Violation> violations;
    bool ok() const { return violations.empty(); }
};

// Pure function.  No globals, no I/O.
ValidationResult ValidateFrame(const Frame& f);

} // namespace render::frame

} // namespace Poseidon
