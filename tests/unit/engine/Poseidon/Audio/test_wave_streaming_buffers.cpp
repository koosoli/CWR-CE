// Unit tests for the Phase 3g streaming infrastructure data
// structures.  These are pure-C++ value types and a thread-safe
// queue — testable without any audio backend, AL context, or
// asset fixture.
//
// Pins:
//   - DecodedChunk holds PCM bytes + stream offset + last-chunk flag
//   - ChunkQueue is FIFO, thread-safe between one producer + one
//     consumer
//   - StreamingBuffers initializes to a well-defined empty state
//   - StreamingBuffers' atomics are usable across threads

#include <catch2/catch_test_macros.hpp>
#include <PoseidonOpenAL/WaveStreamingBuffers.hpp>

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>
#include <stdint.h>
#include <utility>

using namespace Poseidon;
using Poseidon::Audio::ChunkQueue;
using Poseidon::Audio::DecodedChunk;
using Poseidon::Audio::StreamingBuffers;

TEST_CASE("DecodedChunk defaults to empty + zero offset", "[wave_streaming]")
{
    DecodedChunk c;
    REQUIRE(c.pcm.empty());
    REQUIRE(c.streamOffset == 0);
    REQUIRE_FALSE(c.isLastChunk);
}

TEST_CASE("ChunkQueue Push + TryPop in FIFO order", "[wave_streaming][chunk_queue]")
{
    ChunkQueue q;
    REQUIRE(q.Size() == 0);

    DecodedChunk a;
    a.streamOffset = 0;
    a.pcm.push_back(1);
    DecodedChunk b;
    b.streamOffset = 4400;
    b.pcm.push_back(2);
    DecodedChunk c;
    c.streamOffset = 8800;
    c.pcm.push_back(3);

    q.Push(std::move(a));
    q.Push(std::move(b));
    q.Push(std::move(c));
    REQUIRE(q.Size() == 3);

    DecodedChunk out;
    REQUIRE(q.TryPop(out));
    REQUIRE(out.streamOffset == 0);
    REQUIRE(out.pcm.size() == 1);
    REQUIRE(out.pcm[0] == 1);

    REQUIRE(q.TryPop(out));
    REQUIRE(out.streamOffset == 4400);

    REQUIRE(q.TryPop(out));
    REQUIRE(out.streamOffset == 8800);

    REQUIRE_FALSE(q.TryPop(out));
    REQUIRE(q.Size() == 0);
}

TEST_CASE("ChunkQueue TryPop on empty returns false", "[wave_streaming][chunk_queue]")
{
    ChunkQueue q;
    DecodedChunk out;
    REQUIRE_FALSE(q.TryPop(out));
}

TEST_CASE("ChunkQueue Clear empties the queue", "[wave_streaming][chunk_queue]")
{
    ChunkQueue q;
    DecodedChunk a, b;
    q.Push(std::move(a));
    q.Push(std::move(b));
    REQUIRE(q.Size() == 2);
    q.Clear();
    REQUIRE(q.Size() == 0);
}

TEST_CASE("ChunkQueue is safe across one-producer one-consumer threads", "[wave_streaming][chunk_queue][threading]")
{
    // Stress test the producer/consumer pattern that the streaming
    // decode worker (Phase 3g-2) will exercise: TaskPool worker
    // pushes chunks, main thread pops in Commit.  500 chunks pushed
    // by a worker thread, consumer pulls until it sees all of them.
    ChunkQueue q;
    constexpr int kChunks = 500;
    std::atomic<bool> producerDone{false};

    std::thread producer(
        [&]()
        {
            for (int i = 0; i < kChunks; ++i)
            {
                DecodedChunk c;
                c.streamOffset = i * 4400;
                c.pcm.assign(8, static_cast<uint8_t>(i & 0xFF));
                q.Push(std::move(c));
                // Yield occasionally so the consumer thread runs.
                if ((i & 15) == 0)
                {
                    std::this_thread::yield();
                }
            }
            producerDone.store(true, std::memory_order_release);
        });

    int consumed = 0;
    int64_t expectedOffset = 0;
    while (consumed < kChunks)
    {
        DecodedChunk out;
        if (q.TryPop(out))
        {
            REQUIRE(out.streamOffset == expectedOffset);
            REQUIRE(out.pcm.size() == 8);
            REQUIRE(out.pcm[0] == static_cast<uint8_t>(consumed & 0xFF));
            ++consumed;
            expectedOffset += 4400;
        }
        else if (producerDone.load(std::memory_order_acquire))
        {
            // Producer finished but queue still has trailing chunks.
            // Loop continues to drain.
        }
    }

    producer.join();
    REQUIRE(consumed == kChunks);
    REQUIRE(q.Size() == 0);
}

TEST_CASE("StreamingBuffers default-construct to all-zero AL buffers", "[wave_streaming]")
{
    StreamingBuffers s;
    for (int i = 0; i < StreamingBuffers::kNumBuffers; ++i)
    {
        REQUIRE(s.alBuffers[i] == 0);
    }
    REQUIRE(s.buffersInFlight == 0);
    REQUIRE(s.chunkBytes == 0);
    REQUIRE(s.decodeOffsetBytes.load() == 0);
    REQUIRE_FALSE(s.decodeInFlight.load());
    REQUIRE_FALSE(s.eofReached.load());
    REQUIRE_FALSE(s.aborted.load());
    REQUIRE(s.queue.Size() == 0);
    REQUIRE(s.freeBuffers.empty());
}

TEST_CASE("StreamingBuffers constants match the design", "[wave_streaming][config]")
{
    // 100 ms chunks, 4-deep ring = 400 ms queued ahead.  See
    static_assert(StreamingBuffers::kNumBuffers == 4);
    static_assert(StreamingBuffers::kChunkMs == 100);
}

TEST_CASE("StreamingBuffers atomic abort flag flips safely across threads", "[wave_streaming][threading]")
{
    // The destructor sets `aborted = true` and the worker sees it
    // via memory_order_acquire on its next loop iteration.  This
    // test simulates that handoff.
    StreamingBuffers s;
    std::atomic<bool> workerSawAbort{false};

    std::thread worker(
        [&]()
        {
            while (!s.aborted.load(std::memory_order_acquire))
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            workerSawAbort.store(true, std::memory_order_release);
        });

    // Let the worker spin for a beat, then signal abort.
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    s.aborted.store(true, std::memory_order_release);

    worker.join();
    REQUIRE(workerSawAbort.load());
}
