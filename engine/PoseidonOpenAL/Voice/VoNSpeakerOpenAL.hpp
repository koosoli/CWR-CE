#pragma once

#include <Poseidon/Audio/Voice/VoiceBackend.hpp>
#include <Poseidon/Audio/Voice/VonApp.hpp>
#include <PoseidonOpenAL/OpenALRuntime.hpp>
#include <Poseidon/Foundation/Framework/Log.hpp>
#include <cstring>
#include <cmath>


namespace Poseidon
{
class VoNSpeakerOpenAL : public IVoiceSpeakerBackend
{
  public:
    void init() override
    {
        if (!OpenALRuntime::EnsureLoaded())
            return;

        alGenSources(1, &source);
        if (alGetError() != AL_NO_ERROR)
        {
            source = 0;
            return;
        }
        alGenBuffers(NUM_BUFS, bufs);
        if (alGetError() != AL_NO_ERROR)
        {
            alDeleteSources(1, &source);
            source = 0;
            return;
        }
        active = false;
        drainFrames = 0;
        freeBufCount = NUM_BUFS;
        for (int i = 0; i < NUM_BUFS; ++i)
            freeBufs[i] = bufs[i];
    }

    void destroy() override
    {
        if (!source)
            return;

        alSourceStop(source);
        ALint queued = 0;
        alGetSourcei(source, AL_BUFFERS_QUEUED, &queued);
        while (queued-- > 0)
        {
            ALuint buf;
            alSourceUnqueueBuffers(source, 1, &buf);
        }
        alDeleteSources(1, &source);
        alDeleteBuffers(NUM_BUFS, bufs);
        source = 0;
        active = false;
        levelValue = 0.0f;
    }

    void setPosition(float x, float y, float z) override
    {
        if (source)
            alSource3f(source, AL_POSITION, x, y, z);
    }

    void stopStream() override
    {
        if (!source)
            return;
        alSourceStop(source);
        ALint queued = 0;
        alGetSourcei(source, AL_BUFFERS_QUEUED, &queued);
        while (queued-- > 0)
        {
            ALuint buf;
            alSourceUnqueueBuffers(source, 1, &buf);
        }
        active = false;
        drainFrames = 0;
        // Return all buffers to free pool
        freeBufCount = NUM_BUFS;
        for (int i = 0; i < NUM_BUFS; ++i)
            freeBufs[i] = bufs[i];
    }

    bool feed(VoNClient* client, uint32_t channel) override
    {
        if (!source)
            return false;

        int16_t decoded[FRAME_SAMPLES];

        // Recycle processed buffers back to free pool
        ALint processed = 0;
        alGetSourcei(source, AL_BUFFERS_PROCESSED, &processed);
        while (processed > 0)
        {
            ALuint buf;
            alSourceUnqueueBuffers(source, 1, &buf);
            freeBufs[freeBufCount++] = buf;
            --processed;
        }

        // Fill free buffers with data from jitter buffer
        int queued = 0;
        while (freeBufCount > 0)
        {
            if (client->pullSpeaker(channel, decoded, FRAME_SAMPLES) != FRAME_SAMPLES)
                break;
            ALuint buf = freeBufs[--freeBufCount];
            alBufferData(buf, AL_FORMAT_MONO16, decoded,
                         FRAME_SAMPLES * 2, SAMPLE_RATE);
            alSourceQueueBuffers(source, 1, &buf);
            ++queued;
            updateLevel(decoded, FRAME_SAMPLES);
        }

        bool gotData = queued > 0;

        if (!active)
        {
            // Need enough buffered before starting (A3: ~100ms)
            ALint totalQueued = 0;
            alGetSourcei(source, AL_BUFFERS_QUEUED, &totalQueued);
            if (totalQueued >= START_THRESHOLD)
            {
                alSourcePlay(source);
                active = true;
                drainFrames = 0;
                LOG_DEBUG(Audio, "VoN spk: started ch={} queued={}", channel, totalQueued);
            }
            return false;
        }

        // A3-style: if source stopped (underrun), just restart — no reset
        ALint state;
        alGetSourcei(source, AL_SOURCE_STATE, &state);
        if (state != AL_PLAYING)
        {
            ALint stillQueued = 0;
            alGetSourcei(source, AL_BUFFERS_QUEUED, &stillQueued);
            if (stillQueued > 0)
            {
                alSourcePlay(source);
                LOG_TRACE(Audio, "VoN spk: resumed ch={} queued={}", channel, stillQueued);
            }
        }

        // Drain timeout: count frames with no real data from jitter buffer.
        // A3 uses m_noMsgCount > 10 at 50ms = 500ms; we use 30 at ~16ms = ~500ms.
        if (gotData)
        {
            drainFrames = 0;
        }
        else
        {
            ++drainFrames;
            if (drainFrames > DRAIN_LIMIT)
            {
                LOG_DEBUG(Audio, "VoN spk: drain timeout ch={}", channel);
                stopStream();
                levelValue = 0.0f;
            }
        }

        return gotData;
    }

    bool isActive() const override { return active; }
    float level() const override { return levelValue; }

  private:
    void updateLevel(const int16_t* pcm, int n)
    {
        double sum = 0;
        for (int i = 0; i < n; ++i)
            sum += static_cast<double>(pcm[i]) * pcm[i];
        levelValue = static_cast<float>(std::sqrt(sum / n) / 32768.0);
    }

    ALuint source = 0;
    static constexpr int NUM_BUFS = 24;
    static constexpr int FRAME_SAMPLES = 320;
    static constexpr int SAMPLE_RATE = 16000;
    static constexpr int START_THRESHOLD = 5;
    static constexpr int DRAIN_LIMIT = 30;

    ALuint bufs[NUM_BUFS] = {};
    bool active = false;
    int freeBufCount = 0;
    ALuint freeBufs[NUM_BUFS] = {};
    int drainFrames = 0;
    float levelValue = 0.0f;
};

} // namespace Poseidon
