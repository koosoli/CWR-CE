#include <Poseidon/Network/Legacy/netpch.hpp>
#ifndef _WIN32
#include <netinet/in.h>
#endif
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <Poseidon/Foundation/Common/Global.hpp>
#include <Poseidon/Foundation/Common/NetGlobal.hpp>
#include <Poseidon/Foundation/Containers/Maps.hpp>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Poseidon/Foundation/Framework/Log.hpp>
#include <Poseidon/Foundation/Framework/PoTime.hpp>
#include <Poseidon/Foundation/Math/Statistics.hpp>
#include <Poseidon/Foundation/Memory/CheckMem.hpp>
#include <Poseidon/Foundation/Memory/MemAlloc.hpp>
#include <Poseidon/Foundation/Threads/PoCritical.hpp>
#include <Poseidon/Foundation/Types/Pointers.hpp>

using Poseidon::Foundation::IteratorState;
using Poseidon::Foundation::MSG_SERIAL_NULL;
using Poseidon::Foundation::nsCancel;
using Poseidon::Foundation::nsInvalidMessage;
using Poseidon::Foundation::nsNoMoreCallbacks;
using Poseidon::Foundation::nsOutputAck;
using Poseidon::Foundation::nsOutputSent;
using Poseidon::Foundation::nsOutputTimeout;
using Poseidon::Foundation::safeDelete;
using Poseidon::Foundation::safeNew;

MsgSerial NetMessage::nextId = 1;

NetMessage::NetMessage(unsigned len)
{
    LockRegister(lock, "NetMessage");
    if (len < MSG_HEADER_LEN)
    {
        len = MSG_HEADER_LEN;
    }
    data = (unsigned8*)safeNew(totalLen = len);
    processRoutine = nullptr;
    dta = nullptr;
    init();
}

MsgHeader* NetMessage::getHeader() const
{
    return header;
}

unsigned16 NetMessage::getFlags() const
{
    return header->flags;
}

unsigned NetMessage::getLength() const
{
    return (header->length - MSG_HEADER_LEN);
}

void* NetMessage::getData() const
{
    return (data ? data + sizeof(MsgHeader) : nullptr);
}

void NetMessage::getDistant(struct sockaddr_in& _distant) const
{
    _distant = distant;
}

void NetMessage::setDistant(struct sockaddr_in& _distant)
{
    distant = _distant;
}

MsgSerial NetMessage::getSerial() const
{
    return header->serial;
}

NetChannel* NetMessage::getChannel() const
{
    return channel;
}

unsigned64 NetMessage::getTime() const
{
    return refTime;
}

NetStatus NetMessage::getStatus() const
{
    return status;
}

void NetMessage::setFrom(const NetMessage* msg)
{
    NET_ERROR(msg);
    channel = msg->channel;
    distant = msg->distant;
    status = msg->status;
    header->flags = msg->header->flags & ~(MSG_PART_FLAG | MSG_CLOSING_FLAG);
    header->serial = msg->header->serial;
    header->ackOrigin = msg->header->ackOrigin;
    header->ackBitmask = msg->header->ackBitmask;
#ifdef MSG_ID
    header->id = msg->header->id;
#endif
    refTime = msg->refTime;
    firstTime = msg->firstTime;
    ackTimeout = msg->ackTimeout;
    sendTimeout = msg->sendTimeout;
    waitForLatency = msg->waitForLatency;
    canBeSecondary = msg->canBeSecondary;
    processRoutine = msg->processRoutine;
    dta = msg->dta;
    nextEvent = msg->nextEvent;
    heartBeatRequest = msg->heartBeatRequest;
    heartBeatTime = msg->heartBeatTime;
    pred = msg->pred;
    next = nullptr;
}

void NetMessage::init()
{
    status = nsInvalidMessage;
    processRoutine = nullptr;
    next = nullptr;
    pred = nullptr;
    channel = nullptr;
    memset(data, 0, totalLen);
    header->serial = MSG_SERIAL_NULL;
    header->flags = 0;
    header->length = MSG_HEADER_LEN;
    header->ackOrigin = MSG_SERIAL_NULL + 1;
    header->ackBitmask = 0;
    nextEvent = nsInvalidMessage;
    Zero(distant);
    waitForLatency = false;
    canBeSecondary = true;
    refTime = firstTime = sendTimeout = ackTimeout = 0L;
}

void NetMessage::setChannel(NetChannel* ch)
{
    channel = ch;
}

void NetMessage::setMessage(const MsgHeader* hdr)
{
    NET_ERROR(hdr);
    NET_ERROR(hdr->length);
    NET_ERROR(totalLen >= hdr->length);
    memcpy(data, (const char*)hdr, hdr->length);
    refTime = firstTime = Poseidon::Foundation::getSystemTime();
}

void NetMessage::setData(unsigned8* _data, unsigned32 length)
{
    NET_ERROR(!length || _data);
    if (length + MSG_HEADER_LEN > totalLen)
    {
        RptF("setData length %d>%d", length, totalLen - MSG_HEADER_LEN);
        Fail("Message data buffer underflow");
        length = totalLen - MSG_HEADER_LEN;
    }
    if (length)
    {
        memcpy(data + MSG_HEADER_LEN, _data, length);
    }
    header->length = length + MSG_HEADER_LEN;
}

void NetMessage::setLength(unsigned length)
{
    if (length + MSG_HEADER_LEN > totalLen)
    {
        length = totalLen - MSG_HEADER_LEN;
    }
    header->length = length + MSG_HEADER_LEN;
}

void NetMessage::setFlags(unsigned16 andMask, unsigned16 orMask)
{
    andMask |= (MSG_DELAY_FLAG | MSG_BANDWIDTH_FLAG | MSG_ORDERED_FLAG | MSG_BUNCH_FLAG | MSG_DUMMY_FLAG);
    orMask &= ~(MSG_DELAY_FLAG | MSG_BANDWIDTH_FLAG | MSG_ORDERED_FLAG | MSG_BUNCH_FLAG | MSG_DUMMY_FLAG);
    header->flags &= andMask;
    header->flags |= orMask;
}

bool NetMessage::setBunch(bool bunch)
{
    if (bunch && canBeSecondary)
    {
        header->flags |= MSG_BUNCH_FLAG;
        return true;
    }
    header->flags &= ~MSG_BUNCH_FLAG;
    return false;
}

void NetMessage::setOrdered(NetMessage* _pred)
{
    if (_pred)
    { // set VIM & ORDERED
        if (!(_pred->header->flags & MSG_VIM_FLAG))
        {
            return;
        }
        header->flags |= (MSG_ORDERED_FLAG | MSG_VIM_FLAG);
        if (_pred->getSerial() == MSG_SERIAL_NULL)
        {
            pred = _pred; // deferred
        }
        else
        {
            pred = nullptr; // I am able to set serial# of "pred" now
            header->c.control2 = _pred->getSerial();
        }
    }
    else
    { // unset ORDERED
        header->flags &= ~MSG_ORDERED_FLAG;
        pred = nullptr;
    }
}

void NetMessage::setOrderedPrevious()
{
    NET_ERROR(channel);
    channel->enter();
    pred = channel->getLastVIM((header->flags & MSG_URGENT_FLAG) != 0);
    channel->leave();
    if (pred)
    {
        header->flags |= (MSG_ORDERED_FLAG | MSG_VIM_FLAG);
        if (pred->header->flags & MSG_URGENT_FLAG)
        {
            header->flags |= MSG_URGENT_FLAG;
        }
        if (pred->getSerial() != MSG_SERIAL_NULL)
        { // not deferred
            header->c.control2 = pred->getSerial();
            NET_ERROR(header->c.control2 != MSG_SERIAL_NULL);
            pred = nullptr; // I am able to set serial# of "pred" now
        }
    }
    else
    {
        header->flags &= ~MSG_ORDERED_FLAG;
    }
}

void NetMessage::setSendTimeout(unsigned64 timeout)
{
    sendTimeout = timeout;
}

void NetMessage::setCallback(NetCallBack* routine, NetStatus event, void* _dta)
{
    processRoutine = routine;
    dta = _dta;
    nextEvent = routine ? event : nsInvalidMessage;
}

void NetMessage::send(bool urgent)
{
    if (channel)
    {
        channel->dispatchMessage(this, urgent);
    }
}

NetMessage::~NetMessage()
{
#ifdef NET_LOG_DESTRUCTOR
    NetLog("~NetMessage[%08x]: serial=%4u, flags=%04x, len=(%3u,%3u)", (unsigned)this, header->serial, header->flags,
           header->length, totalLen);
#endif
    init();
    if (data)
    {
        safeDelete(data);
        data = nullptr;
    }
}

bool NetMessage::wasSent() const
{
    return (status == nsOutputSent || status == nsOutputTimeout || status == nsOutputAck);
}

void NetMessage::cancel()
{
    status = nsCancel;
    if (processRoutine && nextEvent != nsNoMoreCallbacks)
    {
        nextEvent = (*processRoutine)(this, nsCancel, dta);
    }
}

void NetMessage::recycle()
{
    NetMessagePool::pool()->recycleMessage(this);
}

void* NetMessage::operator new(size_t size)
{
    return safeNew(size);
}

void* NetMessage::operator new(size_t size, const char* file, int line)
{
    return safeNew(size);
}

void NetMessage::operator delete(void* mem)
{
    safeDelete(mem);
}

const unsigned NET_MESSAGE_POOL_TRY = 32; // Slots to try when no exact size-match is found.
#ifdef NET_TEST
const int NET_MESSAGE_POOL_GARBAGE = 10000; // Periodic garbageCollect() invocation.
#else
const int NET_MESSAGE_POOL_GARBAGE = 1000; // Periodic garbageCollect() invocation.
#endif
const unsigned MAX_RECYCLED_MESSAGE_SIZE = 2048;
// Messages this size or larger are disposed immediately.

unsigned netMessageToUnsigned(RefD<NetMessage>& msg)
{
    if (!msg)
    {
        return 0;
    }
    return (msg->totalLen - MSG_HEADER_LEN);
}

// Template specialization static members - unavoidable in C++14
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wglobal-constructors"
#pragma clang diagnostic ignored "-Wexit-time-destructors"

RefD<NetMessage> ImplicitMapTraits<RefD<NetMessage>>::zombie = (NetMessage*)1;
RefD<NetMessage> ImplicitMapTraits<RefD<NetMessage>>::null;

#pragma clang diagnostic pop

// NOTE: Truncates 64-bit addresses to 32-bit for hashing (acceptable for hash map keys).
unsigned netMessageAddressToUnsigned(RefD<NetMessage>& msg)
{
    return (unsigned)(uintptr_t)msg.GetRef();
}

NetMessagePool::NetMessagePool() : recycled(400), used(12)
{
    LockRegister(lock, "NetMessagePool");
    garbageCounter = NET_MESSAGE_POOL_GARBAGE;
}

NetMessagePool::~NetMessagePool()
{
#ifdef NET_LOG_DESTRUCTOR
    NetLog("~NetMessagePool[%08x]: used=%u, recycled=%u", (unsigned)this, used.card(), recycled.card());
#endif
}

Ref<NetMessage> NetMessagePool::newMessage(unsigned minLen, NetChannel* ch)
{
    enter();
    if (--garbageCounter <= 0)
    {
#ifdef TEST_NET_FREE_MEMORY
        static int freeMemoryCounter = 10;
        if (--freeMemoryCounter <= 0)
        {
            unsigned freed = freeMemory();
#ifdef NET_LOG
            NetLog("Freeing up the NetMessagePool memory: %u bytes", freed);
#else
            LOG_DEBUG(Core, "Freeing up the NetMessagePool memory: {} bytes", freed);
#endif
            freeMemoryCounter = 10;
        }
        else
        {
            garbageCounter = NET_MESSAGE_POOL_GARBAGE;
            garbageCollect();
        }
#else
        garbageCounter = NET_MESSAGE_POOL_GARBAGE;
        garbageCollect();
#endif
    }
    RefD<NetMessage> result;
    unsigned taken = minLen;
    if (minLen < MAX_RECYCLED_MESSAGE_SIZE)
    {
        while (!recycled.get(taken, result) && (++taken < minLen + NET_MESSAGE_POOL_TRY))
        {
            ;
        }
    }

#ifdef NET_LOG_NEW_MESSAGE
    NetLog("NetMessagePool::newMessage: %s, len=(%u,%u)", (result ? "recycled" : "new"), minLen, taken);
#endif
    if (!result)
    {
        result = new NetMessage(minLen + MSG_HEADER_LEN);
    }
    else
    {
        NetMessage* next = result->next;
        if (!next)
        {
            recycled.remove(result);
        }
        else
        {
            recycled.put(next);
        }
        result->init();
    }
    used.put(result);
#ifdef MSG_ID
    result->header->id =
#endif
        result->id = NetMessage::nextId++;
    leave();
    result->setChannel(ch);
    return result.GetRef();
}

void NetMessagePool::recycleMessage(NetMessage* msg)
{
    if (!msg)
    {
        return;
    }
    enter();
    if (used.present(msg))
    {
#ifdef NET_LOG_RECYCLE_MESSAGE
        NetLog("NetMessagePool::recycleMessage: len=%3u, serial=%4u", msg->getLength(), msg->getSerial());
#endif
        msg->init();
        if (msg->totalLen - MSG_HEADER_LEN < MAX_RECYCLED_MESSAGE_SIZE)
        {
            // SAFETY: the recycled map must own a reference. AddRef before removing from
            // 'used' so the message survives the transfer between the two maps.
            msg->AddRef();
            RefD<NetMessage> old;
            recycled.put(msg, &old);
            msg->next = old;
            used.remove(msg);
        }
        else
        {
            // too large to recycle: drop from 'used' (it will be deleted)
            used.remove(msg);
        }
    }
    else
    {
#ifdef NET_LOG_RECYCLE_MESSAGE
        NetLog("NetMessagePool::recycleMessage: UNKNOWN MESSAGE len=%3u, serial=%4u", msg->getLength(),
               msg->getSerial());
#endif
        msg->Release();
    }
    leave();
}

void NetMessagePool::garbageCollect()
{
#ifdef NET_LOG_GARBAGE_COLLECT
    unsigned count = 0;
    unsigned size = 0;
    unsigned start = used.card();
#endif
    IteratorState it;
    enter();
    RefD<NetMessage> msg;
    if (used.getFirst(it, msg))
    {
        do
        {
            while (msg->RefCounter() > 2 && // look for NetMessage-s referenced only once
                   used.getNext(it, msg))
            {
                ;
            }
            if (!msg)
            {
                break;
            }
#ifdef NET_LOG_GARBAGE_COLLECT
            count++;
            size += msg->totalLen + sizeof(NetMessage);
#endif
            recycleMessage(msg);
        } while (used.getNext(it, msg));
    }
#ifdef NET_LOG_GARBAGE_COLLECT
    NetLog("NetMessagePool::garbageCollect: %u (of %u) messages has been recycled (%u bytes)", count, start, size);
#endif
    leave();
}

unsigned NetMessagePool::freeMemory()
{
    garbageCounter = NET_MESSAGE_POOL_GARBAGE;
    garbageCollect();
    unsigned size = 0;
    enter();
    IteratorState it;
    NetMessage* tmp;
    RefD<NetMessage> msg;
    if (recycled.getFirst(it, msg))
    {
        do
        {
            tmp = msg;
            msg->Release();
            while (tmp)
            { // count one message
                if (tmp->RefCounter() != 1)
                {
                    LOG_DEBUG(Core, "NetMessagePool::freeMemory: message has RefCounter!=1");
                    break;
                }
                size += tmp->totalLen + sizeof(NetMessage);
                tmp = tmp->next;
            }
            msg->AddRef();
            recycled.remove(msg);
        } while (recycled.getNext(it, msg));
    }
    leave();
    return size;
}

#ifdef SAFE_HEAP_STAT

StatisticsByName* NetMessagePool::getStatistics()
{
    StatisticsByName* stat = new StatisticsByName;
    NET_ERROR(stat);
    enter();
    IteratorState it;
    char buf[16];
    RefD<NetMessage> msg;
    if (recycled.getFirst(it, msg))
    {
        do
        {
            snprintf(buf, sizeof(buf), "%4u", msg->totalLen);
            int count = 0;
            while (msg)
            {
                count++;
                msg = (NetMessage*)msg->next;
            }
            stat->Count(buf, count);
        } while (recycled.getNext(it, msg));
    }
    leave();
    return stat;
}

StatisticsByName* NetMessagePool::getStatisticsUsed()
{
    StatisticsByName* stat = new StatisticsByName;
    NET_ERROR(stat);
    enter();
    IteratorState it;
    char buf[16];
    RefD<NetMessage> msg;
    if (used.getFirst(it, msg))
    {
        do
        {
            snprintf(buf, sizeof(buf), "%4u", msg->totalLen);
            stat->Count(buf);
        } while (used.getNext(it, msg));
    }
    leave();
    return stat;
}

StatisticsByName* messagePoolStatistics()
{
    if (!NetMessagePool::pool())
    {
        return nullptr;
    }
    return NetMessagePool::pool()->getStatistics();
}

StatisticsByName* messagePoolStatisticsUsed()
{
    if (!NetMessagePool::pool())
    {
        return nullptr;
    }
    return NetMessagePool::pool()->getStatisticsUsed();
}

#endif
