#include <Poseidon/Network/Legacy/SocketUtil.hpp>
#include <Poseidon/Network/Legacy/NetApi.hpp>
#include <Poseidon/Foundation/Threads/PoThread.hpp>
#ifdef _MSC_VER
#pragma once
#endif

#ifndef _NETMESSAGE_H
#define _NETMESSAGE_H

#define SAFE_HEAP_STAT

class NetMessagePool;

// Container for data sent/received by the network layer. Network-layer clients
// should use NetMessage's methods in most cases.
class NetMessage : public Poseidon::Foundation::RefCountSafe
{
  protected:
    NetChannel* channel;

    // Network address of the distant peer (network endian).
    struct sockaddr_in distant;

    NetStatus status;

    union
    {
        MsgHeader* header;

        // Data buffer (used from data[sizeof(MsgHeader)]).
        unsigned8* data;
    };

    // Total length of the allocated data[] buffer in bytes.
    unsigned totalLen;

    // Reference time (absolute receive/send time) in micro-seconds.
    unsigned64 refTime;

    // Absolute time the message was first dispatched.
    unsigned64 firstTime;

    // Relative timeout (re-send time if not acknowledged) in micro-seconds.
    unsigned64 ackTimeout;

    // Relative too-old time: send-pending messages older than this are not sent. 0 if unused.
    unsigned64 sendTimeout;

    // Remember this message for latency computation?
    bool waitForLatency;

    // Can this message be secondary (for packet-bunch bandwidth estimation)?
    bool canBeSecondary;

    // Call-back processing message life-cycle events.
    NetCallBack* processRoutine;

    // Data passed to processRoutine.
    void* dta;

    NetStatus nextEvent;

    // Serial number of the message that requested this MSG_DELAY_FLAG (heart-beat).
    MsgSerial heartBeatRequest;

    // Incoming time of the heart-beat request.
    unsigned64 heartBeatTime;

    // Predecessor for VIM-ORDERED messages.
    Ref<NetMessage> pred;

    // Re-initialize (recycle) the instance.
    void init();

    virtual void setChannel(NetChannel* ch);

    friend class NetMessagePool;
    friend unsigned netMessageToUnsigned(RefD<NetMessage>& msg);
    friend class NetChannelBasic;
    friend THREAD_PROC_RETURN THREAD_PROC_MODE udpSend(void* param);

    static MsgSerial nextId;

  public:
    // Unique message ID within the whole application.
    MsgSerial id;

    // Linked list of NetMessage instances.
    Ref<NetMessage> next;

    // Create a message buffer of the given maximal physical length in bytes.
    NetMessage(unsigned len);

    // Recycles the message's memory.
    ~NetMessage() override;

    // Set message data from a raw header (followed by data) in network format;
    // only length and flags are used.
    virtual void setMessage(const MsgHeader* hdr);

    // Copy all control items (but not the data itself) from another message.
    virtual void setFrom(const NetMessage* msg);

    // Set the message's user data. length excludes the header.
    virtual void setData(unsigned8* _data, unsigned32 length);

    // Set the netto message length in bytes (excludes all headers).
    virtual void setLength(unsigned length);

    // AND-then-OR the message flags. Ignores MSG_DELAYED/FRACTION/ORDERED flags
    // (use the dedicated methods for those). andMask is applied first, then orMask.
    virtual void setFlags(unsigned16 andMask, unsigned16 orMask);

    // Mark this message as "secondary" for packet-bunch bandwidth estimation;
    // returns whether it was actually marked.
    virtual bool setBunch(bool bunch);

    // Mark as VIM & ORDERED, depending on pred (received before this one);
    // nullptr unsets ORDERED.
    virtual void setOrdered(NetMessage* pred);

    // Mark as VIM & ORDERED, depending on the previous VIM message.
    virtual void setOrderedPrevious();

    // Set the send-timeout (in micro-seconds).
    virtual void setSendTimeout(unsigned64 timeout);

    // Set the per-message event callback: routine, the next event to respond to,
    // and the user data passed to the routine.
    virtual void setCallback(NetCallBack* routine, NetStatus event, void* _dta);

    // Dispatch this message to the associated output NetChannel.
    virtual void send(bool urgent = false);

    // Direct access to the message header. Use with care.
    virtual MsgHeader* getHeader() const;

    virtual unsigned16 getFlags() const;

    // Length of the user part of the message (excludes the system header).
    virtual unsigned getLength() const;

    // Message data (safe access).
    virtual void* getData() const;

    // Distant network address (network endian).
    virtual void getDistant(struct sockaddr_in& _distant) const;

    // Set the distant network address (network endian). Control channels only.
    virtual void setDistant(struct sockaddr_in& _distant);

    // Serial number (MSG_SERIAL_NULL if not assigned yet).
    virtual MsgSerial getSerial() const;

    // Associated net-channel (nullptr if not in processing mode).
    virtual NetChannel* getChannel() const;

    // Reference (receive/send) time in micro-seconds.
    virtual unsigned64 getTime() const;

    virtual NetStatus getStatus() const;

    // Was this message sent? Acknowledgement does not matter.
    virtual bool wasSent() const;

    virtual void cancel();

    // Return the message to the pool for re-use.
    virtual void recycle();

    // MT-safe new/delete.
    static void* operator new(size_t size);

    static void* operator new(size_t size, const char* file, int line);

    static void operator delete(void* mem);

#ifdef __INTEL_COMPILER

    static void operator delete(void* mem, const char* file, int line);

#endif
};

extern unsigned netMessageToUnsigned(RefD<NetMessage>& msg);

extern unsigned netMessageAddressToUnsigned(RefD<NetMessage>& msg);

template <>
struct ImplicitMapTraits<RefD<NetMessage>>
{
    static RefD<NetMessage> zombie;
    static RefD<NetMessage> null;
};

namespace Poseidon::Foundation { template class ImplicitMap<unsigned, RefD<NetMessage>, netMessageToUnsigned, true, Poseidon::Foundation::MemAllocSafe>; } // namespace Poseidon::Foundation

#ifdef SAFE_HEAP_STAT

namespace Poseidon { class QOStream; }
using Poseidon::QOStream;

#include <Poseidon/Foundation/Math/Statistics.hpp>

#endif

#include <Poseidon/Foundation/Memory/MemFreeReq.hpp>

// Allocator/manager maintaining a recycle-pool of NetMessage instances.
// Can be shared between NetPools and threads.

class RefDNetMessagePool : public RefD<NetMessagePool>, public Poseidon::Foundation::MemoryFreeOnDemandHelper
{
  public:
    RefDNetMessagePool() = default;
    ~RefDNetMessagePool() override = default;

    // MemoryFreeOnDemandHelper interface.
    size_t FreeOneItem() override;
    float Priority() override;
};

class NetMessagePool : public Poseidon::Foundation::RefCountSafe
{
  protected:
    // Non-public constructor (singleton).
    NetMessagePool();

    // Must run only after the network layer is completely shut down.
    ~NetMessagePool() override;

    // Recycled message pool (key is message length).
    ImplicitMap<unsigned, RefD<NetMessage>, netMessageToUnsigned, true, Poseidon::Foundation::MemAllocSafe> recycled;

    // Container for in-use message instances.
    ImplicitMap<unsigned, RefD<NetMessage>, netMessageAddressToUnsigned, true, Poseidon::Foundation::MemAllocSafe> used;

    // Counter driving automatic garbage collection.
    int garbageCounter;

  public:
    // Access to the singleton (Meyer's singleton).
    static NetMessagePool* pool()
    {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
        static NetMessagePool instance;
#pragma clang diagnostic pop
        return &instance;
    }

    // Return a new NetMessage with exclusive access. minLen excludes the low-level
    // header; ch is the channel the message is associated with.
    virtual Ref<NetMessage> newMessage(unsigned minLen, NetChannel* ch);

    // Recycle a message instance so it can be re-used again.
    virtual void recycleMessage(NetMessage* msg);

    // Collect all allocated messages that should be recycled.
    virtual void garbageCollect();

    // Free all recycled memory; returns the number of bytes freed.
    virtual unsigned freeMemory();

#ifdef SAFE_HEAP_STAT

    virtual StatisticsByName* getStatistics();

    virtual StatisticsByName* getStatisticsUsed();

#endif
};

#ifdef SAFE_HEAP_STAT

StatisticsByName* messagePoolStatistics();

StatisticsByName* messagePoolStatisticsUsed();

#endif

#endif
