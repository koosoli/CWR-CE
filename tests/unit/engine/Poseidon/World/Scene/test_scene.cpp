#include <catch2/catch_test_macros.hpp>

#include <Poseidon/AI/AI.hpp>
#include <Poseidon/World/Scene/Scene.hpp>

TEST_CASE("scene.hpp compiles", "[scene]")
{
    SUCCEED("header included successfully");
}

TEST_CASE("Scene skips visible light volumes with missing shapes", "[scene][lights]")
{
    Poseidon::Scene scene;
    Poseidon::Frame frame;

    REQUIRE_NOTHROW(scene.DrawVolumeLight(nullptr, PackedWhite, frame, 1.0f));
}
