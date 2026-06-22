#pragma once
// Opus codec wrapper for VoN — 16kHz mono, 20ms frames

#include <Poseidon/Audio/Voice/VonCodec.hpp>

// opus.h C types — keep at global scope.
struct OpusEncoder;
struct OpusDecoder;

namespace Poseidon
{
class OpusCodec : public VoNCodec
{
  public:
    // bitrate in bps (default 16000 = 16kbps)
    explicit OpusCodec(int sampleRate = 16000, int bitrate = 16000);
    ~OpusCodec() override;

    OpusCodec(const OpusCodec&) = delete;
    OpusCodec& operator=(const OpusCodec&) = delete;

    int encode(const int16_t* pcm, int samples, uint8_t* out, int maxOut) override;
    int decode(const uint8_t* data, int bytes, int16_t* pcm, int maxSamples) override;

    int frameSize() const override { return _frameSize; }
    int sampleRate() const override { return _sampleRate; }
    int maxEncodedSize() const override { return 256; }

    void getInfo(std::vector<uint8_t>& out) const override;

    bool isValid() const { return _encoder && _decoder; }

    // Restore from network info
    static OpusCodec* fromInfo(const uint8_t* data, int size);

  private:
    OpusEncoder* _encoder = nullptr;
    OpusDecoder* _decoder = nullptr;
    int _sampleRate;
    int _bitrate;
    int _frameSize; // samples per frame (sampleRate * 0.020)
};

} // namespace Poseidon
