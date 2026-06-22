#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <Poseidon/Graphics/Core/ZBiasMath.hpp>
#include <Poseidon/Graphics/Dummy/EngineDummy.hpp>

using Poseidon::EngineDummy;

using Poseidon::render::zbias::SoftwareCoefs;

// I-11: backend capability flags are truthful — `CanZBias()` returns
// true iff the backend actually implements hardware z-bias.  Lying
// caused B-024 (terrain occluded UI: GL33 said "I do hardware
// z-bias" but didn't actually apply it, so software fallback skipped
// too).
//
// These tests pin:
//   - The capability flag's current value (false on both backends
//     that ship today — GL33 + Dummy).  A flip-to-true without
//     simultaneous hardware-z-bias implementation regresses B-024.
//   - The software-z-bias coefficient math the false-branch callers
//     in `transLight.cpp` / `v3QuadsP3.cpp` apply via `GetZCoefs`.

TEST_CASE("CanZBias is false on shipping backends (B-024 contract)", "[Graphics][ZBias][I-11]")
{
    // Dummy backend reports false — software z-bias path is active.
    // EngineDummy's GetZCoefs returns (0, 1) — i.e., no-op coefs
    // because the dummy backend has no real frame, but the *contract*
    // is "CanZBias false implies callers apply zMult/zAdd".
    EngineDummy d;
    REQUIRE(d.CanZBias() == false);

    // EngineGL33::CanZBias is hardcoded to false (see the comment in
    // EngineGL33_Draw.cpp).  We can't instantiate EngineGL33 without
    // a GL context, so this test pins the dummy + the file-level
    // contract.  If a future backend switches to true, the
    // accompanying hardware-z-bias implementation needs review.
}

TEST_CASE("SoftwareCoefs: zero bias yields identity transform", "[Graphics][ZBias][I-11]")
{
    const auto c = SoftwareCoefs(0);
    REQUIRE(c.zMult == Catch::Approx(1.0f));
    REQUIRE(c.zAdd == Catch::Approx(0.0f));
}

TEST_CASE("SoftwareCoefs: positive bias produces depth offset and shrink", "[Graphics][ZBias][I-11]")
{
    // Typical bias values used by the engine: 0..160 (range from
    // EngineGL33::SetBias).  Coefficients must be deterministic
    // and applied consistently across callers.
    const auto c = SoftwareCoefs(100);
    REQUIRE(c.zMult == Catch::Approx(1.0f - 100 * 1e-7f).margin(1e-9f));
    REQUIRE(c.zAdd == Catch::Approx(100 * -2e-7f).margin(1e-9f));

    // Bias > 0 must produce zMult < 1 so the perturbed projection's
    // z scale is reduced (pushing fragments forward); and zAdd < 0
    // so the constant offset pulls them further forward.  These
    // signs are what `mcAdjusted(2, 2) = mc(2, 2) * zMult + zAdd`
    // relies on in transLight.cpp.
    REQUIRE(c.zMult < 1.0f);
    REQUIRE(c.zAdd < 0.0f);
}

TEST_CASE("SoftwareCoefs: bias scales linearly", "[Graphics][ZBias][I-11]")
{
    const auto c1 = SoftwareCoefs(10);
    const auto c2 = SoftwareCoefs(20);
    REQUIRE((1.0f - c2.zMult) == Catch::Approx(2.0f * (1.0f - c1.zMult)).margin(1e-9f));
    REQUIRE(c2.zAdd == Catch::Approx(2.0f * c1.zAdd).margin(1e-9f));
}

TEST_CASE("SoftwareCoefs: negative bias inverts the offset direction", "[Graphics][ZBias][I-11]")
{
    const auto c = SoftwareCoefs(-50);
    REQUIRE(c.zMult > 1.0f); // pushes fragments back
    REQUIRE(c.zAdd > 0.0f);
}
