#pragma once
// VoN codec abstraction — encode/decode voice frames

#include <cstdint>
#include <vector>


namespace Poseidon
{
class VoNCodec
{
  public:
    virtual ~VoNCodec() = default;

    // Encode PCM samples into compressed frame. Returns encoded byte count, 0 on error.
    virtual int encode(const int16_t* pcm, int samples, uint8_t* out, int maxOut) = 0;

    // Decode compressed frame into PCM samples. Returns decoded sample count, 0 on error.
    // If data==nullptr, generates concealment frame (PLC).
    virtual int decode(const uint8_t* data, int bytes, int16_t* pcm, int maxSamples) = 0;

    virtual int frameSize() const = 0;      // samples per frame
    virtual int sampleRate() const = 0;     // Hz
    virtual int maxEncodedSize() const = 0; // worst-case bytes per frame
    virtual int channels() const { return 1; }

    // Codec info for network exchange
    virtual void getInfo(std::vector<uint8_t>& out) const = 0;
};

} // namespace Poseidon
