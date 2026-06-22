#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Foundation/Common/Win.h>
#include <Poseidon/Audio/Streaming/WaveLoaders.hpp>
#include <Poseidon/Audio/Streaming/WaveStream.hpp>
#include <Poseidon/Audio/Core/Format/DeltaPack.hpp>
#include "test_fixtures.hpp"
#include <vector>

using namespace Poseidon;

TEST_CASE("SoundLoadMemory: malformed RIFF chunk length does not hang", "[Audio][wav][fuzz]")
{
    // fuzz_wav: a non-fmt chunk with an overflowing length made WaveFile's chunk-search
    // loop seek with no forward progress and spin forever. The loop now bails -- the
    // test returning IS the regression (pre-fix SoundLoadMemory never returns).
    const unsigned char wav[] = {
        'R', 'I', 'F', 'F', 0,    0,    0,    0,    'W', 'A', 'V', 'E', // RIFF / WAVE header
        'J', 'U', 'N', 'K', 0xFF, 0xFF, 0xFF, 0xFF,                     // chunk id != "fmt ", huge length
    };
    WaveStream* ws = SoundLoadMemory(wav, sizeof(wav), ".wav");
    delete ws; // nullptr is fine
    SUCCEED("malformed RIFF chunk length handled without hanging");
}

TEST_CASE("UnpackDelta8/4 clamp out-of-range delta codes", "[Audio][deltapack][fuzz]")
{
    // fuzz_wss: the WSS delta decompressors index a fixed table by the wire code.
    // Delta8 reads a signed char -- code -128 indexed -1, one int before the 255-entry
    // table; Delta4 reads a 4-bit nibble -- code 15 indexed past the 15-entry table
    // (0..14). Both read out of the global table (global-buffer-overflow under ASan);
    // the index is now clamped to the table's last entry.
    SECTION("Delta8: code -128 clamps to the most-negative delta")
    {
        char src[2] = {static_cast<char>(0x80), static_cast<char>(0x80)}; // -128, -128
        short dest[2] = {};
        UnpackD8.Unpack(dest, src, 4, 0);
        REQUIRE(dest[0] == -32768); // clamped to _delta[0] then saturated
    }
    SECTION("Delta4: nibble 15 clamps to the largest delta")
    {
        char src[2] = {static_cast<char>(0xFF), static_cast<char>(0xFF)}; // nibbles 15,15
        short dest[4] = {};
        UnpackD4.Unpack(dest, src, 8, 0);
        REQUIRE(dest[0] == 8192); // clamped to _delta[14]
    }
}
TEST_CASE("WSSLoadFile: accepts synthetic WAV-container fixture", "[Audio]")
{
    const char* path = GET_FIXTURE("audio/click.wss");

    WaveStream* stream = WSSLoadFile(path);
    REQUIRE(stream != nullptr);

    delete stream;
}

TEST_CASE("WSSLoadFile: returns nullptr for nonexistent", "[Audio]")
{
    WaveStream* stream = WSSLoadFile("/nonexistent/file.wss");
    CHECK(stream == nullptr);
}

TEST_CASE("WSSLoadFile: reads synthetic WAV-container fixture", "[Audio]")
{
    const char* path = GET_FIXTURE("audio/click.wss");

    WaveStream* stream = WSSLoadFile(path);
    REQUIRE(stream != nullptr);

    auto size = stream->GetUncompressedSize();
    REQUIRE(size > 0);

    std::vector<char> buf(size);
    auto bytesRead = stream->GetData(buf.data(), 0, size);
    CHECK(bytesRead > 0);

    delete stream;
}

TEST_CASE("SoundLoadFile: loads WAV via generic loader", "[Audio]")
{
    const char* path = GET_FIXTURE("audio/tone.wav");

    WaveStream* stream = SoundLoadFile(path);
    REQUIRE(stream != nullptr);

    WAVEFORMATEX fmt{};
    stream->GetFormat(fmt);
    CHECK(fmt.wFormatTag == WAVE_FORMAT_PCM);
    CHECK(fmt.nChannels == 1);
    CHECK(fmt.nSamplesPerSec == 22050);

    delete stream;
}

TEST_CASE("SoundLoadFile: loads synthetic WAV-container .wss fixture", "[Audio]")
{
    const char* path = GET_FIXTURE("audio/click.wss");

    WaveStream* stream = SoundLoadFile(path);
    REQUIRE(stream != nullptr);

    delete stream;
}

TEST_CASE("SoundLoadFile: returns nullptr for nonexistent", "[Audio]")
{
    WaveStream* stream = SoundLoadFile("/nonexistent/file.wss");
    CHECK(stream == nullptr);
}
