#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Audio/Voice/VonNet.hpp>
#include <Poseidon/Audio/Voice/VonTransport.hpp>
#include <cstring>
#include <vector>
#include <thread>
#include <chrono>
#ifndef _WIN32
#include <arpa/inet.h>
#endif
#ifndef _WIN32
#include <netinet/in.h>
#endif
#include <stdint.h>
#ifndef _WIN32
#include <sys/socket.h>
#endif

// --- Packet structure tests ---

using namespace Poseidon;
TEST_CASE("VoNDataPacket layout and size", "[VoN][net]")
{
    REQUIRE(VoNDataPacket::HEADER_SIZE == 21);
    // Verify packed struct
    VoNDataPacket pkt{};
    pkt.init(42, VoNChatChannel::Group, 1000, 320, 40);
    REQUIRE(pkt.magic == VON_MAGIC_DATA);
    REQUIRE(pkt.channel == 42);
    REQUIRE(pkt.chatChan == VoNChatChannel::Group);
    REQUIRE(pkt.origin == 1000);
    REQUIRE(pkt.samples == 320);
    REQUIRE(pkt.size == 40);
    REQUIRE(pkt.isValid());
    REQUIRE(pkt.totalSize() == 21 + 40);
}

TEST_CASE("VoNControlPacket layout and size", "[VoN][net]")
{
    REQUIRE(VoNControlPacket::HEADER_SIZE == 11);
    VoNControlPacket pkt{};
    pkt.init(7, VoNCommand::Create, 8);
    REQUIRE(pkt.magic == VON_MAGIC_CTRL);
    REQUIRE(pkt.channel == 7);
    REQUIRE(pkt.command == VoNCommand::Create);
    REQUIRE(pkt.size == 8);
    REQUIRE(pkt.isValid());
    REQUIRE(pkt.totalSize() == 11 + 8);
}

TEST_CASE("vonIsDataPacket / vonIsControlPacket", "[VoN][net]")
{
    VoNDataPacket dp{};
    dp.init(1, VoNChatChannel::Direct, 0, 320, 40);
    REQUIRE(vonIsDataPacket(&dp, sizeof(dp)));
    REQUIRE_FALSE(vonIsControlPacket(&dp, sizeof(dp)));

    VoNControlPacket cp{};
    cp.init(1, VoNCommand::Remove, 0);
    REQUIRE(vonIsControlPacket(&cp, sizeof(cp)));
    REQUIRE_FALSE(vonIsDataPacket(&cp, sizeof(cp)));

    // Too short
    REQUIRE_FALSE(vonIsDataPacket("ab", 2));
}

TEST_CASE("VoNDataPacket serialize/deserialize round-trip", "[VoN][net]")
{
    // Build packet with payload
    std::vector<uint8_t> buf(VoNDataPacket::HEADER_SIZE + 40);
    auto* pkt = reinterpret_cast<VoNDataPacket*>(buf.data());
    pkt->init(99, VoNChatChannel::Side, 5000, 320, 40);

    // Fill payload with pattern
    for (int i = 0; i < 40; ++i)
        pkt->payload()[i] = static_cast<uint8_t>(i);

    // "Transmit" (memcpy to simulate)
    std::vector<uint8_t> received(buf.size());
    std::memcpy(received.data(), buf.data(), buf.size());

    auto* rpkt = reinterpret_cast<VoNDataPacket*>(received.data());
    REQUIRE(rpkt->isValid());
    REQUIRE(rpkt->channel == 99);
    REQUIRE(rpkt->chatChan == VoNChatChannel::Side);
    REQUIRE(rpkt->origin == 5000);
    REQUIRE(rpkt->samples == 320);
    REQUIRE(rpkt->size == 40);
    REQUIRE(rpkt->payload()[0] == 0);
    REQUIRE(rpkt->payload()[39] == 39);
}

// --- Transport tests ---

TEST_CASE("VoNTransport open/close", "[VoN][net]")
{
    VoNTransport t;
    REQUIRE_FALSE(t.isOpen());
    REQUIRE(t.open(0));
    REQUIRE(t.isOpen());
    REQUIRE(t.localPort() > 0);
    t.close();
    REQUIRE_FALSE(t.isOpen());
}

TEST_CASE("VoNTransport double close safe", "[VoN][net]")
{
    VoNTransport t;
    t.close();
    t.open(0);
    t.close();
    t.close();
    SUCCEED();
}

TEST_CASE("VoNTransport recvFrom returns 0 when empty", "[VoN][net]")
{
    VoNTransport t;
    REQUIRE(t.open(0));
    uint8_t buf[64];
    sockaddr_in from{};
    REQUIRE(t.recvFrom(buf, sizeof(buf), from) == 0);
}

TEST_CASE("VoNTransport localhost send/receive", "[VoN][net]")
{
    VoNTransport sender, receiver;
    REQUIRE(sender.open(0));
    REQUIRE(receiver.open(0));

    auto dest = VoNTransport::makeAddr("127.0.0.1", receiver.localPort());

    // Build a data packet
    std::vector<uint8_t> buf(VoNDataPacket::HEADER_SIZE + 4);
    auto* pkt = reinterpret_cast<VoNDataPacket*>(buf.data());
    pkt->init(1, VoNChatChannel::Global, 0, 320, 4);
    pkt->payload()[0] = 0xAA;
    pkt->payload()[1] = 0xBB;
    pkt->payload()[2] = 0xCC;
    pkt->payload()[3] = 0xDD;

    REQUIRE(sender.sendTo(dest, buf.data(), static_cast<int>(buf.size())));

    // Small delay for loopback delivery
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    uint8_t recvBuf[256];
    sockaddr_in from{};
    int received = receiver.recvFrom(recvBuf, sizeof(recvBuf), from);
    REQUIRE(received == static_cast<int>(buf.size()));

    auto* rpkt = reinterpret_cast<VoNDataPacket*>(recvBuf);
    REQUIRE(rpkt->isValid());
    REQUIRE(rpkt->channel == 1);
    REQUIRE(rpkt->chatChan == VoNChatChannel::Global);
    REQUIRE(rpkt->payload()[0] == 0xAA);
    REQUIRE(rpkt->payload()[3] == 0xDD);
}

TEST_CASE("VoNTransport control packet send/receive", "[VoN][net]")
{
    VoNTransport a, b;
    REQUIRE(a.open(0));
    REQUIRE(b.open(0));

    auto dest = VoNTransport::makeAddr("127.0.0.1", b.localPort());

    // Send a Create control packet with codec info payload
    std::vector<uint8_t> buf(VoNControlPacket::HEADER_SIZE + 8);
    auto* pkt = reinterpret_cast<VoNControlPacket*>(buf.data());
    pkt->init(5, VoNCommand::Create, 8);
    // Fill codec info
    int sr = 16000, br = 16000;
    std::memcpy(pkt->payload(), &sr, 4);
    std::memcpy(pkt->payload() + 4, &br, 4);

    REQUIRE(a.sendTo(dest, buf.data(), static_cast<int>(buf.size())));

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    uint8_t recvBuf[256];
    sockaddr_in from{};
    int received = b.recvFrom(recvBuf, sizeof(recvBuf), from);
    REQUIRE(received == static_cast<int>(buf.size()));

    REQUIRE(vonIsControlPacket(recvBuf, received));
    auto* rpkt = reinterpret_cast<VoNControlPacket*>(recvBuf);
    REQUIRE(rpkt->command == VoNCommand::Create);
    REQUIRE(rpkt->channel == 5);
    REQUIRE(rpkt->size == 8);

    int rsr = 0;
    std::memcpy(&rsr, rpkt->payload(), 4);
    REQUIRE(rsr == 16000);
}

TEST_CASE("VoNTransport makeAddr", "[VoN][net]")
{
    auto addr = VoNTransport::makeAddr("127.0.0.1", 12345);
    REQUIRE(addr.sin_family == AF_INET);
    REQUIRE(ntohs(addr.sin_port) == 12345);
}
