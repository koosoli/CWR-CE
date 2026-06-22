#pragma once

#include <atomic>
#include <cstdint>
#include <deque>
#include <mutex>
#include <vector>

namespace Poseidon
{
namespace Audio
{

// One slice of decoded PCM produced by a worker, waiting to be
// uploaded into an AL buffer + queued onto a source on the main
// thread.  Owns its bytes.
struct DecodedChunk
{
    std::vector<uint8_t> pcm;
    // Stream byte offset where this chunk starts — used to compute
    // playback position when AL_BUFFERS_PROCESSED ticks.
    int64_t streamOffset = 0;
    // True for the chunk that ends at EOF on a non-looping stream.
    // Tells the main thread to stop kicking new decodes once this
    // one has been queued.
    bool isLastChunk = false;
};

// SPSC-shaped queue (worker pushes, main thread pops) protected by a
// mutex.  Multi-producer would need a different shape; with the
// "one decode in flight at a time per wave" design we have exactly
// one producer.  The mutex cost is negligible vs the decode itself
// (~5 ms per chunk for Vorbis).
class ChunkQueue
{
  public:
    void Push(DecodedChunk chunk)
    {
        std::lock_guard<std::mutex> lock(_mtx);
        _chunks.emplace_back(std::move(chunk));
    }

    bool TryPop(DecodedChunk& out)
    {
        std::lock_guard<std::mutex> lock(_mtx);
        if (_chunks.empty())
        {
            return false;
        }
        out = std::move(_chunks.front());
        _chunks.pop_front();
        return true;
    }

    size_t Size() const
    {
        std::lock_guard<std::mutex> lock(_mtx);
        return _chunks.size();
    }

    void Clear()
    {
        std::lock_guard<std::mutex> lock(_mtx);
        _chunks.clear();
    }

  private:
    mutable std::mutex       _mtx;
    std::deque<DecodedChunk> _chunks;
};

// Per-wave streaming state.  Members live alongside WaveOAL but are
// only touched when the wave is in streaming mode.
struct StreamingBuffers
{
    // Number of AL buffers queued onto the source at any one time.
    // Larger = more decode headroom but longer pause-tail.  4 is
    // the sweet spot per the asset-system-modernization.md analysis.
    static constexpr int kNumBuffers = 4;

    // Target chunk duration in milliseconds.  Smaller = lower start
    // latency, but smaller chunks risk underruns under load.  100 ms
    // is 2 frames at 60 Hz — comfortable polling margin.
    static constexpr int kChunkMs = 100;

    // AL buffer IDs.  Allocated in WaveOAL::OpenStreaming, freed in
    // CloseStreaming.  Stay valid for the wave's lifetime.
    unsigned int alBuffers[kNumBuffers] = {0};

    // How many of `alBuffers` are currently queued on the AL source.
    // The remainder live in `_freeBuffers` available for the next
    // chunk upload.
    int                       buffersInFlight = 0;
    std::vector<unsigned int> freeBuffers; // unused slots from alBuffers

    // PCM byte size of one chunk, derived from WAVEFORMATEX in
    // OpenStreaming.  Always a multiple of (channels * bytes/sample)
    // so chunks land on sample boundaries.
    int chunkBytes = 0;

    // Total decoded byte offset into the source stream.  Advances
    // as the worker produces chunks; the main thread reads it to
    // compute playback position (combined with AL's per-buffer
    // offset and processed count).
    std::atomic<int64_t> decodeOffsetBytes{0};

    // Cumulative bytes that AL has fully played and the main thread
    // has unqueued.  Used by GetCurrentOffsetSeconds — for queued
    // sources AL_BYTE_OFFSET reports position within the CURRENT
    // buffer only, so we accumulate the size of every unqueued
    // buffer ourselves to get cumulative playback position.  Reset
    // on Stop (when the queue is drained + decode offset rewound).
    std::atomic<int64_t> consumedBytes{0};

    // Producer→consumer queue of decoded chunks awaiting AL upload.
    ChunkQueue queue;

    // Set true while a decode task is in flight.  Prevents kicking
    // multiple workers for the same wave (which would race on
    // _stream->GetData).  CAS to transition; main thread reads,
    // worker clears on completion.
    std::atomic<bool> decodeInFlight{false};

    // True when the worker has produced the last chunk (EOF, non-
    // looping).  Main thread stops kicking decodes; playback ends
    // naturally once AL_BUFFERS_PROCESSED catches up.
    std::atomic<bool> eofReached{false};

    // Worker abort signal.  Set true by the WaveOAL destructor (or
    // any other teardown path) so the in-flight decode task notices
    // and bails before touching potentially-freed wave state.
    std::atomic<bool> aborted{false};
};

} // namespace Audio
} // namespace Poseidon
