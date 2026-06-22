#pragma once
// Uncompressed PCM codec — passthrough for testing

#include <Poseidon/Audio/Voice/VonCodec.hpp>
#include <cstring>
#include <algorithm>


namespace Poseidon
{
class PCMCodec : public VoNCodec
{
  public:
    explicit PCMCodec(int sampleRate = 16000) : _sampleRate(sampleRate), _frameSize(sampleRate / 50) {}

    int encode(const int16_t* pcm, int samples, uint8_t* out, int maxOut) override
    {
        if (samples != _frameSize)
            return 0;
        int bytes = samples * 2;
        if (bytes > maxOut)
            return 0;
        std::memcpy(out, pcm, bytes);
        return bytes;
    }

    int decode(const uint8_t* data, int bytes, int16_t* pcm, int maxSamples) override
    {
        if (maxSamples < _frameSize)
            return 0;
        if (data && bytes >= _frameSize * 2)
        {
            std::memcpy(pcm, data, _frameSize * 2);
        }
        else
        {
            std::memset(pcm, 0, _frameSize * 2); // silence for PLC
        }
        return _frameSize;
    }

    int frameSize() const override { return _frameSize; }
    int sampleRate() const override { return _sampleRate; }
    int maxEncodedSize() const override { return _frameSize * 2; }

    void getInfo(std::vector<uint8_t>& out) const override
    {
        out.resize(4);
        std::memcpy(out.data(), &_sampleRate, 4);
    }

  private:
    int _sampleRate;
    int _frameSize;
};

} // namespace Poseidon
