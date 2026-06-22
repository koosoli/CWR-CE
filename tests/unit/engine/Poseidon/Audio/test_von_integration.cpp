#include <catch2/catch_all.hpp>
#include <Poseidon/Audio/Voice/VonApp.hpp>
#include <Poseidon/Audio/Voice/VonTransport.hpp>
#include <Poseidon/Audio/Voice/VonNet.hpp>
#include <Poseidon/Audio/Voice/OpusCodec.hpp>
#include <Poseidon/Audio/Voice/PCMCodec.hpp>
#include <cstring>
#include <thread>
#include <chrono>
#ifndef _WIN32
#include <netinet/in.h>
#endif
#include <stdint.h>
#include <catch2/catch_test_macros.hpp>
#include <format>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

using namespace Poseidon;
static constexpr int FRAME = 320;
static constexpr int RATE = 16000;

static void fillTone(int16_t* buf, int samples, int16_t amp, int period = 40)
{
    for (int i = 0; i < samples; ++i)
        buf[i] = static_cast<int16_t>((i % period < period / 2) ? amp : -amp);
}

static int64_t energy(const int16_t* buf, int n)
{
    int64_t e = 0;
    for (int i = 0; i < n; ++i)
        e += static_cast<int64_t>(buf[i]) * buf[i];
    return e;
}

// Full pipeline: client → encode → transport → server relay → client → decode
TEST_CASE("VoN full pipeline with localhost UDP", "[VoN][integration]")
{
    // Set up sender client
    VoNSystem senderSys;
    senderSys.initClient();
    auto* sender = senderSys.client();
    sender->setCodecFactory([]() { return std::make_unique<PCMCodec>(RATE); });
    sender->setSenderId(1);
    sender->setChatChannel(VoNChatChannel::Group);

    // Set up receiver client
    VoNSystem receiverSys;
    receiverSys.initClient();
    auto* receiver = receiverSys.client();
    receiver->setCodecFactory([]() { return std::make_unique<PCMCodec>(RATE); });
    receiver->createChannel(1, {}); // expect voice from player 1

    // Set up server with routing: player 1 → player 2
    VoNSystem serverSys;
    serverSys.initServer();
    auto* server = serverSys.server();

    // Collect packets from sender
    std::vector<std::vector<uint8_t>> packets;
    sender->setPacketSink([&](const std::vector<uint8_t>& pkt) { packets.push_back(pkt); });

    // Simulate 5 frames of voice
    int16_t tone[FRAME];
    fillTone(tone, FRAME, 7000);

    PCMCodec encCodec(RATE);
    for (int i = 0; i < 5; ++i)
    {
        std::vector<uint8_t> pkt(VoNDataPacket::HEADER_SIZE + FRAME * 2);
        auto* dp = reinterpret_cast<VoNDataPacket*>(pkt.data());
        dp->init(1, VoNChatChannel::Group, i * FRAME, FRAME, FRAME * 2);
        std::memcpy(dp->payload(), tone, FRAME * 2);
        packets.push_back(pkt);
    }

    // Server routes packets to receiver
    int forwarded = 0;
    server->setRouting(1, VoNChatChannel::Group, {2});
    server->setForwarder(
        [&](uint32_t target, const void* data, int size)
        {
            REQUIRE(target == 2);
            ++forwarded;
            // Deliver to receiver
            if (size >= VoNDataPacket::HEADER_SIZE)
            {
                VoNDataPacket pkt;
                std::memcpy(&pkt, data, VoNDataPacket::HEADER_SIZE);
                receiver->onDataPacket(pkt, static_cast<const uint8_t*>(data) + VoNDataPacket::HEADER_SIZE);
            }
        });

    for (auto& pkt : packets)
        server->onDataPacket(pkt.data(), static_cast<int>(pkt.size()));

    REQUIRE(forwarded == 5);

    // Pull decoded audio from receiver
    for (int i = 0; i < 5; ++i)
    {
        int16_t out[FRAME] = {};
        int n = receiver->pullSpeaker(1, out, FRAME);
        REQUIRE(n == FRAME);
        REQUIRE(out[0] == tone[0]);
    }

    senderSys.shutdown();
    receiverSys.shutdown();
    serverSys.shutdown();
}

// Opus round-trip through full pipeline
TEST_CASE("VoN Opus pipeline integration", "[VoN][integration]")
{
    VoNSystem sys;
    sys.initClient();
    sys.initServer();

    auto* client = sys.client();
    client->setCodecFactory([]() { return std::make_unique<OpusCodec>(); });
    client->setSenderId(10);
    client->createChannel(20, {}); // receive from "player 20"

    // Route player 20 → player 10
    sys.server()->setRouting(20, VoNChatChannel::Direct, {10});
    sys.server()->setForwarder(
        [&](uint32_t target, const void* data, int size)
        {
            VoNDataPacket pkt;
            std::memcpy(&pkt, data, VoNDataPacket::HEADER_SIZE);
            client->onDataPacket(pkt, static_cast<const uint8_t*>(data) + VoNDataPacket::HEADER_SIZE);
        });

    // Encode and route 3 frames as if from player 20
    int16_t tone[FRAME];
    fillTone(tone, FRAME, 6000, 32);

    OpusCodec enc;
    std::vector<uint8_t> encBuf(enc.maxEncodedSize());
    for (int i = 0; i < 3; ++i)
    {
        int bytes = enc.encode(tone, FRAME, encBuf.data(), static_cast<int>(encBuf.size()));
        REQUIRE(bytes > 0);

        std::vector<uint8_t> raw(VoNDataPacket::HEADER_SIZE + bytes);
        auto* pkt = reinterpret_cast<VoNDataPacket*>(raw.data());
        pkt->init(20, VoNChatChannel::Direct, i * FRAME, FRAME, static_cast<uint16_t>(bytes));
        std::memcpy(pkt->payload(), encBuf.data(), bytes);

        sys.server()->onDataPacket(raw.data(), static_cast<int>(raw.size()));
    }

    // Pull and verify non-silent output
    for (int i = 0; i < 3; ++i)
    {
        int16_t out[FRAME] = {};
        int n = client->pullSpeaker(20, out, FRAME);
        REQUIRE(n == FRAME);
        REQUIRE(energy(out, FRAME) > 0);
    }

    sys.shutdown();
}

// Test UDP transport send/receive with VoN packets
TEST_CASE("VoN UDP transport round-trip", "[VoN][integration]")
{
    VoNTransport tx, rx;
    REQUIRE(tx.open(0));
    REQUIRE(rx.open(0));

    auto rxAddr = VoNTransport::makeAddr("127.0.0.1", rx.localPort());

    // Build and send a voice data packet
    int16_t tone[FRAME];
    fillTone(tone, FRAME, 5000);

    std::vector<uint8_t> raw(VoNDataPacket::HEADER_SIZE + FRAME * 2);
    auto* pkt = reinterpret_cast<VoNDataPacket*>(raw.data());
    pkt->init(42, VoNChatChannel::Vehicle, 0, FRAME, FRAME * 2);
    std::memcpy(pkt->payload(), tone, FRAME * 2);

    REQUIRE(tx.sendTo(rxAddr, raw.data(), static_cast<int>(raw.size())));

    // Small delay for loopback delivery
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // Receive
    std::vector<uint8_t> buf(2048);
    sockaddr_in from{};
    int got = rx.recvFrom(buf.data(), static_cast<int>(buf.size()), from);
    REQUIRE(got == static_cast<int>(raw.size()));

    // Parse received packet
    REQUIRE(vonIsDataPacket(buf.data(), got));
    VoNDataPacket recv{};
    std::memcpy(&recv, buf.data(), VoNDataPacket::HEADER_SIZE);
    REQUIRE(recv.channel == 42);
    REQUIRE(recv.chatChan == VoNChatChannel::Vehicle);
    REQUIRE(recv.origin == 0);
    REQUIRE(recv.samples == FRAME);

    tx.close();
    rx.close();
}

// Multi-channel routing: two senders, each with different targets
TEST_CASE("VoN multi-channel server routing", "[VoN][integration]")
{
    VoNServer server;

    std::unordered_map<uint32_t, int> fwdCount;
    server.setForwarder([&](uint32_t target, const void*, int) { ++fwdCount[target]; });

    // Player 1 talks to players 3, 4
    // Player 2 talks to player 3 only
    server.setRouting(1, VoNChatChannel::Group, {3, 4});
    server.setRouting(2, VoNChatChannel::Side, {3});

    // 2 packets from player 1
    for (int i = 0; i < 2; ++i)
    {
        std::vector<uint8_t> raw(VoNDataPacket::HEADER_SIZE + 4);
        auto* pkt = reinterpret_cast<VoNDataPacket*>(raw.data());
        pkt->init(1, VoNChatChannel::Group, i * FRAME, FRAME, 4);
        server.onDataPacket(raw.data(), static_cast<int>(raw.size()));
    }

    // 1 packet from player 2
    std::vector<uint8_t> raw(VoNDataPacket::HEADER_SIZE + 4);
    auto* pkt = reinterpret_cast<VoNDataPacket*>(raw.data());
    pkt->init(2, VoNChatChannel::Side, 0, FRAME, 4);
    server.onDataPacket(raw.data(), static_cast<int>(raw.size()));

    REQUIRE(fwdCount[3] == 3);                   // 2 from player1 + 1 from player2
    REQUIRE(fwdCount[4] == 2);                   // 2 from player1
    REQUIRE(fwdCount.find(1) == fwdCount.end()); // player1 never receives own voice
    REQUIRE(fwdCount.find(2) == fwdCount.end()); // player2 never receives own voice
}

// Routing race condition: voice packets arrive before setRouting()
TEST_CASE("VoN server buffers packets before routing", "[VoN][integration]")
{
    VoNServer server;

    std::unordered_map<uint32_t, int> fwdCount;
    server.setForwarder([&](uint32_t target, const void*, int) { ++fwdCount[target]; });

    // Send 5 voice packets from player 1 BEFORE routing is established
    for (int i = 0; i < 5; ++i)
    {
        std::vector<uint8_t> raw(VoNDataPacket::HEADER_SIZE + 4);
        auto* pkt = reinterpret_cast<VoNDataPacket*>(raw.data());
        pkt->init(1, VoNChatChannel::Direct, i * FRAME, FRAME, 4);
        server.onDataPacket(raw.data(), static_cast<int>(raw.size()));
    }

    // Nothing forwarded yet — no route
    REQUIRE(fwdCount.empty());

    // Now routing arrives (the guaranteed message caught up)
    server.setRouting(1, VoNChatChannel::Direct, {2, 3});

    // All 5 buffered packets should have been flushed to targets
    REQUIRE(fwdCount[2] == 5);
    REQUIRE(fwdCount[3] == 5);
    REQUIRE(fwdCount.find(1) == fwdCount.end()); // sender excluded

    // Subsequent packets route immediately
    fwdCount.clear();
    std::vector<uint8_t> raw(VoNDataPacket::HEADER_SIZE + 4);
    auto* pkt = reinterpret_cast<VoNDataPacket*>(raw.data());
    pkt->init(1, VoNChatChannel::Direct, 5 * FRAME, FRAME, 4);
    server.onDataPacket(raw.data(), static_cast<int>(raw.size()));

    REQUIRE(fwdCount[2] == 1);
    REQUIRE(fwdCount[3] == 1);
}

// Repeated transmissions: route cleared between transmissions must re-buffer
TEST_CASE("VoN server handles repeated transmissions across route clear", "[VoN][integration]")
{
    VoNServer server;

    std::unordered_map<uint32_t, int> fwdCount;
    server.setForwarder([&](uint32_t target, const void*, int) { ++fwdCount[target]; });

    // First transmission: set route, send packets
    server.setRouting(1, VoNChatChannel::Direct, {2, 3});
    for (int i = 0; i < 5; ++i)
    {
        std::vector<uint8_t> raw(VoNDataPacket::HEADER_SIZE + 4);
        auto* pkt = reinterpret_cast<VoNDataPacket*>(raw.data());
        pkt->init(1, VoNChatChannel::Direct, i * FRAME, FRAME, 4);
        server.onDataPacket(raw.data(), static_cast<int>(raw.size()));
    }
    REQUIRE(fwdCount[2] == 5);
    REQUIRE(fwdCount[3] == 5);

    // Transmission ends: SetVoiceChannel(CCNone) clears the route
    fwdCount.clear();
    server.setRouting(1, VoNChatChannel::Direct, {});

    // Second transmission starts: voice packets arrive BEFORE new routing
    // (race condition: UDP voice faster than reliable SetVoiceChannel)
    for (int i = 0; i < 3; ++i)
    {
        std::vector<uint8_t> raw(VoNDataPacket::HEADER_SIZE + 4);
        auto* pkt = reinterpret_cast<VoNDataPacket*>(raw.data());
        pkt->init(1, VoNChatChannel::Direct, i * FRAME, FRAME, 4);
        server.onDataPacket(raw.data(), static_cast<int>(raw.size()));
    }

    // Should be buffered (route was erased), not silently dropped
    REQUIRE(fwdCount.empty());

    // New routing arrives
    server.setRouting(1, VoNChatChannel::Direct, {2, 3});

    // Buffered packets flushed
    REQUIRE(fwdCount[2] == 3);
    REQUIRE(fwdCount[3] == 3);

    // Further packets route immediately
    fwdCount.clear();
    std::vector<uint8_t> raw(VoNDataPacket::HEADER_SIZE + 4);
    auto* pkt = reinterpret_cast<VoNDataPacket*>(raw.data());
    pkt->init(1, VoNChatChannel::Direct, 3 * FRAME, FRAME, 4);
    server.onDataPacket(raw.data(), static_cast<int>(raw.size()));
    REQUIRE(fwdCount[2] == 1);
    REQUIRE(fwdCount[3] == 1);
}
TEST_CASE("VoN system lifecycle", "[VoN][integration]")
{
    VoNSystem sys;

    // First init
    sys.initClient();
    sys.initServer();
    REQUIRE(sys.hasClient());
    REQUIRE(sys.hasServer());

    auto* c = sys.client();
    c->setCodecFactory([]() { return std::make_unique<PCMCodec>(RATE); });
    c->createChannel(5, {});
    REQUIRE(c->hasChannel(5));

    // Shutdown
    sys.shutdown();
    REQUIRE_FALSE(sys.hasClient());
    REQUIRE_FALSE(sys.hasServer());

    // Re-init
    sys.initClient();
    REQUIRE(sys.hasClient());
    REQUIRE_FALSE(sys.client()->hasChannel(5)); // fresh client
    sys.shutdown();
}

// System lifecycle: init, use, shutdown, re-init

// Speaker pipeline regression tests

// Helper: simulate a complete transmission (encode N frames, deliver to receiver)
static void simulateTransmission(VoNClient* receiver, uint32_t senderChannel, int numFrames, int16_t amplitude,
                                 int period = 40)
{
    PCMCodec codec(RATE);
    int16_t tone[FRAME];
    fillTone(tone, FRAME, amplitude, period);

    for (int i = 0; i < numFrames; ++i)
    {
        std::vector<uint8_t> raw(VoNDataPacket::HEADER_SIZE + FRAME * 2);
        auto* pkt = reinterpret_cast<VoNDataPacket*>(raw.data());
        pkt->init(senderChannel, VoNChatChannel::Direct, i * FRAME, FRAME, FRAME * 2);
        std::memcpy(pkt->payload(), tone, FRAME * 2);
        receiver->onDataPacket(*pkt, raw.data() + VoNDataPacket::HEADER_SIZE);
    }
}

// Verify all frames from a transmission can be pulled without data loss
TEST_CASE("VoN speaker: all frames pulled without loss", "[VoN][speaker]")
{
    VoNSystem sys;
    sys.initClient();
    auto* client = sys.client();
    client->setCodecFactory([]() { return std::make_unique<PCMCodec>(RATE); });
    client->createChannel(100, {});

    // Push and pull in interleaved batches (jitter buffer capacity = 8)
    int16_t tone[FRAME];
    fillTone(tone, FRAME, 7000);
    int16_t out[FRAME];
    int totalPushed = 0, totalPulled = 0;

    for (int batch = 0; batch < 5; ++batch)
    {
        // Push 4 frames
        for (int i = 0; i < 4; ++i)
        {
            std::vector<uint8_t> raw(VoNDataPacket::HEADER_SIZE + FRAME * 2);
            auto* pkt = reinterpret_cast<VoNDataPacket*>(raw.data());
            pkt->init(100, VoNChatChannel::Direct, totalPushed * FRAME, FRAME, FRAME * 2);
            std::memcpy(pkt->payload(), tone, FRAME * 2);
            client->onDataPacket(*pkt, raw.data() + VoNDataPacket::HEADER_SIZE);
            ++totalPushed;
        }
        // Pull available
        while (client->pullSpeaker(100, out, FRAME) == FRAME)
        {
            REQUIRE(energy(out, FRAME) > 0);
            ++totalPulled;
        }
    }
    // Final drain
    while (client->pullSpeaker(100, out, FRAME) == FRAME)
    {
        REQUIRE(energy(out, FRAME) > 0);
        ++totalPulled;
    }
    REQUIRE(totalPulled == 20);
    sys.shutdown();
}

// Verify transmission boundary: after pull drains, no stale data remains
TEST_CASE("VoN speaker: clean end after drain", "[VoN][speaker]")
{
    VoNSystem sys;
    sys.initClient();
    auto* client = sys.client();
    client->setCodecFactory([]() { return std::make_unique<PCMCodec>(RATE); });
    client->createChannel(200, {});

    // Send 5 frames
    simulateTransmission(client, 200, 5, 6000);

    // Pull all 5
    for (int i = 0; i < 5; ++i)
    {
        int16_t out[FRAME];
        REQUIRE(client->pullSpeaker(200, out, FRAME) == FRAME);
    }

    // Next pull must return 0 — no stale data
    int16_t out[FRAME] = {};
    REQUIRE(client->pullSpeaker(200, out, FRAME) == 0);

    // Verify output buffer untouched (still zeros)
    REQUIRE(energy(out, FRAME) == 0);

    sys.shutdown();
}

// Verify two consecutive transmissions don't interfere
TEST_CASE("VoN speaker: second transmission after first drains", "[VoN][speaker]")
{
    VoNSystem sys;
    sys.initClient();
    auto* client = sys.client();
    client->setCodecFactory([]() { return std::make_unique<PCMCodec>(RATE); });
    client->createChannel(300, {});

    // First transmission: 5 frames at amplitude 7000
    simulateTransmission(client, 300, 5, 7000, 40);

    // Pull all
    int pulled1 = 0;
    int16_t out[FRAME];
    while (client->pullSpeaker(300, out, FRAME) == FRAME)
        ++pulled1;
    REQUIRE(pulled1 == 5);

    // Confirm drained
    REQUIRE(client->pullSpeaker(300, out, FRAME) == 0);

    // Second transmission: 3 frames starting from origin=0 (new transmission)
    // Uses different amplitude to distinguish
    {
        int16_t tone2[FRAME];
        fillTone(tone2, FRAME, 3000, 20);
        for (int i = 0; i < 3; ++i)
        {
            std::vector<uint8_t> raw(VoNDataPacket::HEADER_SIZE + FRAME * 2);
            auto* pkt = reinterpret_cast<VoNDataPacket*>(raw.data());
            pkt->init(300, VoNChatChannel::Direct, i * FRAME, FRAME, FRAME * 2);
            std::memcpy(pkt->payload(), tone2, FRAME * 2);
            client->onDataPacket(*pkt, raw.data() + VoNDataPacket::HEADER_SIZE);
        }
    }

    // Pull all 3 — must get exactly 3
    int pulled2 = 0;
    while (client->pullSpeaker(300, out, FRAME) == FRAME)
        ++pulled2;
    REQUIRE(pulled2 == 3);

    sys.shutdown();
}

// Verify auto-created replayer works for unknown channel
TEST_CASE("VoN speaker: auto-create replayer on first packet", "[VoN][speaker]")
{
    VoNSystem sys;
    sys.initClient();
    auto* client = sys.client();
    client->setCodecFactory([]() { return std::make_unique<PCMCodec>(RATE); });
    // Note: NOT calling createChannel() — replayer should be auto-created

    int16_t tone[FRAME];
    fillTone(tone, FRAME, 5000);

    // Send packet for channel 999 (not pre-created)
    std::vector<uint8_t> raw(VoNDataPacket::HEADER_SIZE + FRAME * 2);
    auto* pkt = reinterpret_cast<VoNDataPacket*>(raw.data());
    pkt->init(999, VoNChatChannel::Direct, 0, FRAME, FRAME * 2);
    std::memcpy(pkt->payload(), tone, FRAME * 2);
    client->onDataPacket(*pkt, raw.data() + VoNDataPacket::HEADER_SIZE);

    // Should be pullable
    int16_t out[FRAME] = {};
    REQUIRE(client->pullSpeaker(999, out, FRAME) == FRAME);
    REQUIRE(energy(out, FRAME) > 0);

    sys.shutdown();
}

// Verify Opus encode→decode round-trip preserves audio energy across transmission boundary
TEST_CASE("VoN speaker: Opus multi-transmission", "[VoN][speaker]")
{
    VoNSystem sys;
    sys.initClient();
    auto* client = sys.client();
    client->setCodecFactory([]() { return std::make_unique<OpusCodec>(); });
    client->createChannel(500, {});

    OpusCodec enc;
    std::vector<uint8_t> encBuf(enc.maxEncodedSize());
    int16_t tone[FRAME];
    fillTone(tone, FRAME, 8000, 32);

    // Transmission 1: 10 frames, pushed/pulled in batches
    int pulled = 0;
    int16_t out[FRAME];
    for (int batch = 0; batch < 2; ++batch)
    {
        for (int i = 0; i < 5; ++i)
        {
            int idx = batch * 5 + i;
            int bytes = enc.encode(tone, FRAME, encBuf.data(), static_cast<int>(encBuf.size()));
            REQUIRE(bytes > 0);
            std::vector<uint8_t> raw(VoNDataPacket::HEADER_SIZE + bytes);
            auto* pkt = reinterpret_cast<VoNDataPacket*>(raw.data());
            pkt->init(500, VoNChatChannel::Direct, idx * FRAME, FRAME, static_cast<uint16_t>(bytes));
            std::memcpy(pkt->payload(), encBuf.data(), bytes);
            client->onDataPacket(*pkt, raw.data() + VoNDataPacket::HEADER_SIZE);
        }
        while (client->pullSpeaker(500, out, FRAME) == FRAME)
        {
            REQUIRE(energy(out, FRAME) > 0);
            ++pulled;
        }
    }
    // Final drain
    while (client->pullSpeaker(500, out, FRAME) == FRAME)
    {
        REQUIRE(energy(out, FRAME) > 0);
        ++pulled;
    }
    REQUIRE(pulled == 10);

    // Transmission 2: 5 frames starting at origin=0
    for (int i = 0; i < 5; ++i)
    {
        int bytes = enc.encode(tone, FRAME, encBuf.data(), static_cast<int>(encBuf.size()));
        REQUIRE(bytes > 0);
        std::vector<uint8_t> raw(VoNDataPacket::HEADER_SIZE + bytes);
        auto* pkt = reinterpret_cast<VoNDataPacket*>(raw.data());
        pkt->init(500, VoNChatChannel::Direct, i * FRAME, FRAME, static_cast<uint16_t>(bytes));
        std::memcpy(pkt->payload(), encBuf.data(), bytes);
        client->onDataPacket(*pkt, raw.data() + VoNDataPacket::HEADER_SIZE);
    }

    pulled = 0;
    while (client->pullSpeaker(500, out, FRAME) == FRAME)
    {
        REQUIRE(energy(out, FRAME) > 0);
        ++pulled;
    }
    REQUIRE(pulled == 5);

    // Confirm fully drained
    REQUIRE(client->pullSpeaker(500, out, FRAME) == 0);

    sys.shutdown();
}
