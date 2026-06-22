#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <Poseidon/Foundation/platform.hpp>
#include <Poseidon/Foundation/Common/Global.hpp>
#ifndef _WIN32
#include <sys/socket.h>
#endif

#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#endif

#include <Poseidon/Foundation/Common/NetGlobal.hpp>

using Poseidon::Foundation::MSG_SERIAL_NULL;
using Poseidon::Foundation::nsCancel;
using Poseidon::Foundation::nsError;
using Poseidon::Foundation::nsInputAck;
using Poseidon::Foundation::nsInputPending;
using Poseidon::Foundation::nsInvalidMessage;
using Poseidon::Foundation::nsNoMoreCallbacks;
using Poseidon::Foundation::nsOK;
using Poseidon::Foundation::nsOutputAck;
using Poseidon::Foundation::nsOutputPending;

TEST_CASE("Netglobal message header layout stays stable", "[network][netglobal][layout]")
{
    REQUIRE(MSG_HEADER_LEN == sizeof(MsgHeader));
    REQUIRE(sizeof(MsgHeader) == 24);
    REQUIRE(sizeof(MsgSerial) == sizeof(unsigned32));
    REQUIRE(MSG_SERIAL_NULL == 0);
}

TEST_CASE("Netglobal packet flags keep documented masks", "[network][netglobal][flags]")
{
    REQUIRE(MSG_VIM_FLAG == 0x8000);
    REQUIRE(MSG_URGENT_FLAG == 0x4000);
    REQUIRE(MSG_ORDERED_FLAG == 0x2000);
    REQUIRE(MSG_USER_FLAGS == 0x000f);
    REQUIRE(MSG_ALL_FLAGS == 0xffff);

    REQUIRE(SHORT_ACK(MSG_ORDERED_FLAG));
    REQUIRE(SHORT_ACK(MSG_DELAY_FLAG));
    REQUIRE(SHORT_ACK(MSG_BANDWIDTH_FLAG));
    REQUIRE_FALSE(SHORT_ACK(MSG_VIM_FLAG));
}

TEST_CASE("Netglobal socket address helpers decode IPv4 addresses", "[network][netglobal][address]")
{
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(2302);
    addr.sin_addr.s_addr = htonl(0x01020304u);

    REQUIRE(PORT(addr) == 2302);
    REQUIRE(ADDR(addr) == 0x01020304u);
    // Both platforms expose IP1..IP4 as the bytes of the network-
    // order address — after `htonl(0x01020304)` the host-side
    // word is 0x04030201 (little-endian), so IP1 (high host byte)
    // = 4, IP4 (low host byte) = 1.  The Windows S_un_b view is
    // wired with the same semantics (`s_b4 = byte 3 = 4`, etc.).
    REQUIRE(IP1(addr) == 4);
    REQUIRE(IP2(addr) == 3);
    REQUIRE(IP3(addr) == 2);
    REQUIRE(IP4(addr) == 1);
}

TEST_CASE("Netglobal status values stay ordered by lifecycle", "[network][netglobal][status]")
{
    REQUIRE(nsError < nsOK);
    REQUIRE(nsInvalidMessage < nsInputPending);
    REQUIRE(nsInputPending < nsInputAck);
    REQUIRE(nsOutputPending < nsOutputAck);
    REQUIRE(nsCancel < nsNoMoreCallbacks);
}
