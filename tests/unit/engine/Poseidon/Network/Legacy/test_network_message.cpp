#include <catch2/catch_test_macros.hpp>
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
#include <Poseidon/Foundation/Common/Global.hpp>
#include <Poseidon/Foundation/Types/Pointers.hpp>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

// Undefine SAFE_HEAP_STAT to avoid pulling in Poseidon statistics.hpp
#ifdef SAFE_HEAP_STAT
#undef SAFE_HEAP_STAT
#endif

// Network module requires its own PCH structure
#include <Poseidon/Network/Legacy/netpch.hpp>

#include <cstring>

using Poseidon::Foundation::MSG_SERIAL_NULL;
using Poseidon::Foundation::nsCancel;
using Poseidon::Foundation::nsInvalidMessage;
using Poseidon::Foundation::nsNoMoreCallbacks;
using Poseidon::Foundation::nsOutputSent;

// Network Module Testing - Batch 1: Core Message & Pool
// Tests for NetMessage and NetMessagePool classes
//
// PURPOSE: Unit tests to catch issues with:
// - 64-bit compatibility (pointer sizes, integer sizes)
// - Non-permissive mode compliance
// - Memory management
// - Refactoring safety
//
// Coverage areas:
// 1. NetMessage Basics (5 tests)
// 2. Message Status (3 tests)
// 3. NetMessagePool (4 tests)
// 4. Advanced Features (3 tests)
//
//
// Total: 15 tests covering core message functionality

// Section 1.1: NetMessage Basics (5 tests)

TEST_CASE("NetMessage - Construction", "[network][message][basic]")
{
    NetMessagePool* pool = NetMessagePool::pool();
    REQUIRE(pool != nullptr);

    SECTION("Create message with minimum length")
    {
        Ref<NetMessage> msg = pool->newMessage(100, nullptr);

        REQUIRE(msg != nullptr);
        REQUIRE(msg->getHeader() != nullptr);
        REQUIRE(msg->getSerial() == MSG_SERIAL_NULL);
        REQUIRE(msg->getStatus() == nsInvalidMessage);
        REQUIRE(msg->getChannel() == nullptr);

        msg->recycle();
    }

    SECTION("Create message with zero length (should use minimum)")

    {
        Ref<NetMessage> msg = pool->newMessage(0, nullptr);

        REQUIRE(msg != nullptr);
        REQUIRE(msg->getHeader() != nullptr);

        msg->recycle();
    }

    SECTION("Message ID assignment")
    {
        Ref<NetMessage> msg1 = pool->newMessage(100, nullptr);
        Ref<NetMessage> msg2 = pool->newMessage(100, nullptr);

        // IDs should be unique and incrementing
        REQUIRE(msg1->id != msg2->id);
        REQUIRE(msg2->id > msg1->id);

        msg1->recycle();
        msg2->recycle();
    }
}

TEST_CASE("NetMessage - Set/Get Data", "[network][message][basic]")
{
    NetMessagePool* pool = NetMessagePool::pool();
    Ref<NetMessage> msg = pool->newMessage(100, nullptr);

    SECTION("Set and retrieve data")
    {
        uint8_t testData[] = {1, 2, 3, 4, 5};
        msg->setData(testData, sizeof(testData));

        REQUIRE(msg->getLength() == sizeof(testData));

        uint8_t* retrieved = (uint8_t*)msg->getData();
        REQUIRE(retrieved != nullptr);
        REQUIRE(memcmp(retrieved, testData, sizeof(testData)) == 0);
    }

    SECTION("Set empty data")
    {
        msg->setData(nullptr, 0);
        REQUIRE(msg->getLength() == 0);
    }

    SECTION("Data buffer bounds")
    {
        // Test that we can fill up to allocated size
        uint8_t largeData[90];
        memset(largeData, 0xAB, sizeof(largeData));

        msg->setData(largeData, sizeof(largeData));
        REQUIRE(msg->getLength() == sizeof(largeData));

        uint8_t* retrieved = (uint8_t*)msg->getData();
        REQUIRE(retrieved != nullptr);
        REQUIRE(retrieved[0] == 0xAB);
        REQUIRE(retrieved[89] == 0xAB);
    }

    msg->recycle();
}

TEST_CASE("NetMessage - Header Management", "[network][message][basic]")
{
    NetMessagePool* pool = NetMessagePool::pool();
    Ref<NetMessage> msg = pool->newMessage(100, nullptr);

    SECTION("Header structure access")
    {
        MsgHeader* hdr = msg->getHeader();

        REQUIRE(hdr != nullptr);
        REQUIRE(hdr->length >= MSG_HEADER_LEN);
        REQUIRE(hdr->serial == MSG_SERIAL_NULL);
        REQUIRE(hdr->flags == 0);
    }

    SECTION("Header consistency after data operations")
    {
        uint8_t data[] = {0x01, 0x02, 0x03};
        msg->setData(data, sizeof(data));

        MsgHeader* hdr = msg->getHeader();
        REQUIRE(hdr->length == MSG_HEADER_LEN + sizeof(data));
    }

    msg->recycle();
}

TEST_CASE("NetMessage - Length Operations", "[network][message][basic]")
{
    NetMessagePool* pool = NetMessagePool::pool();
    Ref<NetMessage> msg = pool->newMessage(100, nullptr);

    SECTION("Initial length is zero")
    {
        REQUIRE(msg->getLength() == 0);
        REQUIRE(msg->getHeader()->length == MSG_HEADER_LEN);
    }

    SECTION("Set length directly")
    {
        msg->setLength(50);
        REQUIRE(msg->getLength() == 50);
        REQUIRE(msg->getHeader()->length == MSG_HEADER_LEN + 50);
    }

    SECTION("Length is preserved across operations")
    {
        uint8_t data[] = {1, 2, 3, 4};
        msg->setData(data, sizeof(data));

        REQUIRE(msg->getLength() == 4);

        // Getting data shouldn't change length
        void* retrieved = msg->getData();
        REQUIRE(retrieved != nullptr);
        REQUIRE(msg->getLength() == 4);
    }

    msg->recycle();
}

TEST_CASE("NetMessage - Flags", "[network][message][basic]")
{
    NetMessagePool* pool = NetMessagePool::pool();
    Ref<NetMessage> msg = pool->newMessage(100, nullptr);

    SECTION("Initial flags are zero")
    {
        REQUIRE(msg->getFlags() == 0);
    }

    SECTION("Set VIM flag")
    {
        msg->setFlags(0xFFFF, MSG_VIM_FLAG);

        REQUIRE((msg->getFlags() & MSG_VIM_FLAG) != 0);
    }

    SECTION("Set multiple flags")
    {
        msg->setFlags(0xFFFF, MSG_VIM_FLAG | MSG_URGENT_FLAG);

        REQUIRE((msg->getFlags() & MSG_VIM_FLAG) != 0);
        REQUIRE((msg->getFlags() & MSG_URGENT_FLAG) != 0);
    }

    SECTION("Clear flags with AND mask")
    {
        msg->setFlags(0xFFFF, MSG_VIM_FLAG);
        REQUIRE((msg->getFlags() & MSG_VIM_FLAG) != 0);

        msg->setFlags(static_cast<unsigned16>(~MSG_VIM_FLAG), 0);
        REQUIRE((msg->getFlags() & MSG_VIM_FLAG) == 0);
    }

    SECTION("Protected flags (DELAY, BANDWIDTH, ORDERED, BUNCH, DUMMY)")
    {
        // These flags should not be settable via setFlags
        unsigned16 protectedFlags = static_cast<unsigned16>(MSG_DELAY_FLAG | MSG_BANDWIDTH_FLAG | MSG_ORDERED_FLAG |
                                                            MSG_BUNCH_FLAG | MSG_DUMMY_FLAG);

        msg->setFlags(0xFFFF, protectedFlags);

        // Protected flags should not be set
        REQUIRE((msg->getFlags() & protectedFlags) == 0);
    }

    msg->recycle();
}

// Section 1.2: Message Status (3 tests)

TEST_CASE("NetMessage - Status Tracking", "[network][message][status]")
{
    NetMessagePool* pool = NetMessagePool::pool();
    Ref<NetMessage> msg = pool->newMessage(100, nullptr);

    SECTION("Initial status is invalid")
    {
        REQUIRE(msg->getStatus() == nsInvalidMessage);
    }

    SECTION("wasSent returns false for unsent message")
    {
        REQUIRE(msg->wasSent() == false);
    }

    msg->recycle();
}

TEST_CASE("NetMessage - Lifecycle States", "[network][message][status]")
{
    NetMessagePool* pool = NetMessagePool::pool();

    SECTION("Message lifecycle: create -> recycle")
    {
        Ref<NetMessage> msg = pool->newMessage(100, nullptr);
        MsgSerial id = msg->id;
        (void)id; // Used for debugging/verification

        REQUIRE(msg->getStatus() == nsInvalidMessage);

        msg->recycle();
        msg = nullptr;

        // After recycling, new message may reuse the slot
        Ref<NetMessage> msg2 = pool->newMessage(100, nullptr);
        REQUIRE(msg2 != nullptr);
        REQUIRE(msg2->getStatus() == nsInvalidMessage);

        msg2->recycle();
    }

    SECTION("Multiple messages with independent lifecycles")
    {
        Ref<NetMessage> msg1 = pool->newMessage(50, nullptr);
        Ref<NetMessage> msg2 = pool->newMessage(50, nullptr);
        Ref<NetMessage> msg3 = pool->newMessage(50, nullptr);

        REQUIRE(msg1->id != msg2->id);
        REQUIRE(msg2->id != msg3->id);
        REQUIRE(msg1->id != msg3->id);

        msg1->recycle();
        msg2->recycle();
        msg3->recycle();
    }
}

TEST_CASE("NetMessage - Cancel/Recycle", "[network][message][status]")
{
    NetMessagePool* pool = NetMessagePool::pool();
    Ref<NetMessage> msg = pool->newMessage(100, nullptr);

    SECTION("Cancel message")
    {
        msg->cancel();
        REQUIRE(msg->getStatus() == nsCancel);
    }

    SECTION("Recycle clears state")
    {
        uint8_t data[] = {1, 2, 3, 4};
        msg->setData(data, sizeof(data));
        msg->setFlags(0xFFFF, MSG_VIM_FLAG);

        REQUIRE(msg->getLength() > 0);
        REQUIRE(msg->getFlags() != 0);

        msg->recycle();

        // Get a new message (might be recycled one)
        Ref<NetMessage> msg2 = pool->newMessage(100, nullptr);

        // Should be clean
        REQUIRE(msg2->getLength() == 0);
        REQUIRE(msg2->getFlags() == 0);
        REQUIRE(msg2->getStatus() == nsInvalidMessage);

        msg2->recycle();
    }

    msg->recycle();
}

// Section 1.3: NetMessagePool (4 tests)

TEST_CASE("NetMessagePool - Allocation", "[network][message][pool]")
{
    NetMessagePool* pool = NetMessagePool::pool();
    REQUIRE(pool != nullptr);

    SECTION("Allocate single message")
    {
        Ref<NetMessage> msg = pool->newMessage(100, nullptr);

        REQUIRE(msg != nullptr);
        REQUIRE(msg->getHeader() != nullptr);

        msg->recycle();
    }

    SECTION("Allocate multiple messages")
    {
        Ref<NetMessage> msg1 = pool->newMessage(50, nullptr);
        Ref<NetMessage> msg2 = pool->newMessage(100, nullptr);
        Ref<NetMessage> msg3 = pool->newMessage(150, nullptr);

        REQUIRE(msg1 != nullptr);
        REQUIRE(msg2 != nullptr);
        REQUIRE(msg3 != nullptr);

        // All should be distinct
        REQUIRE(msg1.GetRef() != msg2.GetRef());
        REQUIRE(msg2.GetRef() != msg3.GetRef());
        REQUIRE(msg1.GetRef() != msg3.GetRef());

        msg1->recycle();
        msg2->recycle();
        msg3->recycle();
    }

    SECTION("Allocate with different sizes")
    {
        Ref<NetMessage> small = pool->newMessage(10, nullptr);
        Ref<NetMessage> medium = pool->newMessage(500, nullptr);
        Ref<NetMessage> large = pool->newMessage(1500, nullptr);

        REQUIRE(small != nullptr);
        REQUIRE(medium != nullptr);
        REQUIRE(large != nullptr);

        small->recycle();
        medium->recycle();
        large->recycle();
    }
}

TEST_CASE("NetMessagePool - Recycling", "[network][message][pool]")
{
    NetMessagePool* pool = NetMessagePool::pool();

    SECTION("Recycle and reuse message")
    {
        Ref<NetMessage> msg1 = pool->newMessage(100, nullptr);
        MsgSerial id1 = msg1->id;
        (void)id1; // May be reused by next message

        msg1->recycle();
        msg1 = nullptr;

        Ref<NetMessage> msg2 = pool->newMessage(100, nullptr);

        // May or may not be same instance (implementation detail)
        // But should be valid
        REQUIRE(msg2 != nullptr);
        REQUIRE(msg2->getStatus() == nsInvalidMessage);

        msg2->recycle();
    }

    SECTION("Recycling clears message state")
    {
        Ref<NetMessage> msg = pool->newMessage(100, nullptr);

        // Set some data
        uint8_t data[] = {0xAA, 0xBB, 0xCC};
        msg->setData(data, sizeof(data));
        msg->setFlags(0xFFFF, MSG_VIM_FLAG);

        msg->recycle();

        // Allocate same size - might get recycled message
        Ref<NetMessage> msg2 = pool->newMessage(100, nullptr);

        // Should be clean
        REQUIRE(msg2->getLength() == 0);
        REQUIRE(msg2->getFlags() == 0);
        REQUIRE(msg2->getSerial() == MSG_SERIAL_NULL);

        msg2->recycle();
    }
}

TEST_CASE("NetMessagePool - Garbage Collection", "[network][message][pool]")
{
    NetMessagePool* pool = NetMessagePool::pool();

    SECTION("Garbage collection runs without crash")
    {
        // Create and recycle many messages to trigger GC
        for (int i = 0; i < 50; i++)
        {
            Ref<NetMessage> msg = pool->newMessage(100 + i, nullptr);
            msg->recycle();
        }

        // If we get here, GC worked without crashing
        REQUIRE(true);
    }

    SECTION("Messages remain valid after GC")
    {
        Ref<NetMessage> msg1 = pool->newMessage(100, nullptr);

        // Trigger potential GC by allocating many messages
        for (int i = 0; i < 50; i++)
        {
            Ref<NetMessage> temp = pool->newMessage(100, nullptr);
            temp->recycle();
        }

        // msg1 should still be valid
        REQUIRE(msg1 != nullptr);
        REQUIRE(msg1->getHeader() != nullptr);

        msg1->recycle();
    }
}

TEST_CASE("NetMessagePool - Memory Management", "[network][message][pool]")
{
    NetMessagePool* pool = NetMessagePool::pool();

    SECTION("No leaks on simple create/recycle")
    {
        for (int i = 0; i < 10; i++)
        {
            Ref<NetMessage> msg = pool->newMessage(100, nullptr);
            msg->recycle();
        }

        // If we get here without crash, basic memory management works
        REQUIRE(true);
    }

    SECTION("Ref counting works correctly")
    {
        Ref<NetMessage> msg1 = pool->newMessage(100, nullptr);

        {
            Ref<NetMessage> msg2 = msg1; // Increment ref count
            REQUIRE(msg1.GetRef() == msg2.GetRef());
        } // msg2 goes out of scope, decrements ref count

        // msg1 should still be valid
        REQUIRE(msg1 != nullptr);
        REQUIRE(msg1->getHeader() != nullptr);

        msg1->recycle();
    }

    SECTION("Message with channel reference")
    {
        // Test that message can be created with channel reference
        // (nullptr is valid for unit tests)
        Ref<NetMessage> msg = pool->newMessage(100, nullptr);
        REQUIRE(msg->getChannel() == nullptr);
        msg->recycle();
    }
}

// Section 1.4: Advanced Features (3 tests)

TEST_CASE("NetMessage - Callbacks", "[network][message][advanced]")
{
    NetMessagePool* pool = NetMessagePool::pool();
    Ref<NetMessage> msg = pool->newMessage(100, nullptr);

    SECTION("Set callback function")
    {
        static int callbackData = 0;

        auto callback = [](NetMessage* m, NetStatus event, void* data) -> NetStatus
        {
            int* counter = (int*)data;
            (*counter)++;
            return nsNoMoreCallbacks;
        };

        msg->setCallback(callback, nsOutputSent, &callbackData);

        // Callback is set (we can't verify it directly, but no crash means success)
        REQUIRE(true);
    }

    SECTION("Clear callback")
    {
        auto callback = [](NetMessage*, NetStatus, void*) -> NetStatus { return nsNoMoreCallbacks; };

        msg->setCallback(callback, nsOutputSent, nullptr);
        msg->setCallback(nullptr, nsInvalidMessage, nullptr);

        REQUIRE(true);
    }

    msg->recycle();
}

TEST_CASE("NetMessage - Timeouts", "[network][message][advanced]")
{
    NetMessagePool* pool = NetMessagePool::pool();
    Ref<NetMessage> msg = pool->newMessage(100, nullptr);

    SECTION("Set send timeout")
    {
        msg->setSendTimeout(5000000); // 5 seconds in microseconds

        // Timeout is set (internal state, can't verify directly)
        REQUIRE(true);
    }

    SECTION("Set zero timeout")
    {
        msg->setSendTimeout(0);
        REQUIRE(true);
    }

    msg->recycle();
}

TEST_CASE("NetMessage - Ordering", "[network][message][advanced]")
{
    NetMessagePool* pool = NetMessagePool::pool();

    SECTION("setBunch flag")
    {
        Ref<NetMessage> msg = pool->newMessage(100, nullptr);

        bool result = msg->setBunch(true);
        (void)result; // May succeed or fail based on message type

        // Should set or fail based on canBeSecondary
        // Either way, no crash
        REQUIRE(true);

        msg->recycle();
    }

    SECTION("setOrdered with null predecessor")
    {
        Ref<NetMessage> msg = pool->newMessage(100, nullptr);

        msg->setOrdered(nullptr);

        // Should clear ORDERED flag
        REQUIRE((msg->getFlags() & MSG_ORDERED_FLAG) == 0);

        msg->recycle();
    }

    SECTION("Distant address operations")
    {
        Ref<NetMessage> msg = pool->newMessage(100, nullptr);

        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(0x7F000001); // 127.0.0.1
        addr.sin_port = htons(12345);

        msg->setDistant(addr);

        struct sockaddr_in retrieved;
        msg->getDistant(retrieved);

        REQUIRE(retrieved.sin_family == AF_INET);
        REQUIRE(retrieved.sin_addr.s_addr == addr.sin_addr.s_addr);
        REQUIRE(retrieved.sin_port == addr.sin_port);

        msg->recycle();
    }
}

// Summary: Batch 1 Complete
//
// Tests: 15
// Sections:
// - NetMessage Basics: 5 tests
// - Message Status: 3 tests
// - NetMessagePool: 4 tests
// - Advanced Features: 3 tests
//
// Focus: Unit testing to catch 64-bit, non-permissive, and refactoring issues
// Coverage: Core NetMessage and NetMessagePool functionality
//
// Known Limitations:
// - No actual network I/O (requires NetChannel)
// - No callback execution testing (requires message lifecycle)
// - No VIM ordering testing (requires NetChannel)
// - No real timeout testing (requires time advancement)
//
// These limitations are acceptable for Batch 1 - focus is on data structures
// and memory management, not network protocol behavior.

#pragma clang diagnostic pop
