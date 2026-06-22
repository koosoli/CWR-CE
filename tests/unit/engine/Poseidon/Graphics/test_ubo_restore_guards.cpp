#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <stddef.h>

// I-32 / B-036 — defensive UBO restore must guard against uninitialised
// source.
//
// B-036 was the "viewer enters charcoal-screen mode" failure: the
// B-001 fix uploaded `_frameState.projection` unconditionally at the
// end of every state mutation; in viewer mode the slot was never
// populated so the upload wrote zero, and every subsequent draw
// projected through a zero matrix.  The structural fix wraps each
// such restore in `if (IsIn3DPass())`.
//
// This audit reads `EngineGL33_Draw.cpp` and asserts every
// `UploadVSProjection(_frameState)` call sits inside a gating
// predicate.  A regression that drops the guard reproduces the
// B-036 path and trips the audit before viewer mode breaks again.

namespace
{
std::string ReadTextFile(const std::filesystem::path& p)
{
    std::ifstream f(p);
    if (!f.is_open())
        return {};
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}
} // namespace

TEST_CASE("I-32: every UploadVSProjection in Draw.cpp is gated by IsIn3DPass", "[Graphics][UBORestoreGuard][I-32]")
{
    const std::filesystem::path drawCpp =
        std::filesystem::path(TESTS_ROOT_DIR).parent_path() / "engine" / "PoseidonGL33" / "EngineGL33_Draw.cpp";
    const std::string body = ReadTextFile(drawCpp);
    REQUIRE_FALSE(body.empty());

    int uploads = 0;
    int guarded = 0;
    size_t pos = 0;
    while ((pos = body.find("UploadVSProjection(", pos)) != std::string::npos)
    {
        ++uploads;

        // Walk backwards a few hundred chars and require an
        // `IsIn3DPass()` mention — the gate the B-036 fix
        // installed.  The window is large enough to span a
        // function prelude (`{ ... if (IsIn3DPass()) { ...
        // UploadVSProjection ... } }`) but tight enough to fail
        // if a sibling code path appears outside the guard.
        const size_t scanFrom = pos > 600 ? pos - 600 : 0;
        const std::string before = body.substr(scanFrom, pos - scanFrom);
        if (before.find("IsIn3DPass()") != std::string::npos)
            ++guarded;

        pos += 1;
    }

    // Sanity: there IS at least one upload site (the SetBias
    // restore introduced for B-001).  If this drops to zero, the
    // codebase shifted enough that the audit should be rewritten,
    // not silently passing on a vacuous match.
    REQUIRE(uploads >= 1);
    REQUIRE(uploads == guarded);
}
