#include <catch2/catch_test_macros.hpp>
#include <string>
#include <Poseidon/Core/Version.hpp>

using Poseidon::ComposeVersionString;
using Poseidon::ResolveVersionTag;

namespace
{
std::string Tag(const char* buildTag, const char* gitSha, bool dev, bool demo = false)
{
    return std::string((const char*)ResolveVersionTag(buildTag, gitSha, dev, demo));
}
std::string Compose(const char* base, const char* tag)
{
    return std::string((const char*)ComposeVersionString(base, tag));
}
} // namespace

// Version-tag policy (card: "game versioning, mp versioning"):
//   BUILD_VERSION_TAG="release"  -> empty tag (version is bare 3.00)
//   BUILD_VERSION_TAG="rc1"      -> "rc1"
//   unset                        -> the git sha is the tag
//   non-release --dev            -> "-dev" suffix (so dev clients can't join non-dev servers)
// ResolveVersionTag is the join-matching string; ComposeVersionString is the display form.

TEST_CASE("ResolveVersionTag: release renders as no tag", "[version]")
{
    CHECK(Tag("release", "d83a", false) == "");
    CHECK(Tag("RELEASE", "d83a", false) == ""); // case-insensitive
    // Pure resolver still honors devMode; final release binaries reject --dev before this path.
    CHECK(Tag("release", "d83a", true) == "dev");
}

TEST_CASE("ResolveVersionTag: explicit tag wins over git sha", "[version]")
{
    CHECK(Tag("rc1", "d83a", false) == "rc1");
    CHECK(Tag("rc1", "d83a", true) == "rc1-dev");
}

TEST_CASE("ResolveVersionTag: git sha is the default tag", "[version]")
{
    CHECK(Tag(nullptr, "d83a", false) == "d83a");
    CHECK(Tag(nullptr, "d83a", true) == "d83a-dev");
    CHECK(Tag("", "d83a", false) == "d83a"); // empty BUILD_VERSION_TAG == unset
}

TEST_CASE("ResolveVersionTag: nothing known, dev only", "[version]")
{
    CHECK(Tag(nullptr, nullptr, false) == "");
    CHECK(Tag(nullptr, nullptr, true) == "dev");
}

TEST_CASE("ResolveVersionTag: demo inserts -demo before any -dev", "[version]")
{
    CHECK(Tag("rc1", "d83a", false, true) == "rc1-demo");
    CHECK(Tag(nullptr, "d83a", false, true) == "d83a-demo");
    CHECK(Tag(nullptr, "d83a", true, true) == "d83a-demo-dev"); // 3.0-d83a-demo-dev
    CHECK(Tag("release", "d83a", true, true) == "demo-dev");    // release demo, dev
    CHECK(Tag(nullptr, nullptr, false, true) == "demo");        // nothing known
    CHECK(Tag(nullptr, "d83a", true, false) == "d83a-dev");     // non-demo unchanged
}

TEST_CASE("ComposeVersionString: tag joined with a dash, empty omitted", "[version]")
{
    CHECK(Compose("3.0", "rc1") == "3.0-rc1");
    CHECK(Compose("3.0", "d83a-dev") == "3.0-d83a-dev");
    CHECK(Compose("3.0", "") == "3.0"); // release
    CHECK(Compose("3.0", nullptr) == "3.0");
}
