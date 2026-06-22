#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <Poseidon/Audio/Voice/OpusCodec.hpp>
#include <Poseidon/Audio/Voice/PCMCodec.hpp>
#include <cmath>
#include <vector>
#include <numeric>
#include <stdint.h>

using namespace Poseidon;
using Catch::Approx;

static constexpr int RATE = 16000;
static constexpr int FRAME = RATE / 50; // 320 samples = 20ms

// Generate sine wave frame
static void genSine(int16_t* buf, int samples, float freq, float amp, int offset = 0)
{
    for (int i = 0; i < samples; ++i)
    {
        float t = static_cast<float>(offset + i) / RATE;
        buf[i] = static_cast<int16_t>(amp * std::sin(2.0f * 3.14159265f * freq * t));
    }
}

static double rms(const int16_t* buf, int n)
{
    double sum = 0;
    for (int i = 0; i < n; ++i)
        sum += static_cast<double>(buf[i]) * buf[i];
    return std::sqrt(sum / n);
}

// --- Opus tests ---

TEST_CASE("OpusCodec creation", "[VoN][codec]")
{
    OpusCodec codec(RATE, 16000);
    REQUIRE(codec.isValid());
    REQUIRE(codec.frameSize() == FRAME);
    REQUIRE(codec.sampleRate() == RATE);
    REQUIRE(codec.channels() == 1);
    REQUIRE(codec.maxEncodedSize() > 0);
}

TEST_CASE("OpusCodec encode silence round-trip", "[VoN][codec]")
{
    OpusCodec codec(RATE, 16000);
    REQUIRE(codec.isValid());

    std::vector<int16_t> pcm(FRAME, 0);
    std::vector<uint8_t> encoded(codec.maxEncodedSize());

    int encBytes = codec.encode(pcm.data(), FRAME, encoded.data(), (int)encoded.size());
    REQUIRE(encBytes > 0);
    REQUIRE(encBytes < FRAME * 2); // compressed smaller than raw

    std::vector<int16_t> decoded(FRAME);
    int decSamples = codec.decode(encoded.data(), encBytes, decoded.data(), FRAME);
    REQUIRE(decSamples == FRAME);

    // Decoded silence should have very low RMS
    REQUIRE(rms(decoded.data(), FRAME) < 100.0);
}

TEST_CASE("OpusCodec encode sine round-trip", "[VoN][codec]")
{
    OpusCodec codec(RATE, 24000);
    REQUIRE(codec.isValid());

    std::vector<int16_t> pcm(FRAME);
    genSine(pcm.data(), FRAME, 440.0f, 16000.0f);

    std::vector<uint8_t> encoded(codec.maxEncodedSize());
    int encBytes = codec.encode(pcm.data(), FRAME, encoded.data(), (int)encoded.size());
    REQUIRE(encBytes > 0);

    std::vector<int16_t> decoded(FRAME);
    int decSamples = codec.decode(encoded.data(), encBytes, decoded.data(), FRAME);
    REQUIRE(decSamples == FRAME);

    // Decoded signal should have significant energy
    double origRms = rms(pcm.data(), FRAME);
    double decRms = rms(decoded.data(), FRAME);
    REQUIRE(decRms > origRms * 0.3); // at least 30% of original energy preserved
}

TEST_CASE("OpusCodec multi-frame continuity", "[VoN][codec]")
{
    OpusCodec codec(RATE, 16000);
    REQUIRE(codec.isValid());

    constexpr int FRAMES = 10;
    std::vector<int16_t> allDecoded;

    for (int f = 0; f < FRAMES; ++f)
    {
        std::vector<int16_t> pcm(FRAME);
        genSine(pcm.data(), FRAME, 300.0f, 10000.0f, f * FRAME);

        std::vector<uint8_t> enc(codec.maxEncodedSize());
        int encBytes = codec.encode(pcm.data(), FRAME, enc.data(), (int)enc.size());
        REQUIRE(encBytes > 0);

        std::vector<int16_t> dec(FRAME);
        int decSamples = codec.decode(enc.data(), encBytes, dec.data(), FRAME);
        REQUIRE(decSamples == FRAME);
        allDecoded.insert(allDecoded.end(), dec.begin(), dec.end());
    }

    // All decoded frames should have signal
    REQUIRE(rms(allDecoded.data(), (int)allDecoded.size()) > 1000.0);
}

TEST_CASE("OpusCodec PLC (packet loss concealment)", "[VoN][codec]")
{
    OpusCodec codec(RATE, 16000);
    REQUIRE(codec.isValid());

    // Feed one real frame first
    std::vector<int16_t> pcm(FRAME);
    genSine(pcm.data(), FRAME, 440.0f, 10000.0f);
    std::vector<uint8_t> enc(codec.maxEncodedSize());
    int encBytes = codec.encode(pcm.data(), FRAME, enc.data(), (int)enc.size());
    REQUIRE(encBytes > 0);

    std::vector<int16_t> dec(FRAME);
    codec.decode(enc.data(), encBytes, dec.data(), FRAME);

    // Now request PLC frame (null data)
    std::vector<int16_t> plc(FRAME);
    int plcSamples = codec.decode(nullptr, 0, plc.data(), FRAME);
    REQUIRE(plcSamples == FRAME);
    // PLC should produce some output (concealment, not crash)
}

TEST_CASE("OpusCodec wrong frame size rejected", "[VoN][codec]")
{
    OpusCodec codec(RATE, 16000);
    std::vector<int16_t> pcm(FRAME + 1, 0);
    std::vector<uint8_t> enc(codec.maxEncodedSize());
    int ret = codec.encode(pcm.data(), FRAME + 1, enc.data(), (int)enc.size());
    REQUIRE(ret == 0);
}

TEST_CASE("OpusCodec getInfo/fromInfo round-trip", "[VoN][codec]")
{
    OpusCodec original(RATE, 24000);
    REQUIRE(original.isValid());

    std::vector<uint8_t> info;
    original.getInfo(info);
    REQUIRE(info.size() == 8);

    auto* restored = OpusCodec::fromInfo(info.data(), (int)info.size());
    REQUIRE(restored != nullptr);
    REQUIRE(restored->sampleRate() == RATE);
    REQUIRE(restored->frameSize() == FRAME);
    REQUIRE(restored->isValid());

    // Verify restored codec can encode/decode
    std::vector<int16_t> pcm(FRAME);
    genSine(pcm.data(), FRAME, 440.0f, 10000.0f);
    std::vector<uint8_t> enc(restored->maxEncodedSize());
    int encBytes = restored->encode(pcm.data(), FRAME, enc.data(), (int)enc.size());
    REQUIRE(encBytes > 0);

    delete restored;
}

// --- PCM codec tests ---

TEST_CASE("PCMCodec identity round-trip", "[VoN][codec]")
{
    PCMCodec codec(RATE);
    REQUIRE(codec.frameSize() == FRAME);
    REQUIRE(codec.sampleRate() == RATE);

    std::vector<int16_t> pcm(FRAME);
    genSine(pcm.data(), FRAME, 440.0f, 16000.0f);

    std::vector<uint8_t> enc(codec.maxEncodedSize());
    int encBytes = codec.encode(pcm.data(), FRAME, enc.data(), (int)enc.size());
    REQUIRE(encBytes == FRAME * 2);

    std::vector<int16_t> dec(FRAME);
    int decSamples = codec.decode(enc.data(), encBytes, dec.data(), FRAME);
    REQUIRE(decSamples == FRAME);

    // PCM should be exact
    for (int i = 0; i < FRAME; ++i)
        REQUIRE(dec[i] == pcm[i]);
}

TEST_CASE("PCMCodec PLC produces silence", "[VoN][codec]")
{
    PCMCodec codec(RATE);
    std::vector<int16_t> dec(FRAME, 999);
    int ret = codec.decode(nullptr, 0, dec.data(), FRAME);
    REQUIRE(ret == FRAME);
    for (int i = 0; i < FRAME; ++i)
        REQUIRE(dec[i] == 0);
}
