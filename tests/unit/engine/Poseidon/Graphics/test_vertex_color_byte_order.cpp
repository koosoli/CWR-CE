#include <catch2/catch_test_macros.hpp>

#include <Poseidon/Graphics/Rendering/ColorsK.hpp>

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

// I-31 / B-027 — CPU integer field order vs GL attribute byte order.
//
// `PackedColor::PackedColor(r, g, b, a)` stores
// `(a<<24)|(r<<16)|(g<<8)|b` in a 32-bit DWORD.  On little-endian
// that means memory byte order is `b, g, r, a` — BGRA, not RGBA.
// The GL attribute pointer for vertex color must therefore declare
// `GL_BGRA`; declaring `GL_RGBA` swaps R and B at sample time.

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

TEST_CASE("I-31: PackedColor memory layout is BGRA on little-endian", "[Graphics][VertexColor][I-31]")
{
    // PackedColor(r=10, g=20, b=30, a=40) → DWORD 0x280A1416 in
    // numeric value; memory bytes (little-endian) read 0x16, 0x14,
    // 0x0A, 0x28 = B, G, R, A.
    const PackedColor c(/*r*/ 10, /*g*/ 20, /*b*/ 30, /*a*/ 40);
    std::uint8_t bytes[4];
    std::memcpy(bytes, &c, sizeof(bytes));

    REQUIRE(bytes[0] == 30); // blue first
    REQUIRE(bytes[1] == 20); // green
    REQUIRE(bytes[2] == 10); // red
    REQUIRE(bytes[3] == 40); // alpha last
}

TEST_CASE("I-31: GLVertexAttribLayouts uses GL_BGRA for color attributes", "[Graphics][VertexColor][I-31]")
{
    // The audit reads the canonical header that owns TLVertex's
    // attribute pointer setup.  A regression to `GL_RGBA` for
    // either color slot would reintroduce the B-027 R/B swap.
    const std::filesystem::path layouts =
        std::filesystem::path(TESTS_ROOT_DIR).parent_path() / "engine" / "PoseidonGL33" / "GLVertexAttribLayouts.hpp";
    const std::string body = ReadTextFile(layouts);
    REQUIRE_FALSE(body.empty());

    // Two color attribute lines (color + specular) must both use GL_BGRA.
    int bgraColorCalls = 0;
    size_t pos = 0;
    while ((pos = body.find("GL_BGRA, GL_UNSIGNED_BYTE", pos)) != std::string::npos)
    {
        ++bgraColorCalls;
        pos += 1;
    }
    REQUIRE(bgraColorCalls == 2);

    // Negative: no GL_RGBA-with-UNSIGNED_BYTE in the TLVertex setup
    // (which would silently swap channels).
    REQUIRE(body.find("GL_RGBA, GL_UNSIGNED_BYTE") == std::string::npos);
}
