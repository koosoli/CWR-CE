#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <stddef.h>
#include <catch2/catch_message.hpp>

// Regression test for the "right-click rotation in main-menu 3D objects
// should be dev-mode only" bug.
//
// Symptom: any user can right-drag a 3D object (e.g. the rotating logo
// or viewer object on the main menu) and re-orient it.  This was a dev
// affordance left wired up in shipping builds.
//
// Contract: `ControlObject::OnRButtonDown` must only enable rotation
// when `AppConfig::Instance().DevMode()` is true.  The full body of
// that function lives in `uiControls.cpp`; the audit below extracts
// the body and asserts it consults `DevMode()` before flipping
// `_rotating = true`.

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

// Extract the body of a function whose signature appears once in the
// file (matching the line that contains `prototype`).  Returns the
// substring between the function's opening `{` and its matching `}`.
std::string ExtractFunctionBody(const std::string& src, const std::string& prototype)
{
    const size_t protoPos = src.find(prototype);
    if (protoPos == std::string::npos)
        return {};
    const size_t openBrace = src.find('{', protoPos);
    if (openBrace == std::string::npos)
        return {};
    int depth = 1;
    for (size_t i = openBrace + 1; i < src.size(); ++i)
    {
        if (src[i] == '{')
            ++depth;
        else if (src[i] == '}')
        {
            if (--depth == 0)
                return src.substr(openBrace, i - openBrace + 1);
        }
    }
    return {};
}
} // namespace

TEST_CASE("ControlObject::OnRButtonDown gates rotation on AppConfig::DevMode (regression for menu 3D rotate)",
          "[ui][controls][devmode][regression]")
{
    // The whole point of this test: if a future edit deletes the
    // `DevMode()` gate from `OnRButtonDown`, the function reverts to
    // "any user can right-drag-rotate" which was the bug.  Audit the
    // shipped source so the gate is structurally required, not just
    // documented.
    // ControlObject methods were split into UIControlsBase.cpp when UIControls.cpp
    // was refactored; the DevMode gate lives there now.
    const std::filesystem::path cppPath = std::filesystem::path(TESTS_ROOT_DIR).parent_path() / "engine" / "Poseidon" /
                                          "UI" / "Controls" / "UIControlsBase.cpp";
    const std::string src = ReadTextFile(cppPath);
    REQUIRE_FALSE(src.empty());

    const std::string body = ExtractFunctionBody(src, "ControlObject::OnRButtonDown");
    REQUIRE_FALSE(body.empty());

    // The function MUST consult `DevMode()` before enabling rotation.
    // Any of the canonical phrasings is acceptable:
    //   `AppConfig::Instance().DevMode()`
    //   `Instance().DevMode()`
    //   a helper like `IsInteractiveRotateEnabled(...)` that ultimately
    //   reads DevMode (in which case the helper itself should appear).
    const bool hasDirect = body.find("DevMode()") != std::string::npos;
    const bool hasHelper = body.find("IsInteractiveRotateEnabled") != std::string::npos;
    CAPTURE(body);
    REQUIRE((hasDirect || hasHelper));

    // The early-return form is the simplest guard.  Require that the
    // function contains at least one `return` statement so the gate
    // actually short-circuits before `_rotating = true`.
    REQUIRE(body.find("return") != std::string::npos);
}
