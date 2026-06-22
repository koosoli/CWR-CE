#include <PoseidonOpenAL/Voice/MicLoopbackOpenAL.hpp>
#include <Poseidon/Foundation/Logging/Logging.hpp>

#include <Poseidon/Audio/Voice/VonCapture.hpp>
#include <PoseidonOpenAL/OpenALRuntime.hpp>

#include <algorithm>
#include <cstdint>


namespace Poseidon
{
MicLoopbackOpenAL::~MicLoopbackOpenAL()
{
    close();
}

bool MicLoopbackOpenAL::open(int sampleRate)
{
    if (_source != 0)
        return true;
    if (!OpenALRuntime::EnsureLoaded())
    {
        LOG_ERROR(Audio, "MicLoopback: OpenAL runtime unavailable ({})", OpenALRuntime::LastError());
        return false;
    }

    ALuint src = 0;
    alGetError();
    alGenSources(1, &src);
    if (alGetError() != AL_NO_ERROR || src == 0)
    {
        LOG_ERROR(Audio, "MicLoopback: alGenSources failed");
        return false;
    }

    // Bypass listener / category attenuation: this is a diagnostic
    // tool, the user wants to hear their own mic clearly regardless
    // of the Speech volume slider.  AL_GAIN at 1.0 + relative source
    // means it plays at full level without spatialisation.
    alSource3f(src, AL_POSITION, 0.f, 0.f, 0.f);
    alSource3f(src, AL_VELOCITY, 0.f, 0.f, 0.f);
    alSourcei(src, AL_LOOPING, AL_FALSE);
    alSourcei(src, AL_SOURCE_RELATIVE, AL_TRUE);
    alSourcef(src, AL_GAIN, 1.0f);

    ALuint bufs[kBufferCount];
    alGenBuffers(kBufferCount, bufs);
    if (alGetError() != AL_NO_ERROR)
    {
        LOG_ERROR(Audio, "MicLoopback: alGenBuffers failed");
        alDeleteSources(1, &src);
        return false;
    }

    _bufferPool.assign(bufs, bufs + kBufferCount);
    _source = src;
    _sampleRate = sampleRate;
    return true;
}

void MicLoopbackOpenAL::close()
{
    if (_source == 0)
        return;

    alSourceStop(_source);

    // Reclaim queued buffers before delete.
    ALint queued = 0;
    alGetSourcei(_source, AL_BUFFERS_QUEUED, &queued);
    while (queued-- > 0)
    {
        ALuint b = 0;
        alSourceUnqueueBuffers(_source, 1, &b);
        if (b)
            _bufferPool.push_back(b);
    }

    if (!_bufferPool.empty())
    {
        alDeleteBuffers((ALsizei)_bufferPool.size(), reinterpret_cast<const ALuint*>(_bufferPool.data()));
        _bufferPool.clear();
    }

    alDeleteSources(1, &_source);
    _source = 0;
    _sampleRate = 0;
}

void MicLoopbackOpenAL::tick(VoNCapture& capture)
{
    if (_source == 0 || !capture.isCapturing())
        return;

    // Recycle any buffers the AL has finished playing.
    ALint processed = 0;
    alGetSourcei(_source, AL_BUFFERS_PROCESSED, &processed);
    while (processed-- > 0)
    {
        ALuint b = 0;
        alSourceUnqueueBuffers(_source, 1, &b);
        if (b)
            _bufferPool.push_back(b);
    }

    // Drain the capture into whatever pool buffers we have free.  Stop
    // when either the capture runs dry or we're out of pool buffers —
    // dropping a frame here is cheaper than blocking on AL.
    int avail = capture.availableSamples();
    while (avail >= kSamplesPerBuffer && !_bufferPool.empty())
    {
        int16_t scratch[kSamplesPerBuffer];
        int got = capture.read(scratch, kSamplesPerBuffer);
        if (got <= 0)
            break;

        ALuint b = _bufferPool.back();
        _bufferPool.pop_back();
        alBufferData(b, AL_FORMAT_MONO16, scratch, got * (ALsizei)sizeof(int16_t), _sampleRate);
        alSourceQueueBuffers(_source, 1, &b);
        avail -= got;
    }

    // (Re)start the source if it stalled because the queue ran dry
    // mid-stream.  Initial start happens here too on the first frame
    // that successfully queues a buffer.
    ALint state = 0;
    alGetSourcei(_source, AL_SOURCE_STATE, &state);
    ALint queued = 0;
    alGetSourcei(_source, AL_BUFFERS_QUEUED, &queued);
    if (state != AL_PLAYING && queued > 0)
        alSourcePlay(_source);
}

} // namespace Poseidon
