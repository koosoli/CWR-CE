#include <Poseidon/Audio/Voice/OpusCodec.hpp>
#include <opus/opus.h>
#include <cstring>
#include <opus_defines.h>
#include <vector>

namespace Poseidon
{
OpusCodec::OpusCodec(int sampleRate, int bitrate)
    : _sampleRate(sampleRate), _bitrate(bitrate), _frameSize(sampleRate / 50) // 20ms
{
    int err = 0;
    _encoder = opus_encoder_create(sampleRate, 1, OPUS_APPLICATION_VOIP, &err);
    if (err != OPUS_OK || !_encoder)
    {
        _encoder = nullptr;
        return;
    }

    opus_encoder_ctl(_encoder, OPUS_SET_BITRATE(bitrate));
    opus_encoder_ctl(_encoder, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));
    opus_encoder_ctl(_encoder, OPUS_SET_COMPLEXITY(5));

    _decoder = opus_decoder_create(sampleRate, 1, &err);
    if (err != OPUS_OK || !_decoder)
    {
        _decoder = nullptr;
    }
}

OpusCodec::~OpusCodec()
{
    if (_encoder)
        opus_encoder_destroy(_encoder);
    if (_decoder)
        opus_decoder_destroy(_decoder);
}

int OpusCodec::encode(const int16_t* pcm, int samples, uint8_t* out, int maxOut)
{
    if (!_encoder || samples != _frameSize)
        return 0;
    int ret = opus_encode(_encoder, pcm, samples, out, maxOut);
    return ret > 0 ? ret : 0;
}

int OpusCodec::decode(const uint8_t* data, int bytes, int16_t* pcm, int maxSamples)
{
    if (!_decoder || maxSamples < _frameSize)
        return 0;
    int ret;
    if (data && bytes > 0)
        ret = opus_decode(_decoder, data, bytes, pcm, _frameSize, 0);
    else
        ret = opus_decode(_decoder, nullptr, 0, pcm, _frameSize, 0); // PLC
    return ret > 0 ? ret : 0;
}

void OpusCodec::getInfo(std::vector<uint8_t>& out) const
{
    // Format: [sampleRate:4][bitrate:4]
    out.resize(8);
    std::memcpy(out.data(), &_sampleRate, 4);
    std::memcpy(out.data() + 4, &_bitrate, 4);
}

OpusCodec* OpusCodec::fromInfo(const uint8_t* data, int size)
{
    if (size < 8)
        return nullptr;
    int sr = 0, br = 0;
    std::memcpy(&sr, data, 4);
    std::memcpy(&br, data + 4, 4);
    auto* c = new OpusCodec(sr, br);
    if (!c->isValid())
    {
        delete c;
        return nullptr;
    }
    return c;
}

} // namespace Poseidon
