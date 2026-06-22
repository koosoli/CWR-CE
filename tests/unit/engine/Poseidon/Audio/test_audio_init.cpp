#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <Poseidon/Asset/Probes/AudioInit.hpp>
#include <Poseidon/Asset/Probes/SoundPlayer.hpp>
#include <Poseidon/Audio/IAudioSystem.hpp>
#include "test_fixtures.hpp"

using namespace Poseidon;
TEST_CASE("CreateToolAudio returns non-null audio system", "[Audio][tools]")
{
    auto* audio = Poseidon::CreateToolAudio();
    REQUIRE(audio != nullptr);
    REQUIRE(audio->IsReady());
    delete audio;
}

TEST_CASE("SoundPlayer: plays valid WAV file", "[Audio][tools]")
{
    Poseidon::SoundPlayer player;
    REQUIRE(player.isReady());

    const char* path = GET_FIXTURE("audio/tone.wav");
    REQUIRE(player.play(path));
    CHECK(player.duration() > 0.0f);
    CHECK(player.isPlaying());

    player.stop();
    CHECK(!player.isPlaying());
}

TEST_CASE("SoundPlayer: stop is idempotent", "[Audio][tools]")
{
    Poseidon::SoundPlayer player;
    player.stop(); // no crash when nothing is playing
    CHECK(!player.isPlaying());
}
