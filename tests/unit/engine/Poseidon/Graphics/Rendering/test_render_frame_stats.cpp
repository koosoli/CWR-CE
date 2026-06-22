#include <catch2/catch_test_macros.hpp>

#include <Poseidon/Graphics/Rendering/Frame/BuildFrame.hpp>
#include <Poseidon/Graphics/Rendering/Frame/FrameStats.hpp>

// CountFrameStats — the pure fold over the Frame value that feeds the
// --render-frame-log summary line.  Replaces the dispatch-walk stats
// backend: same numbers, no backend object, no call sequence.

namespace v2 = Poseidon::render::frame;

namespace
{

v2::SceneInputs makeMinimal()
{
    v2::SceneInputs s;
    s.camera.viewport = {0, 0, 800, 600};
    s.flags.hudEnabled = false;
    return s;
}

v2::SceneDraw makeDraw()
{
    v2::SceneDraw d;
    d.descriptor.pass = Poseidon::render::PassKind::WorldOpaque;
    d.descriptor.depth = Poseidon::render::DepthMode::Normal;
    d.descriptor.blend = Poseidon::render::BlendMode::Opaque;
    d.indexCount = 3;
    return d;
}

} // namespace

TEST_CASE("Frame/FrameStats: empty Frame folds to zeroes", "[render-frame][frame-stats]")
{
    const v2::Frame f; // truly empty — no passes at all
    const v2::FrameStats s = v2::CountFrameStats(f);

    REQUIRE(s.passCount == 0);
    REQUIRE(s.drawCount == 0);
    REQUIRE(s.maxDrawsInPass == 0);
}

TEST_CASE("Frame/FrameStats: counts passes and draws on a one-draw frame", "[render-frame][frame-stats]")
{
    auto si = makeMinimal();
    si.worldOpaqueDraws.push_back(makeDraw());

    const v2::FrameStats s = v2::CountFrameStats(v2::BuildFrame(si));

    REQUIRE(s.passCount == 1);
    REQUIRE(s.drawCount == 1);
    REQUIRE(s.maxDrawsInPass == 1);
}

TEST_CASE("Frame/FrameStats: maxDrawsInPass tracks the busiest pass", "[render-frame][frame-stats]")
{
    auto si = makeMinimal();
    // 3 draws in WorldOpaque, 1 in WorldCutout — max is 3.
    si.worldOpaqueDraws.push_back(makeDraw());
    si.worldOpaqueDraws.push_back(makeDraw());
    si.worldOpaqueDraws.push_back(makeDraw());

    v2::SceneDraw cd = makeDraw();
    cd.descriptor.pass = Poseidon::render::PassKind::WorldCutout;
    si.worldCutoutDraws.push_back(cd);

    const v2::FrameStats s = v2::CountFrameStats(v2::BuildFrame(si));

    REQUIRE(s.passCount == 2);
    REQUIRE(s.drawCount == 4);
    REQUIRE(s.maxDrawsInPass == 3);
}

TEST_CASE("Frame/FrameStats: a pure fold never accumulates across frames", "[render-frame][frame-stats]")
{
    auto s1 = makeMinimal();
    s1.worldOpaqueDraws.push_back(makeDraw());
    s1.worldOpaqueDraws.push_back(makeDraw());

    auto s2 = makeMinimal();
    s2.worldOpaqueDraws.push_back(makeDraw());

    REQUIRE(v2::CountFrameStats(v2::BuildFrame(s1)).drawCount == 2);
    REQUIRE(v2::CountFrameStats(v2::BuildFrame(s2)).drawCount == 1);
}
