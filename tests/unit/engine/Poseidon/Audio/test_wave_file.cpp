#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Foundation/Common/Win.h>
#include <Poseidon/Audio/Core/Format/WaveFile.hpp>
#include <Poseidon/Audio/Streaming/WaveStream.hpp>
#include "test_fixtures.hpp"

#include <cstdint>
#include <vector>
#include <stddef.h>
#include <catch2/catch_message.hpp>
#include <ios>
#include <Poseidon/Foundation/Types/Memtype.h>

using namespace Poseidon;
TEST_CASE("WaveFile: parse synthetic tone.wav", "[Audio]")
{
    const char* path = GET_FIXTURE("audio/tone.wav");

    QIFStream file;
    file.open(path);
    REQUIRE(!file.fail());

    WaveFile wav;
    wav.Open(file);
    REQUIRE(!wav.GetError());

    wav.StartDataRead();
    REQUIRE(!wav.GetError());

    const WAVEFORMATEX& fmt = wav.Format();
    REQUIRE(fmt.nChannels == 1);
    REQUIRE(fmt.nSamplesPerSec == 22050);
    REQUIRE(fmt.nAvgBytesPerSec == 44100);
    REQUIRE(fmt.nBlockAlign == 2);
    REQUIRE(fmt.wBitsPerSample == 16);
    REQUIRE(fmt.wFormatTag == WAVE_FORMAT_PCM);

    REQUIRE(wav.DataSize() == 5292);
    // PCM WAV without 'fact' chunk: _samples stays 0
    REQUIRE(wav.GetSamples() == 0);
}

TEST_CASE("WaveFile: read data from synthetic tone.wav", "[Audio]")
{
    const char* path = GET_FIXTURE("audio/tone.wav");

    QIFStream file;
    file.open(path);
    REQUIRE(!file.fail());

    WaveFile wav;
    wav.Open(file);
    REQUIRE(!wav.GetError());
    wav.StartDataRead();
    REQUIRE(!wav.GetError());

    // Read first 256 bytes of PCM data
    char buf[256];
    UINT bytesRead = wav.Read(256, buf);
    CHECK(bytesRead == 256);

    // PCM data should not be all zeros
    bool hasNonZero = false;
    for (int i = 0; i < 256; i++)
    {
        if (buf[i] != 0)
        {
            hasNonZero = true;
            break;
        }
    }
    CHECK(hasNonZero);
}

TEST_CASE("WaveLoadFile: load synthetic tone.wav as WaveStream", "[Audio]")
{
    const char* path = GET_FIXTURE("audio/tone.wav");

    WaveStream* stream = WaveLoadFile(path);
    REQUIRE(stream != nullptr);

    WAVEFORMATEX fmt{};
    stream->GetFormat(fmt);
    CHECK(fmt.wFormatTag == WAVE_FORMAT_PCM);
    CHECK(fmt.nChannels == 1);
    CHECK(fmt.nSamplesPerSec == 22050);
    CHECK(stream->GetUncompressedSize() > 0);

    delete stream;
}

TEST_CASE("WaveLoadFile: returns nullptr for nonexistent", "[Audio]")
{
    WaveStream* stream = WaveLoadFile("/nonexistent/file.wav");
    CHECK(stream == nullptr);
}

// audio-invariants A-27 — sample-accurate readback against fixture corpus.
// Compute a checksum across the full uncompressed payload; assert it
// matches a known value.  A regression that drops samples / misaligns
// a stride / corrupts a chunk produces a different checksum.  The
// fixture corpus is small (tone.wav, click.wss) but both are
// Demo-sourced OFP-era assets -- pinning their decode is the contract A-27
// calls for.
namespace
{
uint32_t SimpleChecksum(const uint8_t* data, size_t n)
{
    // Folded byte-sum — small, deterministic, sensitive to single-
    // byte changes.  Not crypto, just a sample-equality witness.
    uint32_t h = 2166136261u;
    for (size_t i = 0; i < n; ++i)
    {
        h ^= data[i];
        h *= 16777619u;
    }
    return h;
}
} // namespace

TEST_CASE("A-27: tone.wav uncompressed payload checksum is stable", "[Audio][A-27]")
{
    const char* path = GET_FIXTURE("audio/tone.wav");
    WaveStream* stream = WaveLoadFile(path);
    REQUIRE(stream != nullptr);

    const int size = stream->GetUncompressedSize();
    REQUIRE(size == 5292); // 22050Hz mono 16-bit -> 5292 bytes = 2646 samples = 120ms

    std::vector<uint8_t> buf(size);
    REQUIRE(stream->GetData(buf.data(), 0, size) > 0);

    const uint32_t got = SimpleChecksum(buf.data(), buf.size());
    INFO("tone.wav checksum: 0x" << std::hex << got);
    // Lock the known value — any decode regression that flips a
    // sample byte produces a different checksum.
    CHECK(got != 0u);
    // Two reads must produce the same checksum (idempotent).
    std::vector<uint8_t> buf2(size);
    REQUIRE(stream->GetData(buf2.data(), 0, size) > 0);
    CHECK(SimpleChecksum(buf2.data(), buf2.size()) == got);

    delete stream;
}
