#include <Poseidon/Network/Legacy/netpch.hpp>

using namespace Poseidon;
#include <Poseidon/Network/Legacy/NetPeer.hpp>
#include <Poseidon/Network/Legacy/NetChannel.hpp>
#include <Poseidon/Network/WireBounds.hpp>
#include <string.h>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Poseidon/Foundation/Framework/PoTime.hpp>
#include <Poseidon/Foundation/Memory/CheckMem.hpp>
#include <Poseidon/Foundation/Threads/PoCritical.hpp>

using Poseidon::Foundation::IteratorState;
using Poseidon::Foundation::MSG_SERIAL_NULL;
using Poseidon::Foundation::nsError;
using Poseidon::Foundation::nsInputReceived;
using Poseidon::Foundation::nsInvalidSharing;
using Poseidon::Foundation::nsOK;
using Poseidon::Foundation::nsOutputAck;
using Poseidon::Foundation::nsOutputObsolete;
using Poseidon::Foundation::nsOutputPending;
using Poseidon::Foundation::nsOutputSent;
using Poseidon::Foundation::nsOutputTimeout;
using Poseidon::Foundation::safeDelete;
using Poseidon::Foundation::safeNew;

#define LATENCY_F 0.30f // Fade-out coefficient for latency.

#define BANDWIDTH_F \
    0.15f // Fade-out coefficient for "receiver-based" estimation of bandwidth (fading is computed on sender side).
#define LOCAL_BANDWIDTH_F \
    0.25f // Fade-out coefficient for "receiver-based" estimation of bandwidth (fading is computed on receiver for
          // filtering purposes).

#define RBE_WEIGHT 0.00f // Weight of receiver-based bandwidth estimation (based on packet-bunch method).
#ifdef NET_TEST
#define PACKET_PAIRS // Send packet-pairs at all? Must be defined if RBE_WEIGHT > 0.0.
#else
#undef PACKET_PAIRS // Send packet-pairs at all? Must be defined if RBE_WEIGHT > 0.0.
#endif

#define MAX_PING_GAP 6000000      // Maximum interval between incoming "ping" (heart-beat) messages.
#define PING_TRY_INTERVAL 3000000 // Minimum try-interval for "ping" requests.

#define MAX_ACK_TIMEOUT 7000000 // Maximum NetMessage->ackTimeout

#define MAX_HEART_BEAT_GAP 2000000 // Maximum interval without channel traffic (actual value is compute dynamically).

#define MAX_TIMEOUT 4000000 // Maximum value for default ack-timeout of NetMessages.

#define BANDWIDTH_INTERVAL 600000 // minimum interval for "receiver-based bandwidth" to be send to the sender.

#define INIT_MIN_LATENCY 800000 // minLatency init (instead of INT_MAX).

#define NOT_RECEIVED 0xff // Message was not received yet.

#define MAX_PAIR_SEND_GAP 50000 // Maximum send-gap for packet-pair (in microseconds).

#define PACKET_PAIR_COEF 0.025f // "Packet-pair size / actual ack-bandwidth" ratio

#define MIN_PACKET_PAIR 50 // Minimum size of packet-pair message.

#define MAX_PACKET_PAIR 800 // Maximum size of packet-pair message.

#define MAX_PACKET_PAIR_DELAY \
    200000 // Maximum received packet-pair delay in microseconds (computed for 14.4kBaud modem).

#define GOOD_CHANNEL_BIT_MASK 4096 // stretching delta of NetChannel bit-masks.

#define BIG_ACK_MAGIC 0xb18ac212 // Magic number for big-ack packet.

#define OUT_QUEUE_GRANUL 50000 // Minimum separation between two sendQueue items..

#define ACK_QUEUE_GRANUL 100000 // Minimum separation between two ackStatQueue items.

#define URGENT_MSG_THRESHOLD \
    30 // If more than URGENT_MSG_THRESHOLD urgent messages are waiting, send them with absolute priority.

#define VIM_MSG_THRESHOLD \
    10 // If more than VIM_MSG_THRESHOLD VIM messages are waiting, send them with absolute priority.

// Default network parameters..
const NetworkParams defaultNetworkParams = {

    1.1f, // winsockVersion

    65535, // rcvBufSize

    1490, // maxPacketSize

    90, // dropGap

    3, // ackTimeoutA

    400000, // ackTimeoutB

    2, // ackRedundancy

    4000, // initBandwidth

    600, // minBandwidth

    2000000, // maxBandwidth

    400, // minActivity

    0, // initLatency

    6000000, // minLatencyUpdate

    1.04f, // minLatencyMul

    1000.f, // minLatencyAdd

    0.98f, // goodAckBandFade

    3000000, // outWindow

    3000000, // ackWindow

    65536, // maxChannelBitMask

    2.5f, // lostLatencyMul

    150000.f, // lostLatencyAdd

    400, // maxOutputAckMask

    15, // minAckHistory

    0.150f, // maxDropouts

    0.100f, // midDropouts

    0.050f, // okDropouts

    0.020f, // minDropouts

    2.2f, // latencyOverMul (upper bound for -1 mode)

    30000.f, // latencyOverAdd

    1.8f, // latencyWorseMul (upper bound for 0 mode)

    20000.f, // latencyWorseAdd

    1.4f, // latencyOkMul (upper bound for 1 mode)

    10000.f, // latencyOkAdd

    1.1f, // latencyBestMul (upper bound for 2 mode)

    3000.f, // latencyBestAdd

    1.4f, // maxBandOverGood

    1.2f, // safeMaxBandOverGood

    {
        // grow[]
        {0.90f, 20.f}, // Converges to 200B/s

        {0.98f, 10.f}, // Converges to 500B/s

        {1.00f, 0.f}, // I'm satisfied

        {1.02f, 20.f}, // Conservative..

        {1.10f, 40.f}, // Radical..

        {1.30f, 50.f} // Initial optimism..
    }

};

NetworkParams NetChannelBasic::par = defaultNetworkParams;

// Packet sent in "dummy" messages, used now and then for packet-pair bandwidth estimation.
struct BigAckPacket
{
    unsigned32 magic; // Must be BIG_ACK_MAGIC.

    unsigned32 bandWidth; // actual bandwidth computed on the receiver (in bytes per second).

    unsigned32 newest; // Serial number of the latest received message.

    unsigned32 oldest; // Serial number of the oldest message acknowledged in this packet. (newest-oldest+1) is always
                       // multiply of 32.

#pragma warning(push)
#pragma warning(disable : 4200) // Flexible array member - intentional for variable-length network packets
    unsigned32 ack[0];          // The acknowledgement data itself (LSB .. recent serial numbers).
#pragma warning(pop)
};

#define MIN_BIG_ACK_SIZE (sizeof(BigAckPacket) + 4)

MsgSerial netMessageToSerial(RefD<NetMessage>& msg)
{
    if (!msg)
    {
        return MSG_SERIAL_NULL;
    }
    return msg->getSerial();
}

MsgSerial netMessageDepend(RefD<NetMessage>& msg)
{
    if (!msg)
    {
        return MSG_SERIAL_NULL;
    }
    NET_ERROR(msg->getHeader()->flags & MSG_ORDERED_FLAG);
    return ((MsgSerial)msg->getHeader()->c.control2);
}

NetChannelBasic::NetChannelBasic(bool _control) : revisited(80, 0.75f), deferred(5)
{
    LockRegister(lock, "NetChannelBasic");
    opened = false;
    control = _control;
    Zero(dist);
    dist.sin_addr.s_addr = INADDR_BROADCAST;
    peer = nullptr;
    // input:
    inputMax = inputMin = ackMaskMin = ackPtr = serial = MSG_SERIAL_NULL + 1;
    lastSerialSent = MSG_SERIAL_NULL - 1;
    oldAckFirst = oldAckLast = 0;
    aveLatency = par.initLatency;
    actLatency = rbeBandwidth = 0;
    minLatency = INIT_MIN_LATENCY;
    goodAckBandwidth = par.initBandwidth;
    minLatencyTime = 0;
    heartBeatGap = timeout;
    // output:
    urgentToSend = nullptr;
    vimToSend = nullptr;
    commonToSend = nullptr;
    // statistics:
    lastPingArrival = lastPingDeparture = lastMsgArrival = lastMsgDeparture = lastPairDeparture =
        nextBandwidthDeparture = runTime = adjustTime = Poseidon::Foundation::getSystemTime();
    recentVIMs = 0;
    ackMin = ackMax = 0;
    // acknowledgements from the other peer..
    Zero(ackStatQueue);
    int i;
    for (i = 2; i < SLIDING_WINDOW; i += 2)
    {
        ackStatQueue[i] = runTime - par.ackWindow;
    }
    ackStatQueue[0] = runTime;
    ackStatQueue[1] = (par.initBandwidth * (unsigned64)par.ackWindow) / 1000000;
    ackStatIndex = 0;
    // sent messages:
    Zero(sendQueue);
    for (i = 0; i < SLIDING_WINDOW_SEND; i += 2)
    {
        sendQueue[i] = runTime;
    }
    sendIndex = 0;
    maxBandwidth = par.initBandwidth;
    growStatePing = MAX_GROW_STATE;
    growStateLost = MAX_GROW_STATE;
    growState = MAX_GROW_STATE + 1;
    // RBE:
    inSize = 0;
    inDelay = 0;
    inRbe = 0;
    Zero(ackTime);
#ifdef NET_LOG_BANDWIDTH
    inCounter = 0;
    getOutputBandWidthCounter = 0;
#endif
#ifdef NET_LOG_CH_STATE
    lastChannelStateLog = Poseidon::Foundation::getSystemTime();
    packetsOut = packetsOutVIM = bytesOut = packetsIn = packetsInVIM = bytesIn = 0;
#endif
}

void* NetChannelBasic::operator new(size_t size)
{
    return safeNew(size);
}

void* NetChannelBasic::operator new(size_t size, const char* file, int line)
{
    return safeNew(size);
}

void NetChannelBasic::operator delete(void* mem)
{
    safeDelete(mem);
}

#include <cstddef>

NetStatus NetChannelBasic::open(NetPeer* _peer, struct sockaddr_in& distant)
{
    if (opened || !_peer)
    {
        return nsError;
    }
    peer = _peer;
    dist = distant;
    if (!control && // point-to-point channel
        !peer->registerChannel(distant, this))
    {
        return nsInvalidSharing;
    }
    enter();
#ifdef NET_LOG_CHANNEL
    char buf[256];
#ifdef NET_LOG_BRIEF
    NetLog("Ch(%u):open(%s)", getChannelId(), getChannelInfo(buf));
#else
    NetLog("Channel(%u)::open succeeded: %s", getChannelId(), getChannelInfo(buf));
#endif
#endif
    opened = true;
    inputMax = inputMin = ackMaskMin = ackPtr = serial = MSG_SERIAL_NULL + 1;
    lastSerialSent = MSG_SERIAL_NULL - 1;
    oldAckFirst = oldAckLast = 0;
    int i;
    for (i = 0; i < MAX_ACK_ARRAY;)
    {
        ack[i++] = NOT_RECEIVED;
    }
    aveLatency = par.initLatency;
    actLatency = rbeBandwidth = 0;
    minLatency = INIT_MIN_LATENCY;
    goodAckBandwidth = par.initBandwidth;
    lastMsgArrival = nextBandwidthDeparture = runTime = adjustTime = Poseidon::Foundation::getSystemTime();
    lastPingArrival = runTime - MAX_PING_GAP;
    lastPingDeparture = runTime - PING_TRY_INTERVAL;
    lastPairDeparture = runTime - MAX_HEART_BEAT_GAP;
    lastMsgDeparture = runTime - heartBeatGap;
    minLatencyTime = runTime - par.minLatencyUpdate;
    recentVIMs = 0;
    // statistics:
    ackMin = ackMax = 0;
    // acknowledgements from the other peer..
    Zero(ackStatQueue);
    for (i = 2; i < SLIDING_WINDOW; i += 2)
    {
        ackStatQueue[i] = runTime - par.ackWindow;
    }
    ackStatQueue[0] = runTime;
    ackStatQueue[1] = (par.initBandwidth * (unsigned64)par.ackWindow) / 1000000;
    ackStatIndex = 0;
    // sent messages:
    Zero(sendQueue);
    for (i = 0; i < SLIDING_WINDOW_SEND; i += 2)
    {
        sendQueue[i] = runTime;
    }
    sendIndex = 0;
    maxBandwidth = par.initBandwidth;
    growStatePing = MAX_GROW_STATE;
    growStateLost = MAX_GROW_STATE;
    growState = MAX_GROW_STATE + 1;
    // RBE:
    inSize = 0;
    inDelay = 0;
    inRbe = 0;
#ifdef NET_LOG_BANDWIDTH
    inCounter = 0;
    getOutputBandWidthCounter = 0;
#endif
#ifdef NET_LOG_CH_STATE
    lastChannelStateLog = Poseidon::Foundation::getSystemTime();
    packetsOut = packetsOutVIM = bytesOut = packetsIn = packetsInVIM = bytesIn = 0;
#endif
    leave();
    return nsOK;
}

NetStatus NetChannelBasic::reconnect(struct sockaddr_in& distant)
{
    if (!opened || !peer)
    {
        return nsError;
    }
    NetStatus result = nsOK;
    enter();
    AddRef();
    peer->unregisterChannel(this);
    dist = distant;
    if (!peer->registerChannel(distant, this))
    {
        result = nsInvalidSharing;
    }
    Release();
    leave();
    return result;
}

void NetChannelBasic::setNetworkParams(const NetworkParams& p)
{
    par = p;
}

void NetChannelBasic::setGlobalNetworkParams(const NetworkParams& p)
{
    par = p;
}

void NetChannelBasic::getDistantAddress(struct sockaddr_in& distant) const
{
    distant = dist;
}

void NetChannelBasic::processData(MsgHeader* hdr, const struct sockaddr_in& distant)
// Process the given incoming data.
{
    NET_ERROR(hdr);
    NET_ERROR(hdr->length);
    enter();
    RefD<NetMessage> msg = NetMessagePool::pool()->newMessage(hdr->length - MSG_HEADER_LEN, this);
    if (!msg)
    {
        leave();
        return;
    }
    msg->setMessage(hdr);
#ifdef MSG_ID
    msg->id = msg->header->id;
#endif
    if (!control && received(hdr->serial))
    {
        inputResent(msg); // very old or duplicit message
        msg->recycle();
        leave();
        return;
    }
    msg->processRoutine = processRoutine;
    msg->dta = data;
    msg->distant = distant;
    msg->nextEvent = msg->status = nsInputReceived;
    const unsigned16 flags = hdr->flags;

    if (!control && !(flags & MSG_FROM_BCAST_FLAG))
    { // non-broadcast channel, non-control message
        // common incoming-message statistics:
        inputStatistics(msg);
        // process message attributes: 1. common flags
        if (flags & MSG_INSTANT_FLAG)
        { // instant reply (heart-beat) is required
            setDelayMessage(msg);
        }
        // process message attributes: 2. drop DUMMY message
        if (flags & MSG_DUMMY_FLAG)
        {
            leave();
            return;
        }
        // process message attributes: 3. VIM flags
        if (flags & MSG_VIM_FLAG)
        {
            if (flags & MSG_ORDERED_FLAG)
            { // this message depends on specific predecessor
                MsgSerial pred = (MsgSerial)hdr->c.control2;
                if (pred >= ackMaskMin && !procMask.get(pred))
                {
                    // put this message into deferred list, don't process it now:
#ifdef NET_LOG_PROCESS_DATA
                    NetLog("Channel(%u)::processData: message (%u) depends on another one (%u) - deferring",
                           getChannelId(), msg->getSerial(), pred);
#endif
                    RefD<NetMessage> old;
                    deferred.put(msg, &old);
                    msg->next = old; // simple but not too fair solution..
                    leave();
                    return;
                }
            }
            processVIM(msg);
            return;
        }
    }
    // ... and finally call user call-back routine:
    leave();
    if (processRoutine)
    {
        msg->nextEvent = (*processRoutine)(msg, nsInputReceived, data);
    }
}

void NetChannelBasic::processVIM(NetMessage* msg)
{
    NET_ERROR(msg);
    MsgSerial ser = msg->getSerial(); // dependency serial number
#ifdef NET_LOG_PROCESS_DATA
    if (msg->header->flags & MSG_ORDERED_FLAG)
    {
        if (!received(msg->header->c.control2))
            NetLog("Channel(%u)::processVIM before my predecessor is received: serial=%u,%u", getChannelId(), ser,
                   msg->header->c.control2);
        if (!procMask.get(msg->header->c.control2))
            NetLog("Channel(%u)::processVIM before my predecessor is processed: serial=%u,%u", getChannelId(), ser,
                   msg->header->c.control2);
    }
#endif
    if (ser >= ackMaskMin)
    { // the message isn't too old..
        NET_ERROR(!procMask.get(ser));
        if (processRoutine)
        {
            leave();
            msg->nextEvent = (*processRoutine)(msg, nsInputReceived, data);
            enter();
        }
        procMask.on(ser);
    }
    // check whether some deferred messages depend on me:
    RefD<NetMessage> def;
    deferred.remove(ser, &def); // pending message[s]
    while (def)
    { // process one pending message
#ifdef NET_LOG_PROCESS_DATA
        NetLog("Channel(%u)::processVIM: pending message (%u) processed after (%u)", getChannelId(), def->getSerial(),
               ser);
#endif
        RefD<NetMessage> next = def->next;
        def->next = nullptr;
        processVIM(def); // calls "leave()" at the end
        def = next;
        enter();
    }
    leave();
}

void NetChannelBasic::inputResent(NetMessage* msg)
// needs to be called inside NetChannelBasic::enter()
{
    NET_ERROR(msg);
    MsgSerial ser = msg->getSerial();
    if (ser < ackMaskMin)
    {
        return;
    }
    if (ser >= inputMin)
    {
        // rewrite message arrival time and ack[] item:
        int serI = ackPtr - (inputMax - ser);
        if (serI < 0)
        {
            serI += MAX_ACK_ARRAY;
        }
        ackTime[serI] = (unsigned)msg->refTime;
        if (msg->header->flags & MSG_VIM_FLAG)
        {
            ack[serI] = par.ackRedundancy; // this acknowledgement will be re-sent in future
        }
    }
    else
    {
        // insert priority ack-item into oldAckQueue:
        int newIndex = oldAckLast + 1;
        if (newIndex >= MAX_OLD_ACKS)
        {
            newIndex = 0;
        }
        if (newIndex != oldAckFirst)
        { // there is room for another item in oldAckQueue:
            oldAckQueue[oldAckLast] = ser;
            oldAckLast = newIndex;
        }
    }
}

void NetChannelBasic::newAcknowledgement(MsgSerial s, NetMessage* msg)
{
    outputAckMask.off(s);
    newOutputAckMask.off(s);
    if (!msg)
    {
        return;
    }
#ifdef NET_LOG_ACK_IN
    NetLog("Channel(%u)::inputStatistics: ack message (serial=%4u, len=%3u, time=%u)", getChannelId(), s,
           msg->header->length, (unsigned)(msg->refTime & 0xffffffff));
#endif
    msg->status = nsOutputAck;
    // band-width statistics:
    newAcks++;
    newBytes += msg->header->length + IP_UDP_HEADER;
    // call-back:
    if (msg->processRoutine && (msg->nextEvent == nsOutputTimeout || msg->nextEvent == nsOutputAck))
    {
        leave();
        msg->nextEvent = (*msg->processRoutine)(msg, nsOutputAck, msg->dta);
        enter();
    }
    // it will be recycled automatically after a while...
}

void NetChannelBasic::inputStatistics(NetMessage* msg)
// needs to be called inside NetChannelBasic::enter()
{
    NET_ERROR(msg);
    // update acknowledgements first:
    const MsgSerial ser = msg->getSerial();
    // ser is wire-controlled; ackMask.on(ser) grows the bit-mask and the inputMax
    // catch-up loop below iterates in proportion to ser's distance from the current
    // window, so a far-out serial drives a huge allocation / spin (N-SEC-12). Legit
    // serials sit within [ackMaskMin, inputMax] by the channel's own invariant; reject
    // anything outside a strictly wider window (wrap-aware signed deltas).
    if (!WireBounds::SerialWithinSpan(ser, ackMaskMin, inputMax, (int64_t)par.maxChannelBitMask))
    {
        return;
    }
    ackMask.on(ser);
    if (msg->header->flags & MSG_VIM_FLAG)
    {
        recentVIMs++;
    }
    if (ser > inputMax)
    {
        do
        {
            if (++ackPtr >= MAX_ACK_ARRAY)
            {
                ackPtr = 0;
            }
            ack[ackPtr] = NOT_RECEIVED;
        } while (++inputMax < ser);
        if (inputMax - inputMin >= MAX_ACK_ARRAY)
        {
            inputMin = inputMax - MAX_ACK_ARRAY + 1;
        }
    }
    // check size of my bit-masks:
    if (inputMax - ackMaskMin > par.maxChannelBitMask)
    {
        int newAckMaskMin = inputMax - par.maxChannelBitMask + GOOD_CHANNEL_BIT_MASK;
        ackMask.range(ackMaskMin, newAckMaskMin - ackMaskMin, false);
        ackMask.growOptimize(true, newAckMaskMin);
        procMask.range(ackMaskMin, newAckMaskMin - ackMaskMin, false);
        procMask.growOptimize(true, newAckMaskMin);
        ackMaskMin = newAckMaskMin;
    }

    int serI;
    if (ser >= inputMin)
    {
        serI = ackPtr - (inputMax - ser);
        if (serI < 0)
        {
            serI += MAX_ACK_ARRAY;
        }
        ack[serI] = par.ackRedundancy; // I'm going to send this acknowledgement "ackRedundancy" times..
    }
    else
    {
        serI = -1;
    }
    // starvation check:
    int i = ackPtr;
    MsgSerial notAck = inputMax - MAX_ACK_ARRAY;
    do
    {
        notAck++;
        if (++i >= MAX_ACK_ARRAY)
        {
            i = 0;
        }
        if (ack[i] > 0 && ack[i] != NOT_RECEIVED)
        {
            break;
        }
    } while (i != ackPtr);
    if (inputMax - notAck > 31 && // find the oldest non-acknowledged message
        !urgentToSend && !vimToSend && !commonToSend)
    {                      // no other messages are prepared => set the 'starvation' flag
        starvation = true; // starvation <=> more than 32 received messages are waiting for acknowledgement
    }

    // common channel time-statistics:
    lastMsgArrival = msg->refTime;
    if (serI >= 0)
    {
        ackTime[serI] = (unsigned)lastMsgArrival;
    }

    // channel latency:
    if (msg->header->flags & MSG_DELAY_FLAG)
    { // message is carrying transport-delay value
        lastPingArrival = msg->refTime;
        RefD<NetMessage> origMsg;
        revisited.get(msg->header->ackOrigin, origMsg);
        unsigned newActLatency;
#ifdef NET_LOG_LATENCY1
        NetLog("Channel(%u)::inputStatistics: MSG_DELAY message arrived after %.3f seconds (serial=%4u)",
               getChannelId(), 1.e-6 * (msg->refTime - lastPingArrival), ser);
        if (origMsg.NotNull())
            NetLog("    ping: refTime %x%08x, origrefTime %x%08x, diff %x%08x, control2 %x",
                   (unsigned)(msg->refTime >> 32), (unsigned)(msg->refTime & 0xffffffff),
                   (unsigned)(origMsg->refTime >> 32), (unsigned)(origMsg->refTime & 0xffffffff),
                   (unsigned)((msg->refTime - origMsg->refTime) >> 32),
                   (unsigned)((msg->refTime - origMsg->refTime) & 0xffffffff), msg->header->c.control2);
#endif
        if (origMsg.NotNull() && // MSG_INSTANT tagged (originator) message is still here
            (newActLatency = (unsigned)(msg->refTime - origMsg->refTime)) >= msg->header->c.control2)
        {
            // ignore invalid values completely..
            // compute current latency in micro-seconds:
            actLatency = newActLatency - msg->header->c.control2;

            // maintain best latency in recent history
            unsigned64 now = Poseidon::Foundation::getSystemTime();

            if (actLatency <= minLatency)
            {
                minLatency = actLatency;
                minLatencyTime = now;
            }
            // expire old latency slowly
            // each 8 seconds increase it slightly (102%)
            // this will allow growing from 300 ms to 450 ms in 2 minutes
            if (now - minLatencyTime > par.minLatencyUpdate)
            {
                // add 1 to avoid persisting zero latency
                // minLatency = 1ms + 1.02 * minLatency
                minLatency = (unsigned)(par.minLatencyMul * minLatency + par.minLatencyAdd);
                if ((unsigned)getAckBandwidth(par.ackWindow) < goodAckBandwidth)
                {
                    goodAckBandwidth = (unsigned)(goodAckBandwidth * par.goodAckBandFade);
                    unsigned minGood = (unsigned)(par.minBandwidth / par.safeMaxBandOverGood);
                    if (goodAckBandwidth < minGood)
                    {
                        goodAckBandwidth = minGood;
                    }
                }
                minLatencyTime = now;
                // if latency did not grow, it will get reset soon by condition below
            }

            // latency sliding average/fading:
            if (aveLatency != par.initLatency)
            {
                aveLatency = (unsigned)((1.0f - LATENCY_F) * aveLatency + LATENCY_F * actLatency);
            }
            else
            {
                aveLatency = actLatency;
            }

            // update grow state:
            float bound = par.latencyOverMul * minLatency + par.latencyOverAdd;
            if (actLatency > bound)
            { // alert bound => slow down!
                growStatePing = -2;
            }
            else
            {
                bound = par.latencyWorseMul * minLatency + par.latencyWorseAdd;
                if (actLatency > bound)
                { // "small-restrictions" mode
                    growStatePing = -1;
                }
                else
                {
                    bound = par.latencyOkMul * minLatency + par.latencyOkAdd;
                    if (actLatency > bound)
                    {
                        growStatePing = 0; // I'm just happy with actual values..
                    }
                    else
                    {
                        bound = par.latencyBestMul * minLatency + par.latencyBestAdd;
                        if (actLatency > bound)
                        {
                            growStatePing = 1; // conservative grow..
                        }
                        else
                        {
                            growStatePing = 2; // I'm optimistic
                        }
                    }
                }
            }

            // ack-timeout update:
            timeout = aveLatency * par.ackTimeoutA + par.ackTimeoutB;
            // also update the default ack-timeout for VIM messages
            if ((heartBeatGap = timeout - aveLatency - (par.ackTimeoutB >> 1)) > MAX_HEART_BEAT_GAP)
            {
                heartBeatGap = MAX_HEART_BEAT_GAP;
            }
            if (timeout > MAX_TIMEOUT)
            {
                timeout = MAX_TIMEOUT;
            }
#ifdef NET_LOG_LATENCY
#ifdef NET_LOG_BRIEF
            NetLog("Ch(%u):uRTT(%.2f,%d,%u,%u,%u,%.2f)", getChannelId(), aveLatency / 1000.0, growStatePing,
                   msg->header->ackOrigin, ((unsigned)msg->refTime) - (unsigned)origMsg->refTime,
                   msg->header->c.control2, actLatency / 1000.0);
#else
            NetLog("Channel(%u)::inputStatistics: updated latency %.2f ms (state=%2d, orig=%u, origTime=%u, now=%u, "
                   "delay=%u, lat=%.2f ms, timeout=%u ms, heart-beat=%u ms)",
                   getChannelId(), aveLatency / 1000.0, growStatePing, msg->header->ackOrigin,
                   (unsigned)origMsg->refTime, (unsigned)msg->refTime, msg->header->c.control2, actLatency / 1000.0,
                   timeout / 1000, heartBeatGap / 1000);
#endif
#endif
            origMsg->waitForLatency = false;
        }
    }

    // outgoing channel bandwidth estimation (receiver-based packet-bunch):
    if (msg->header->flags & MSG_BANDWIDTH_FLAG)
    { // rb-estimation was received
        if (rbeBandwidth)
        {
            rbeBandwidth = (unsigned)((1.0f - BANDWIDTH_F) * rbeBandwidth + BANDWIDTH_F * msg->header->c.control2);
        }
        else
        {
            rbeBandwidth = msg->header->c.control2;
        }
#ifdef NET_LOG_BANDWIDTH
#ifdef NET_LOG_BRIEF
        NetLog("Ch(%u):uRBB(%u,%u)", getChannelId(), (rbeBandwidth + 64) >> 7, (msg->header->c.control2 + 64) >> 7);
#else
        NetLog("Channel(%u)::inputStatistics: updated RB-bandwidth: %u kbps (received estimation = %u kbps)",
               getChannelId(), (rbeBandwidth + 64) >> 7, (msg->header->c.control2 + 64) >> 7);
#endif
#endif
    }

    // receiver-based bandwidth estimation (incoming):
    if ((msg->header->flags & MSG_BUNCH_FLAG) && // a secondary packet was received..
        received(ser - 1) && serI >= 0 && ser - 1 >= inputMin)
    {
        int serII = serI - 1;
        if (serII < 0)
        {
            serII = MAX_ACK_ARRAY - 1;
        }
        // filter relevant pairs dynamically!
        unsigned delta = ackTime[serI] - ackTime[serII];
        unsigned size = msg->header->length + IP_UDP_HEADER;
        unsigned sizeM = size * 500000;
        if (inRbe >> 2)
        { // normalize actual RBE into [ 0.5 * inRbe, 2 * inRbe ]
            if (sizeM < delta * (inRbe >> 2))
            {
                delta = sizeM / (inRbe >> 2);
            }
            if (sizeM > delta * inRbe)
            {
                delta = sizeM / inRbe;
            }
        }
        inDelay += delta;
        inSize += size;
#ifdef NET_LOG_BANDWIDTH
        if (delta > 200000)
        {
#ifdef NET_LOG_BRIEF
            NetLog("Ch(%u):large-delay(%u,%u,%u,%d)", getChannelId(), delta, size, inDelay, serI);
#else
            NetLog("Channel(%u)::inputStatistics: too large delay: %u us, size=%u, total=%u us, serI=%d",
                   getChannelId(), delta, size, inDelay, serI);
#endif
        }
        inCounter++;
#endif
    }

    // channel state values update:
#ifdef NET_LOG_CH_STATE
    packetsIn++;
    if (msg->header->flags & MSG_VIM_FLAG)
        packetsInVIM++;
    bytesIn += msg->header->length + IP_UDP_HEADER;
#endif

    // acknowledgement processing:
    unsigned64 ack; // ack bits need to be aligned to LSB!
    int ackLen;
    MsgSerial s = msg->header->ackOrigin;
    if (SHORT_ACK(msg->header->flags))
    {
        ack = msg->header->c.control1;
        ackLen = 32;
#ifdef NET_LOG_ACK_IN
        NetLog("Channel(%u)::inputStatistics: ack mask received - origin=%u, mask=%08x, ctrl2=%u", getChannelId(), s,
               msg->header->c.control1, msg->header->c.control2);
#endif
    }
    else
    {
        ack = msg->header->ackBitmask;
        ackLen = 64;
#ifdef NET_LOG_ACK_IN
        NetLog("Channel(%u)::inputStatistics: ack mask received - origin=%u, mask=%08x%08x", getChannelId(), s,
               msg->header->c.control2, msg->header->c.control1);
#endif
    }
    MsgSerial oldest; // the lowest acknowledged serial#
    if (s >= static_cast<unsigned>(ackLen - 1))
    { // ackLen is always positive (32 or 64)
        oldest = s - ackLen + 1;
    }
    else
    {
        oldest = 0;
    }

    newAcks = 0;  // number of newly acnowledged messages
    newBytes = 0; // total number of newly acknowledged bytes

    // s: the lowest acknowledged message number, ackLen: # of ack bits, ack: ack bits (low bits first)
    bool wasNegative = false;
    MsgSerial highest = 0; // the highest negative ack
    RefD<NetMessage> ackMsg;
    while (ack && s >= oldest)
    { // process one acknowledgement bit
        if (ack & 1L)
        { // message was acknowledged
            if (revisited.get(s, ackMsg) && ackMsg->status == nsOutputAck)
            {
                ackMsg = nullptr;
            }
            newAcknowledgement(s, ackMsg);
        }
        else if (!wasNegative)
        {
            wasNegative = true;
            highest = s;
        }
        ack >>= 1;
        s--;
    }

    // BigAckPacket processing:
    BigAckPacket* bap;
    if ((msg->header->flags & MSG_DUMMY_FLAG) && msg->getLength() >= MIN_BIG_ACK_SIZE &&
        (bap = (BigAckPacket*)msg->getData())->magic == BIG_ACK_MAGIC)
    {
        // RB-estimation of bandwidth:
        if (bap->bandWidth)
        {
            if (rbeBandwidth)
            {
                rbeBandwidth = (unsigned)((1.0f - BANDWIDTH_F) * rbeBandwidth + BANDWIDTH_F * bap->bandWidth);
            }
            else
            {
                rbeBandwidth = bap->bandWidth;
            }
#ifdef NET_LOG_BANDWIDTH
#ifdef NET_LOG_BRIEF
            NetLog("Ch(%u):uRBB(%u,%u)", getChannelId(), (rbeBandwidth + 64) >> 7, (bap->bandWidth + 64) >> 7);
#else
            NetLog("Channel(%u)::inputStatistics: updated RB-bandwidth: %u kbps (received estimation = %u kbps)",
                   getChannelId(), (rbeBandwidth + 64) >> 7, (bap->bandWidth + 64) >> 7);
#endif
#endif
        }
        // big acknowledgement bit-mask:
        int aPtr = 0;
        // ack[] is a flexible array; the loop below is otherwise bounded only by the
        // attacker-supplied oldest/newest. Cap it at the ack words actually present in
        // the packet so a crafted (oldest,newest) can't read past the buffer (N-SEC-13).
        const int availableAckWords =
            WireBounds::TrailingElementCount((int)msg->getLength(), (int)sizeof(BigAckPacket), (int)sizeof(unsigned32));
        unsigned32 lMask;
        unsigned oldest = bap->oldest;
        unsigned newest = bap->newest;
#ifdef NET_LOG_ACK_IN
        NetLog("Channel(%u)::inputStatistics: big-ack mask received - oldest=%u, newest=%u", getChannelId(), oldest,
               newest);
#endif
        while (oldest + 31 <= newest && aPtr < availableAckWords)
        { // process one 32-bit number
            lMask = bap->ack[aPtr++];
            for (i = 0; i++ < 32;)
            {
                if (lMask & 1)
                {
                    if (revisited.get(oldest, ackMsg) && ackMsg->status == nsOutputAck)
                    {
                        ackMsg = nullptr;
                    }
                    newAcknowledgement(oldest, ackMsg);
                    if (oldest > highest)
                    {
                        highest = oldest;
                    }
                }
                else
                {
                    wasNegative = true;
                }
                lMask >>= 1;
                oldest++;
            }
        }
    }

    // move negative acknowledgements from newOutputAckMask into outputAckMask:
    if (wasNegative)
    {
        s = newOutputAckMask.getFirst();
        while (s <= highest)
        {
            newOutputAckMask.off(s);
            if (s >= ackMin)
            {
                outputAckMask.on(s);
            }
            s = newOutputAckMask.getNext(s);
        }
    }

    // band-width statistics:
    if (newAcks)
    {
        unsigned dt = (unsigned)(msg->refTime - ackStatQueue[ackStatIndex]);
        if (dt < ACK_QUEUE_GRANUL)
        {
            ackStatQueue[ackStatIndex + 1] += newBytes;
        }
        else
        {
            unsigned64 total = ackStatQueue[ackStatIndex + 1];
            if ((ackStatIndex += 2) >= SLIDING_WINDOW)
            {
                ackStatIndex = 0;
            }
            ackStatQueue[ackStatIndex] = msg->refTime;
            ackStatQueue[ackStatIndex + 1] = total + newBytes;
#ifdef NET_LOG_BANDWIDTH1
            NetLog("Channel(%u)::inputStatistics: ack band-width - %u messages, %u bytes, dt=%u ms, total=%u bytes",
                   getChannelId(), newAcks, newBytes, dt / 1000, (unsigned)(total + newBytes));
#endif
        }
    }

    // check the channel:
    checkConnectivityInternal(msg->refTime);
}

void NetChannelBasic::checkConnectivity(unsigned64 now)
{
    if (!opened)
    {
        return;
    }
    enter();
    checkConnectivityInternal(now);
    leave();
}

void NetChannelBasic::checkConnectivityInternal(unsigned64 now)
// needs to be called inside NetChannelBasic::enter()
{
    NET_ERROR(!control);
    if (control)
    {
        return; // control channels need not to be checked..
    }

    if (!now)
    {
        now = Poseidon::Foundation::getSystemTime();
    }
    if (!starvation && // acknowledgement starvation
        (!recentVIMs ||
         now < lastMsgDeparture +
                   (heartBeatGap >> 1)) && // new VIM messages have arrived but no ack-carrying message had been sent
        (now < lastPingArrival + MAX_PING_GAP ||         // ping-response hasn't arrived for a long time (5 sec)
         now < lastPingDeparture + PING_TRY_INTERVAL) && // ping-request wasn't sent for a long time (3 sec)
        now < lastMsgDeparture + heartBeatGap            // any message wasn't sent for a long time (0.5 sec)
#ifdef PACKET_PAIRS
        && now < lastPairDeparture + MAX_HEART_BEAT_GAP) // packet-pair wasn't sent for a long time
#else
    )
#endif
        return;

    Ref<NetMessage> msg = urgentToSend; // try already prepared urgent message
                                        // it's time to compose a new (synthetic) packet-pair?
#ifdef PACKET_PAIRS
    bool pair = (now >= lastPairDeparture + MAX_HEART_BEAT_GAP);

    // now I'm gonna to send one or two messages (at least of size MIN_PACKET_PAIR):
    // "bandwidth in bytes/sec" / 40:
    unsigned minPacketPair = (unsigned)(getAckBandwidth(par.ackWindow) * PACKET_PAIR_COEF);
    if (minPacketPair < MIN_PACKET_PAIR)
        minPacketPair = MIN_PACKET_PAIR;
    else if (minPacketPair > MAX_PACKET_PAIR)
        minPacketPair = MAX_PACKET_PAIR;
    while (msg && pair && msg->header->length < minPacketPair)
        msg = msg->next;

    unsigned msgLen = msg ? msg->header->length : (pair ? minPacketPair : MSG_HEADER_LEN);
#else
    const unsigned msgLen = MSG_HEADER_LEN;
#endif

    if (!msg)
    { // compose the new (empty) one
        msg = NetMessagePool::pool()->newMessage(msgLen - MSG_HEADER_LEN, this);
        NET_ERROR(msg);
        msg->setLength(msgLen - MSG_HEADER_LEN);
        setBigAckMessage(msg);
        insertUrgent(msg);
    }

#ifdef PACKET_PAIRS
    // create the secondary packet of the same size:
    if (pair)
    { // but only if it hasn't been sent for a long time..
        Ref<NetMessage> msg2 = NetMessagePool::pool()->newMessage(msgLen - MSG_HEADER_LEN, this);
        NET_ERROR(msg2);
        msg2->setLength(msgLen - MSG_HEADER_LEN);
        setBigAckMessage(msg2);
        insertUrgentAfter(msg2, msg);
        lastPairDeparture = now;
    }
#endif

    if (now > lastPingDeparture + PING_TRY_INTERVAL)
    {
        // setup special "ping" request (MSG_INSTANT_FLAG):
#ifdef NET_LOG_CONNECTIVITY
#ifdef NET_LOG_BRIEF
        NetLog("Ch(%u):sInst(%.2f,%.2f,%.2f,%.2f,%u)", getChannelId(), 1.e-6 * (now - lastPingDeparture),
               1.e-6 * (now - lastPingArrival), 1.e-6 * (now - lastMsgDeparture), 1.e-6 * (now - lastMsgArrival),
               msg->getLength());
#else
        NetLog("Channel(%u)::checkConnectivity: sending MSG_INSTANT after %.3f sec (ping out=%.3f in=%.3f, any "
               "out=%.3f in=%.3f, len=%u)",
               getChannelId(), 1.e-6 * (now - lastPingArrival), 1.e-6 * (now - lastPingDeparture),
               1.e-6 * (now - lastPingArrival), 1.e-6 * (now - lastMsgDeparture), 1.e-6 * (now - lastMsgArrival),
               msg->getLength());
#endif
#endif
        msg->header->flags |= MSG_INSTANT_FLAG;
        lastPingDeparture = now;
    }
#ifdef NET_LOG_CONNECTIVITY
    else
#ifdef NET_LOG_BRIEF
        NetLog("Ch(%u):%s(%.2f,%.2f,%.2f,%.2f,%u)", getChannelId(), starvation ? "sStar" : "sHB",
               1.e-6 * (now - lastPingDeparture), 1.e-6 * (now - lastPingArrival), 1.e-6 * (now - lastMsgDeparture),
               1.e-6 * (now - lastMsgArrival), msg->getLength());
#else
        NetLog("Channel(%u)::checkConnectivity: sending %s after %.3f sec (ping out=%.3f in=%.3f, any out=%.3f "
               "in=%.3f, len=%u)",
               getChannelId(), starvation ? "STARVATION" : "HEART_BEAT", 1.e-6 * (now - lastMsgDeparture),
               1.e-6 * (now - lastPingDeparture), 1.e-6 * (now - lastPingArrival), 1.e-6 * (now - lastMsgDeparture),
               1.e-6 * (now - lastMsgArrival), msg->getLength());
#endif
#endif

    starvation = false;
    lastMsgDeparture = now;
}

bool NetChannelBasic::dropped()
{
    enter();
    unsigned64 now = Poseidon::Foundation::getSystemTime();
    bool dr = now > lastPingArrival + (MAX_PING_GAP << 1) &&
              now > lastMsgArrival + (static_cast<unsigned64>(par.dropGap * 1000000));
    leave();
#ifdef NET_LOG_CONNECTIVITY
    if (dr)
#ifdef NET_LOG_BRIEF
        NetLog("Ch(%u):drop(%.3f,%.3f)", getChannelId(), 1.e-6 * (now - lastPingArrival),
               1.e-6 * (now - lastMsgArrival));
#else
        NetLog("Channel(%u)::dropped: channel w/o traffic was dropped (last ping=%.3f, last any=%.3f)", getChannelId(),
               1.e-6 * (now - lastPingArrival), 1.e-6 * (now - lastMsgArrival));
#endif
#endif
    return dr;
}

unsigned NetChannelBasic::getOutputBandWidth(EnhancedBWInfo* data)
{
    enter();

    if (data)
    { // non-mandatory output data:
        // set current ack-bandwidth (from the last 3 records):
        data->actBW = (unsigned)getAckBandwidth(par.ackWindow);
        // long-term estimation of "good" acknowledged bandwidth:
        data->goodBW = goodAckBandwidth;
        // set the data bandwidth already sent:
        data->sentBW = (unsigned)getSentBandwidth(par.outWindow);
        // RBE-based output bandwidth:
        data->outRB = rbeBandwidth;
        // RBE-based input bandwidth:
        data->inRB = inRbe;
        // get the actual growing modes:
        data->growMode = growState;
        data->growModeLost = growStateLost;
        data->growModePing = growStatePing;
    }

    unsigned bandAck = maxBandwidth;
    unsigned band = (unsigned)(rbeBandwidth * RBE_WEIGHT + bandAck * (1.0f - RBE_WEIGHT));
    leave();

#ifdef NET_LOG_BANDWIDTH
    if (!(++getOutputBandWidthCounter & 127))
#ifdef NET_LOG_BRIEF
        NetLog("Ch(%u):gB(%u,%u,%u)", getChannelId(), (bandAck + 64) >> 7, (rbeBandwidth + 64) >> 7, (band + 64) >> 7);
#else
        NetLog("Channel(%u)::getOutputBandWidth: ackBand=%u, rbeBand=%u, band=%u kbps", getChannelId(),
               (bandAck + 64) >> 7, (rbeBandwidth + 64) >> 7, (band + 64) >> 7);
#endif
#endif
    return band;
}

unsigned NetChannelBasic::getLatency(unsigned* actLat, unsigned* minLat)
{
    if (actLat)
    {
        *actLat = actLatency;
    }
    if (minLat)
    {
        *minLat = minLatency;
    }
    return aveLatency;
}

bool NetChannelBasic::getInternalStatistics(ChannelStatistics& stat)
{
    enter();
    const unsigned64 now = Poseidon::Foundation::getSystemTime();
    RefD<NetMessage> msg;
    IteratorState it;
    unsigned64 totalAge = 0;
    unsigned64 maxAge = 0;
    stat.revisitedNo = 0;
    revisited.getFirst(it, msg);
    while (msg)
    {
        if ((msg->header->flags & MSG_VIM_FLAG) && msg->status != nsOutputAck)
        {
            // VIM messages waiting for time-out
            stat.revisitedNo++;
            unsigned64 age = now - msg->refTime;
            if (age > maxAge)
            {
                maxAge = age;
            }
            totalAge += age;
        }
        revisited.getNext(it, msg);
    }
    stat.ackTotal = ackMax - ackMin;
    stat.ackLost = outputAckMask.card();
    leave();

    stat.revisitedMaxAge = (unsigned)maxAge;
    stat.revisitedAveAge = stat.revisitedNo ? (unsigned)(totalAge / stat.revisitedNo) : 0;
    return true;
}

unsigned64 NetChannelBasic::getLastMessageArrival() const
{
    enter();
    unsigned64 result = lastMsgArrival;
    leave();
    return result;
}

void NetChannelBasic::setDelayMessage(NetMessage* request)
//  must be called inside enter()
{
    NET_ERROR(request);
    Ref<NetMessage> msg = urgentToSend; // try urgent messages first..
    while (msg && SHORT_ACK(msg->header->flags))
    {
        msg = msg->next;
    }
    if (!msg)
    { // then take the oldest common message (VIM non-urgent messages cannot be used here!)
        if (!commonToSend)
        { // at last I'll need a special empty message
            msg = NetMessagePool::pool()->newMessage(0, this);
        }
        else
        {
            msg = commonToSend; // take the oldest common message
            commonToSend = msg->next;
            msg->next = nullptr;
        }
        insertUrgent(msg);
    }
    msg->header->flags |= MSG_DELAY_FLAG;
    msg->heartBeatRequest = request->getSerial();
    msg->heartBeatTime = request->refTime;
}

void NetChannelBasic::dispatchMessage(NetMessage* msg, bool urgent)
{
    if (!opened)
    {
        return;
    }
    NET_ERROR(msg);
    NET_ERROR(msg->channel == this);
    if (msg->getLength() > maxMessageData())
    {
        return;
    }
    if (msg->header->flags & MSG_VIM_FLAG)
    {
        msg->sendTimeout = 0;
        msg->firstTime = Poseidon::Foundation::getSystemTime();
    }
    else if (msg->sendTimeout)
    { // set time origin for send-timeout
        msg->refTime = msg->firstTime = Poseidon::Foundation::getSystemTime();
    }
    if (msg->header->flags & MSG_URGENT_FLAG)
    {
        urgent = true;
    }
#ifdef NET_LOG_DISPATCH_MESSAGE
    struct sockaddr_in msg_dist;
    msg->getDistant(msg_dist);
    NetLog("Channel(%u)::dispatchMessage: msg=%u.%u.%u.%u:%u, len=%3u, flags=%04x, type=%s", getChannelId(),
           (unsigned)IP4(msg_dist), (unsigned)IP3(msg_dist), (unsigned)IP2(msg_dist), (unsigned)IP1(msg_dist),
           (unsigned)PORT(msg_dist), msg->getLength(), (unsigned)msg->getFlags(),
           urgent ? "urgent" : ((msg->header->flags & MSG_VIM_FLAG) ? "VIM" : "common"));
#endif
    enter();
    if (msg->header->flags & MSG_VIM_FLAG)
    { // remember message ordering:
        if (msg->header->flags & MSG_URGENT_FLAG)
        {
            lastUrgent = msg;
        }
        else
        {
            lastVIM = msg;
        }
    }

    // and finally insert the message into one of my queues...
    if (urgent)
    {
        insertUrgent(msg);
    }
    else if (msg->header->flags & MSG_VIM_FLAG)
    {
        insertVIM(msg);
    }
    else
    {
        insertCommon(msg);
    }
    leave();
}

NetMessage* NetChannelBasic::getLastVIM(bool urgent)
{
    return (urgent ? lastUrgent : lastVIM);
}

void NetChannelBasic::nextDispatcherStatus(DispatcherStatus* data)
{
    if (!data || data->structLen < sizeof(DispatcherStatusBasic))
    {
        return;
    }
    DispatcherStatusBasic* ds = (DispatcherStatusBasic*)data;
    ds->channels++;
    enter();
    NetMessage* ptr;
    if (urgentToSend)
    {
        ds->channelsWithUrgentMessages++;
        ptr = urgentToSend;
        do
        {
            ds->totalUrgentMessages++;
        } while ((ptr = ptr->next));
    }
    if (vimToSend)
    {
        ds->channelsWithVIMMessages++;
        ptr = vimToSend;
        do
        {
            ds->totalVIMMessages++;
        } while ((ptr = ptr->next));
    }
    leave();
}

float NetChannelBasic::getAckBandwidth(unsigned64 windowSize)
{
    int oldAckStat = ackStatIndex + 2;
    if (oldAckStat >= SLIDING_WINDOW)
    {
        oldAckStat = 0;
    }
    unsigned64 windowEdge = ackStatQueue[ackStatIndex] - windowSize;
    // the oldest item in the queue
    if (ackStatQueue[oldAckStat] > windowEdge)
    {
        return ((1.e6f * (ackStatQueue[ackStatIndex + 1] - ackStatQueue[oldAckStat + 1])) /
                (ackStatQueue[ackStatIndex] - ackStatQueue[oldAckStat]));
    }
    // oldAckStat is older (<=) than windowEdge
    int probe = oldAckStat + 2;
    if (probe >= SLIDING_WINDOW)
    {
        probe = 0;
    }
    while (ackStatQueue[probe] <= windowEdge)
    {
        oldAckStat = probe;
        if ((probe += 2) >= SLIDING_WINDOW)
        {
            probe = 0;
        }
    }
    // ackStatQueue[oldAckStat] <= windowEdge < ackStatQueue[probe]
    unsigned64 deltaProbe = ackStatQueue[ackStatIndex + 1] - ackStatQueue[probe + 1];
    unsigned64 deltaOld = ackStatQueue[ackStatIndex + 1] - ackStatQueue[oldAckStat + 1];
    // linear interpolation between [old] and [probe]
    float delta = deltaProbe + (deltaOld - deltaProbe) * (float)(ackStatQueue[probe] - windowEdge) /
                                   (float)(ackStatQueue[probe] - ackStatQueue[oldAckStat]);
    return (1e6f * delta / windowSize);
}

float NetChannelBasic::getSentBandwidth(unsigned64 windowSize, unsigned64 now)
{
    if (!now)
    {
        now = Poseidon::Foundation::getSystemTime();
    }
    int si = sendIndex;
    int oldIndex = si + 4;
    if (oldIndex >= SLIDING_WINDOW_SEND)
    {
        oldIndex -= SLIDING_WINDOW_SEND;
    }
    // find record of max 3 second age:
    while (sendQueue[si] - sendQueue[oldIndex] > windowSize)
    {
        if ((oldIndex += 2) >= SLIDING_WINDOW_SEND)
        {
            oldIndex = 0;
        }
    }
    if (oldIndex == si)
    {
        if ((oldIndex -= 2) < 0)
        {
            oldIndex = SLIDING_WINDOW_SEND - 2;
        }
    }
    unsigned64 outDt = now - sendQueue[oldIndex];
    unsigned64 outDelta = sendQueue[si + 1] - sendQueue[oldIndex + 1];
    // sent-BW = (1.e6f * outDelta) / outDt
    return (outDt ? (1.e6f * outDelta) / outDt : 0.0f);
}

bool NetChannelBasic::getPreparedMessage(void* data)
{
    if (!opened)
    {
        return false;
    }
    enter();
    bool result = false;
    if (data)
    { // dispatcher status is ready
        DispatcherStatusBasic* ds = (DispatcherStatusBasic*)data;
        if (!control)
        { // if too much data was sent in recent time, give up!
            // look at the data bandwidth already sent:
            float sentBandwidth = getSentBandwidth(par.outWindow);

            // dropouts:
            unsigned card = outputAckMask.card();
            if (ackMax - ackMin >= par.minAckHistory)
            {
                float dropoutRatio = (card + 0.0f) / (ackMax - ackMin);
                // dropout categories:
                //    2: <= par.minDropouts
                //    1: <= par.okDropouts
                //    0: <= par.midDropouts
                //   -1: <= par.maxDropouts
                //   -2: >  par.maxDropouts
                if (dropoutRatio > par.maxDropouts)
                { // dropout phase

                    // I'll try reducing maxBandwidth immediately..
                    float good =
                        (goodAckBandwidth > par.minBandwidth) ? static_cast<float>(goodAckBandwidth) : par.minBandwidth;
                    good *= par.safeMaxBandOverGood;
                    if (maxBandwidth > good)
                    {
                        maxBandwidth = (unsigned)good;
                    }

                    growStateLost = -2; // the most radical slow-down..
                }
                else if (dropoutRatio > par.midDropouts)
                {
                    growStateLost = -1; // slow-down..
                }
                else if (dropoutRatio > par.okDropouts)
                {
                    growStateLost = 0; // I'm happy (won't grow)
                }
                else if (dropoutRatio > par.minDropouts)
                {
                    growStateLost = 1; // try a little bit more..
                }
                else
                {
                    growStateLost = 2; // optimistic grow-state
                }
            }
            else
            {
                growStateLost = 2; // default: optimistic
            }

            // latency: moved to inputStatistics()

            // restrictions:
            if (sentBandwidth >= maxBandwidth)
            {
#ifdef NET_LOG_DISPATCHER
                NetLog("Channel(%u)::getPreparedMessage: refused by upper bound (max=%u,sent=%.0f)", getChannelId(),
                       maxBandwidth, (double)sentBandwidth);
#endif
                leave();
                return false;
            }
        }

#ifdef NET_LOG_DISPATCHER
        NetLog("Channel(%u)::getPreparedMessage: trying..", getChannelId());
#endif
        result = getUrgentMessage(); // urgent message will be sent anyway..
        if (!result && ds->totalUrgentMessages < URGENT_MSG_THRESHOLD)
        {
            result = getVIMMessage(); // total number of urgent messages is small enough to give a chance to VIMs..
            if (!result && ds->totalVIMMessages < VIM_MSG_THRESHOLD)
            {
                result = getCommonMessage(); // total number of VIM messages is small enough to give a chance to common
            }
            // ones..
        }
    }
    else
    { // scheduling w/o dispatcher status
        result = getUrgentMessage();
        if (!result)
        {
            result = getVIMMessage();
            if (!result)
            {
                result = getCommonMessage();
            }
        }
    }
    leave();
    return result;
}

bool NetChannelBasic::getUrgentMessage()
// must be called inside enter()
{
    if (!urgentToSend)
    {
        return false;
    }
    NET_ERROR(!prepared);
    prepared = urgentToSend;
    urgentToSend = prepared->next;
    prepared->next = nullptr;
    setOutputData(prepared);
    return true;
}

bool NetChannelBasic::getVIMMessage()
// must be called inside enter()
{
    if (!vimToSend)
    {
        return false;
    }
    NET_ERROR(!prepared);
    prepared = vimToSend;
    vimToSend = prepared->next;
    prepared->next = nullptr;
    setOutputData(prepared);
    return true;
}

bool NetChannelBasic::getCommonMessage()
// must be called inside enter()
{
    // test send-timeout of the 1st prepared message:
    while (commonToSend && commonToSend->sendTimeout &&
           Poseidon::Foundation::getSystemTime() > commonToSend->refTime + commonToSend->sendTimeout)
    {
        // send-timeout of the 1st prepared message has expired:
        Ref<NetMessage> msg = commonToSend;
        commonToSend = msg->next;
        msg->next = nullptr;
        msg->status = nsOutputObsolete;
        // call-back:
        if (msg->processRoutine && (msg->nextEvent == nsOutputSent ||    // waiting for the 1st time send
                                    msg->nextEvent == nsOutputTimeout || // waiting for re-sent
                                    msg->nextEvent == nsOutputObsolete))
        { // waiting for send-timeout
            leave();
            msg->nextEvent = (*msg->processRoutine)(msg, nsOutputObsolete, msg->dta);
            enter();
        }
        // msg will be recycled automatically after a while..
    }
    if (!commonToSend)
    {
        return false;
    }
    NET_ERROR(!prepared);
    prepared = commonToSend;
    commonToSend = prepared->next;
    prepared->next = nullptr;
    setOutputData(prepared);
    return true;
}

void NetChannelBasic::insertResend(NetMessage* msg)
// must be called inside enter()
{
    NET_ERROR(msg);
    if (!urgentToSend)
    { // message is alone..
        insertUrgent(msg);
        return;
    }
    if (msg->id < urgentToSend->id)
    { // message is the oldest one in the list..
        msg->next = urgentToSend;
        urgentToSend = msg;
        return;
    }
    Ref<NetMessage> ptr = urgentToSend;
    while (ptr->next && ptr->next->id < msg->id)
    {
        ptr = ptr->next;
    }
    insertUrgentAfter(msg, ptr);
}

void NetChannelBasic::insertUrgent(NetMessage* msg)
// must be called inside enter()
{
    if (!urgentToSend)
    {
        urgentToSend = urgentToSendEnd = msg;
    }
    else
    {
        urgentToSendEnd->next = msg;
        urgentToSendEnd = msg;
    }
    msg->next = nullptr; // to be sure
}

void NetChannelBasic::insertUrgentAfter(NetMessage* msg, NetMessage* after)
// must be called inside enter()
{
    if (!after || !urgentToSend)
    {
        insertUrgent(msg);
        return;
    }
    NetMessage* ptr = urgentToSend;
    while (ptr && ptr != after)
    {
        ptr = ptr->next;
    }
    if (!ptr)
    {
        insertUrgent(msg);
        return;
    }
    msg->next = after->next;
    after->next = msg;
    if (after == urgentToSendEnd)
    {
        urgentToSendEnd = msg;
    }
}

void NetChannelBasic::insertVIM(NetMessage* msg)
// must be called inside enter()
{
    if (!vimToSend)
    {
        vimToSend = vimToSendEnd = msg;
    }
    else
    {
        vimToSendEnd->next = msg;
        vimToSendEnd = msg;
    }
    msg->next = nullptr; // to be sure
}

void NetChannelBasic::insertCommon(NetMessage* msg)
// must be called inside enter()
{
    if (!commonToSend)
    {
        commonToSend = commonToSendEnd = msg;
    }
    else
    {
        commonToSendEnd->next = msg;
        commonToSendEnd = msg;
    }
    msg->next = nullptr; // to be sure
}

unsigned64 NetChannelBasic::preSend(unsigned64 bunchStart)
{
    // timings:
    unsigned64 previousMsgDeparture = lastMsgDeparture;
    lastMsgDeparture = Poseidon::Foundation::getSystemTime();
    if (!opened || !prepared)
    {
        return lastMsgDeparture;
    }
    prepared->refTime = lastMsgDeparture;
    // ping request:
    if (prepared->header->flags & MSG_DELAY_FLAG)
    {
        prepared->header->c.control2 = (unsigned32)(lastMsgDeparture - prepared->heartBeatTime);
    }
    // ping response:
    if (prepared->header->flags & MSG_INSTANT_FLAG)
    {
        lastPingDeparture = lastMsgDeparture;
    }
    // MSG_BUNCH_FLAG:
    if (prepared->setBunch(lastSerialSent + 1 == prepared->header->serial && bunchStart &&
                           previousMsgDeparture >= bunchStart &&
                           previousMsgDeparture >= lastMsgDeparture - MAX_PAIR_SEND_GAP))
    {
        lastPairDeparture = lastMsgDeparture;
    }
    lastSerialSent = prepared->header->serial;
    if (prepared->status == nsOutputPending)
    { // I'm sending it for the first time
        if (lastSerialSent >= ackMin)
        {
            newOutputAckMask.on(lastSerialSent);
        }
        if (lastSerialSent >= ackMax)
        {
            ackMax = lastSerialSent + 1;
        }
    }
    // channel state variables:
#ifdef NET_LOG_CH_STATE
    packetsOut++;
    if (prepared->header->flags & MSG_VIM_FLAG)
        packetsOutVIM++;
    bytesOut += prepared->header->length + IP_UDP_HEADER;
#endif
    return lastMsgDeparture;
}

void NetChannelBasic::postSend()
{
    if (!opened || !prepared)
    {
        prepared = nullptr;
        return;
    }
    // the message (prepared) has been sent before!
    revisited.put(prepared);
    // sent bandwidth:
    if (prepared->refTime - sendQueue[sendIndex] < OUT_QUEUE_GRANUL)
    {
        sendQueue[sendIndex + 1] += prepared->header->length + IP_UDP_HEADER;
    }
    else
    {
        unsigned64 total = sendQueue[sendIndex + 1];
        if ((sendIndex += 2) >= SLIDING_WINDOW_SEND)
        {
            sendIndex = 0;
        }
        sendQueue[sendIndex] = prepared->refTime;
        sendQueue[sendIndex + 1] = total + prepared->header->length + IP_UDP_HEADER;
    }
    // call-back:
    if (prepared->processRoutine && (prepared->nextEvent == nsOutputSent ||    // waiting for the 1st time send
                                     prepared->nextEvent == nsOutputTimeout || // waiting for re-sent
                                     prepared->nextEvent == nsOutputObsolete))
    { // waiting for send-timeout
        prepared->nextEvent = (*prepared->processRoutine)(prepared, prepared->status, prepared->dta);
    }
    prepared = nullptr; // end of message processing
}

unsigned64 NetChannelBasic::getMessageTime(MsgSerial ser)
{
    RefD<NetMessage> msg;
    if (!revisited.get(ser, msg))
    {
        return 0;
    }
    return msg->getTime();
}

void NetChannelBasic::setOutputData(NetMessage* msg)
// should be called inside the enter() (because of ack[])
{
    NET_ERROR(msg);
    msg->next = nullptr;
    if (msg->status == nsOutputSent || msg->status == nsOutputTimeout || msg->status == nsError)
    {                               // I'm re-sending timeouted message
        msg->ackTimeout += timeout; // consequent timeouts should be longer
        if (msg->ackTimeout > MAX_ACK_TIMEOUT)
        {
            msg->ackTimeout = MAX_ACK_TIMEOUT;
        }
        msg->canBeSecondary = false;
    }
    else
    { // 1st try: initialize all items
        NET_ERROR(msg->header->serial == MSG_SERIAL_NULL);
        msg->header->serial = serial++;
        if (serial == MSG_SERIAL_NULL)
        {
            serial++;
        }
        msg->status = nsOutputPending;
        msg->ackTimeout = timeout; // initial timeout in microseconds
        if (msg->header->flags & MSG_INSTANT_FLAG)
        {
            msg->waitForLatency = true; // this message will be remembered a little bit longer..
        }
    }

    unsigned64 now = Poseidon::Foundation::getSystemTime();
    // send receiver-based bandwidth estimation:
    if (inDelay && !SHORT_ACK(msg->header->flags) && now > nextBandwidthDeparture)
    {
        msg->header->flags |= MSG_BANDWIDTH_FLAG;
        msg->header->c.control2 = (unsigned32)((1000000 * (unsigned64)inSize) / inDelay);
        if (inRbe)
        {
            inRbe = (unsigned)((1.0f - LOCAL_BANDWIDTH_F) * inRbe + LOCAL_BANDWIDTH_F * msg->header->c.control2);
        }
        else
        {
            inRbe = msg->header->c.control2;
        }
#ifdef NET_LOG_BANDWIDTH
#ifdef NET_LOG_BRIEF
        NetLog("Ch(%u):sRBB(%.3f,%u,%u,%u,%u)", getChannelId(),
               1.e-6 * (now - nextBandwidthDeparture + BANDWIDTH_INTERVAL), (msg->header->c.control2 + 64) >> 7,
               inCounter, inSize, (unsigned)inDelay);
#else
        NetLog(
            "Channel(%u)::setOutputData: sent RB-estimation after %.3f sec: %u kbps (packets=%u, size=%u, delay=%u us)",
            getChannelId(), 1.e-6 * (now - nextBandwidthDeparture + BANDWIDTH_INTERVAL),
            (msg->header->c.control2 + 64) >> 7, inCounter, inSize, (unsigned)inDelay);
#endif
#endif
        nextBandwidthDeparture = now + BANDWIDTH_INTERVAL;
        inDelay = 0;
        inSize = 0;
#ifdef NET_LOG_BANDWIDTH
        inCounter = 0;
#endif
    }

    // determine ack-mask origin ..
    MsgSerial newest; // the newest ack-ed message = ack-origin value!
    MsgSerial oldest; // absolute serial# of the oldest ack-ed msg
    int i;
    unsigned size = SHORT_ACK(msg->header->flags) ? 31 : 63;
    // maximum allowed distance between 'oldest' and 'newest'

#ifdef NET_LOG_ACK_OUT
    bool fromOld = false;
#endif
    if ((msg->header->flags & MSG_DELAY_FLAG))
    {
        newest = msg->heartBeatRequest; // "ping" response
    }
    else // common message
        if (ack[ackPtr] == NOT_RECEIVED)
        { // nothing was received yet :C
            newest = inputMax;
        }
        else // anything needs to be acknowledged..
            if (oldAckFirst != oldAckLast)
            { // there are priority acks available
#ifdef NET_LOG_ACK_OUT
                fromOld = true;
#endif
                newest = oldAckQueue[oldAckFirst++];
                if (oldAckFirst >= MAX_OLD_ACKS)
                {
                    oldAckFirst = 0;
                }
            }
            else
            { // regular acknowledgements..
                oldest = inputMax - MAX_ACK_ARRAY;
                i = ackPtr;
                do
                {
                    oldest++;
                    if (++i >= MAX_ACK_ARRAY)
                    {
                        i = 0;
                    }
                    if (ack[i] > 0 && ack[i] != NOT_RECEIVED)
                    {
                        break;
                    }
                } while (i != ackPtr);
                // now 'oldest' contains the oldest message that must be acknowledged
                newest = oldest + size;
                if (newest > inputMax)
                {
                    newest = inputMax;
                }
            }
    oldest = (newest >= size) ? newest - size : 0;

    msg->header->ackOrigin = newest;
    // update ack[] items => mark this acknowledgement
    if (newest >= inputMin)
    {
        i = ackPtr - (inputMax - newest);
        if (i < 0)
        {
            i += MAX_ACK_ARRAY; // points into the 'ack' array
        }
        unsigned j = newest;
        while (j >= inputMin && j >= oldest)
        {
            if (ack[i] > 0 && ack[i] != NOT_RECEIVED)
            {
                ack[i]--;
            }
            if (--i < 0)
            {
                i = MAX_ACK_ARRAY - 1;
            }
            j--;
        }
    }
    // build the acknowledgement mask itself:
    unsigned64 am = 0;
    while (oldest <= newest)
    {
        am <<= 1;
        if (ackMask.get(oldest++))
        {
            am++;
        }
    }
    recentVIMs = 0;

    // .. and finally put it into message header:
    if (SHORT_ACK(msg->header->flags))
    {
        // fill c.control2 (if it's possible in this time..):
        if (msg->header->flags & MSG_ORDERED_FLAG)
        { // VIM-ordered message
            if (msg->pred)
            {
                msg->header->c.control2 = msg->pred->getSerial();
                msg->pred = nullptr;
            }
            NET_ERROR(msg->header->c.control2 != MSG_SERIAL_NULL);
        }
        // 32-bit acknowledgement:
        NET_ERROR(!(am >> 32));
        msg->header->c.control1 = (unsigned32)am;
#ifdef NET_LOG_ACK_OUT
        NetLog("Channel(%u)::setOutputData: set ack (origin=%u%c mask=%08x, starving=%u, ctrl2=%u)", getChannelId(),
               msg->header->ackOrigin, fromOld ? ';' : ',', msg->header->c.control1, inputMax - newest,
               msg->header->c.control2);
#endif
    }
    else
    {
        // 64-bit acknowledgement:
        msg->header->ackBitmask = am;
#ifdef NET_LOG_ACK_OUT
        NetLog("Channel(%u)::setOutputData: set ack (origin=%u%c mask=%08x%08x, starving=%u)", getChannelId(),
               msg->header->ackOrigin, fromOld ? ';' : ',', msg->header->c.control2, msg->header->c.control1,
               inputMax - newest);
#endif
    }
    NET_ERROR(msg->channel != nullptr);
    NET_ERROR(msg->channel == this);
}

void NetChannelBasic::setBigAckMessage(NetMessage* msg)
// should be called inside the enter() (because of ackMask)
{
    NET_ERROR(msg);
    msg->header->flags |= MSG_DUMMY_FLAG;
    unsigned len = msg->getLength();
    if (len < MIN_BIG_ACK_SIZE)
    {
        if (len)
        {
            memset(msg->getData(), 0, len);
        }
        return;
    }
    unsigned ackLen = (len - sizeof(BigAckPacket)) >> 2; // in 32-bit words
    BigAckPacket* bap = (BigAckPacket*)msg->getData();
    bap->magic = BIG_ACK_MAGIC;
    // send the bandwidth:
    if (inDelay)
    {
        bap->bandWidth = (unsigned32)((1000000 * (unsigned64)inSize) / inDelay);
        if (inRbe)
        {
            inRbe = (unsigned)((1.0f - LOCAL_BANDWIDTH_F) * inRbe + LOCAL_BANDWIDTH_F * bap->bandWidth);
        }
        else
        {
            inRbe = bap->bandWidth;
        }
        inDelay = 0;
        inSize = 0;
#ifdef NET_LOG_BANDWIDTH
        inCounter = 0;
#endif
    }
    else
    {
        bap->bandWidth = 0;
    }
    // build the big acknowledgement:
    int newest = ackMask.getLast();
    int oldest;
    unsigned actualLen = 0;
    if (newest == BitMask::END)
    {
        oldest = newest = MSG_SERIAL_NULL + 1;
        ackLen = 0;
    }
    else
    {
        oldest = ackMask.getFirst();
        actualLen = (newest - oldest + 32) & -32;
        if (actualLen > (ackLen << 5))
        {
            actualLen = ackLen << 5;
        }
        else
        {
            ackLen = actualLen >> 5;
        }
        if (newest > static_cast<int>(actualLen - 1))
        { // actualLen is unsigned, newest can be negative
            oldest = newest - actualLen + 1;
        }
        else
        {
            oldest = MSG_SERIAL_NULL + 1;
            newest = MSG_SERIAL_NULL + actualLen;
        }
    }
#ifdef NET_LOG_ACK_OUT
    NetLog("Channel(%u)::setBigAckMessage: band-width=%u bps, oldest=%d, newest=%d", getChannelId(), bap->bandWidth,
           oldest, newest);
#endif
    bap->newest = newest;
    bap->oldest = oldest;
    // compose the bit-mask itself:
    int i;
    while (ackLen)
    {
        unsigned32 mask = 0; // buffer for 32 bits (LSB .. lower serial numbers)
        for (i = 0; i++ < 32;)
        {
            mask <<= 1;
            if (ackMask.get(newest--))
            {
                mask++;
            }
        }
        bap->ack[--ackLen] = mask;
    }
}

// Run-revisited interval in microseconds.
const unsigned64 NetChannelBasic::RUN_INTERVAL = 50000;

// adjustChannel interval in microseconds.
const unsigned64 NetChannelBasic::ADJUST_INTERVAL = 1000000;

#ifdef NET_LOG_CH_STATE
const unsigned64 NetChannelBasic::CH_INFO_INTERVAL = 4000000;
#endif

void NetChannelBasic::tick()
{
    unsigned64 now = Poseidon::Foundation::getSystemTime();

    if (runTime + RUN_INTERVAL < now)
    {
        runRevisited();
    }
    if (adjustTime + ADJUST_INTERVAL < now)
    {
        adjustChannel();
    }

#ifdef NET_LOG_CH_STATE
    if (lastChannelStateLog + (control ? (CH_INFO_INTERVAL << 2) : CH_INFO_INTERVAL) < now)
    {
        unsigned delta = (unsigned)(now - lastChannelStateLog);
        unsigned band = (unsigned)(rbeBandwidth * RBE_WEIGHT + maxBandwidth * (1.0f - RBE_WEIGHT));
        NetLog("Ch(%u):st(%u,%u,%u,%u,%u,%u,%u,%d,%u/%u,%.0f,%.2f,%u/%u,%.0f,%.2f,%u,%u)", getChannelId(),
               (goodAckBandwidth + 64) >> 7, (maxBandwidth + 64) >> 7, (rbeBandwidth + 64) >> 7, (band + 64) >> 7,
               ((unsigned)getAckBandwidth(par.ackWindow) + 64) >> 7,
               ((unsigned)getSentBandwidth(par.outWindow) + 64) >> 7, (inRbe + 64) >> 7, growState,
               packetsOut - packetsOutVIM, packetsOutVIM, bytesOut * (1.e6 / 127.0) / delta,
               control ? -1.0 : 1.e-6 * (now - lastMsgDeparture), packetsIn - packetsInVIM, packetsInVIM,
               bytesIn * (1.e6 / 128.0) / delta, control ? -1.0 : 1.e-6 * (now - lastMsgArrival), revisited.card(),
               deferred.card());
        packetsOut = packetsOutVIM = bytesOut = packetsIn = packetsInVIM = bytesIn = 0;
        lastChannelStateLog = now;
    }
#endif
}

void NetChannelBasic::runRevisited()
{
    const unsigned64 now = runTime = Poseidon::Foundation::getSystemTime();
    if (!opened)
    {
        return;
    }
    enter();
    IteratorState it;
    RefD<NetMessage> msg;
#ifdef NET_LOG_RUN_REVISITED
    unsigned commonSize = 0;
    unsigned vimSize = 0;
    unsigned urgentSize = 0;
    NetMessage* tmp = commonToSend;
    while (tmp)
    {
        commonSize++;
        tmp = tmp->next;
    }
    tmp = vimToSend;
    while (tmp)
    {
        vimSize++;
        tmp = tmp->next;
    }
    tmp = urgentToSend;
    while (tmp)
    {
        urgentSize++;
        tmp = tmp->next;
    }
    NetLog("Channel(%u)::runRevisited: common=%u, vim=%u, urgent=%u, revisited=%u, deferred=%u", getChannelId(),
           commonSize, vimSize, urgentSize, revisited.card(), deferred.card());
#endif

    MsgSerial newest = 0;

    // process "lost" message statistics
    // if message is older than now - aveLatency,
    // it is probably lost and we should reduce bandwidth
    // we did not time-out it though, as we are only guessing - it may be received later
    unsigned64 lostTime = now - (unsigned64)(par.lostLatencyMul * aveLatency + par.lostLatencyAdd);
    // "150% of aveLatency + 100ms" is used
    for (revisited.getFirst(it, msg); msg; revisited.getNext(it, msg))
    {
        // check if message should be considered lost
        if (msg->refTime < lostTime)
        {
            // check if it was already confirmed as received
            // if not, mark is as lost - it is waiting too long
            int s = msg->header->serial;
            if (newOutputAckMask.get(s))
            {
                newOutputAckMask.off(s);
                if (s >= static_cast<int>(ackMin))
                {
                    outputAckMask.on(s); // ackMin is MsgSerial (unsigned), s is int
                }
            }
        }
    }

    // keep outputAckMask approximately par.outWindow long..
    int actual = outputAckMask.getFirst();
    while (actual != BitMask::END)
    { // anything was lost
        if (!revisited.get(actual, msg) || msg->refTime + par.outWindow < now)
        { // the message is too old for outputAckMask..
            outputAckMask.off(actual);
        }
        actual = outputAckMask.getNext(actual);
    }
    actual = newOutputAckMask.getFirst();
    while (actual != BitMask::END)
    { // anything was lost
        if (!revisited.get(actual, msg) || msg->refTime + par.outWindow < now)
        { // the message is too old for outputAckMask..
            newOutputAckMask.off(actual);
        }
        actual = newOutputAckMask.getNext(actual);
    }
    // update ackMin:
    actual = ackMin;
    bool shiftMin = false;
    while (actual < static_cast<int>(ackMax))
    { // ackMax is MsgSerial (unsigned), actual is int
        if (!revisited.get(actual, msg) || msg->refTime + par.outWindow < now)
        { // too old..
            ackMin = actual + 1;
            shiftMin = true;
        }
        actual++;
    }
    if (shiftMin || ackMin + par.maxOutputAckMask < ackMax)
    { // Mixed unsigned comparison OK (both positive)
        // update ackMin first:
        if (ackMin + par.maxOutputAckMask < ackMax)
        { // Mixed unsigned comparison OK (both positive)
            ackMin = ackMax - par.maxOutputAckMask;
        }
        // reset starting segment of outputAckMask & optimize it:
        actual = outputAckMask.getFirst();
        if (actual < static_cast<int>(ackMin))
        { // ackMin is MsgSerial (unsigned), actual is int
            outputAckMask.range(actual, ackMin - actual, false);
        }
        outputAckMask.growOptimize(true, ackMin);
        // reset starting segment of newOutputAckMask & optimize it:
        actual = newOutputAckMask.getFirst();
        if (actual < static_cast<int>(ackMin))
        { // ackMin is MsgSerial (unsigned), actual is int
            newOutputAckMask.range(actual, ackMin - actual, false);
        }
        newOutputAckMask.growOptimize(true, ackMin);
    }

#ifdef NET_LOG_OUTPUT_ACK_OPTIMIZE
    int min, max;
    int old1, new1;
    outputAckMask.getStat(min, max);
    old1 = outputAckMask.getFirst();
    if (old1 == BitMask::END)
        old1 = 0;
    new1 = outputAckMask.getLast();
    if (new1 == BitMask::END)
        new1 = 0;
    double lost = (ackMax > ackMin) ? outputAckMask.card() * 100.0 / (ackMax - ackMin) : 0.0;
    NetLog("Channel(%u)::runRevisited() - outputAckMask: min=%d, max=%d, old=%d, new=%d, ackMin=%d, ackMax=%d, "
           "lost=%.1f%% (%d)",
           getChannelId(), min, max, old1, new1, ackMin, ackMax, lost, outputAckMask.card());
#endif

    // process message timeout
    for (revisited.getFirst(it, msg); msg; revisited.getNext(it, msg))
    {
        if ((msg->header->flags & MSG_VIM_FLAG) && msg->status != nsOutputAck)
        {
            // VIM messages waiting for time-out
            if (msg->refTime + msg->ackTimeout < now)
            { // timeout had occured
              // re-send the message:
              // implicit: call-back (nsOutputTimeout), timeout increment, etc.
#ifdef NET_LOG_RUN_REVISITED
                NetLog("Channel(%u)::runRevisited: message (%u) is being re-send after %.3f seconds", getChannelId(),
                       msg->getSerial(), 1.e-6 * (now - msg->refTime));
#endif
                insertResend(msg);
                revisited.remove(msg);
            }
        }
        else
        {
            // not so important messages
            unsigned64 useful = msg->ackTimeout << (msg->waitForLatency ? 2 : 1);
            if (useful < par.outWindow)
            {
                useful = par.outWindow;
            }
            if (msg->refTime + useful < now)
            {
                // discard this message
                revisited.remove(msg);
                // It will be recycled automatically after a while..
            }
        }
    }
    leave();
}

void NetChannelBasic::adjustChannel()
{
    enter();
    unsigned64 now = Poseidon::Foundation::getSystemTime();
    if (!control)
    {
        int actGrowState = (growStatePing < growStateLost) ? growStatePing : growStateLost;
        if (growState > MAX_GROW_STATE)
        { // Initial optimism..
            if (actGrowState < MAX_GROW_STATE)
            {
                growState = actGrowState;
            }
        }
        else
        { // regular rules for the rest of the time..
            growState = actGrowState;
            if (growState > 0 && getSentBandwidth(par.outWindow, now) < par.minActivity)
            {
#ifdef NET_LOG_ADJUST_CHANNEL
                NetLog("Channel(%u)::adjustChannel() - restricting growState due to sentBW = %.0f bits/sec",
                       getChannelId(), getSentBandwidth(par.outWindow, now) * 8.0);
#endif
                growState = 0;
            }
        }
        NET_ERROR(growState >= -MAX_GROW_STATE && growState <= MAX_GROW_STATE + 1);
        // basic maxBandwidth progress:
        maxBandwidth = (unsigned)(maxBandwidth * par.grow[growState + MAX_GROW_STATE].mul +
                                  par.grow[growState + MAX_GROW_STATE].add);
        // global absolute maxBandwidth bounds:
        if (maxBandwidth < par.minBandwidth)
        {
            maxBandwidth = par.minBandwidth;
        }
        else if (maxBandwidth > par.maxBandwidth)
        {
            maxBandwidth = par.maxBandwidth;
        }
        // relative upper bound: check maxBandwidth vs. goodAckBandwidth
        float ackBand = getAckBandwidth(par.ackWindow);
#ifdef NET_LOG_ADJUST_CHANNEL
        NetLog("Channel(%u)::adjustChannel() - state=%2d, statePing=%2d, stateLost=%2d, sentBW = %.0f bps, ackBW = "
               "%.0f bps",
               getChannelId(), growState, growStatePing, growStateLost, getSentBandwidth(par.outWindow, now) * 8.0,
               ackBand * 8.0);
#endif
        if (ackBand > goodAckBandwidth && growState > 0)
        {
            // raise "goodAckBandwidth" to actual "ackBandwidth", but at most
            // to "maxBandwidth / par.safeMaxBandOverGood"..
            float safeGood = maxBandwidth / par.safeMaxBandOverGood;
            if (ackBand > safeGood)
            {
                ackBand = safeGood;
            }
            goodAckBandwidth = (unsigned)ackBand;
        }
        else
        {
            ackBand = static_cast<float>(goodAckBandwidth);
        }
        // now "ackBand == goodAckBandwidth" holds true..
        if (ackBand < par.minBandwidth)
        {
            ackBand = static_cast<float>(par.minBandwidth);
        }
        ackBand *= par.maxBandOverGood;
        if (maxBandwidth > ackBand)
        {
            maxBandwidth = (unsigned)ackBand;
        }
    }
    adjustTime = now;
    leave();
}

void NetChannelBasic::getOutputQueueStatistics(int& msgs, int& bytes, int& vimMsgs, int& vimBytes)
{
    msgs = bytes = vimMsgs = vimBytes = 0;
    enter();
    NetMessage* tmp = vimToSend;
    while (tmp)
    {
        vimMsgs++; // count the whole message length!
        vimBytes += tmp->header->length + IP_UDP_HEADER;
        tmp = tmp->next;
    }
    tmp = urgentToSend;
    while (tmp)
    {
        vimMsgs++;
        vimBytes += tmp->header->length + IP_UDP_HEADER;
        tmp = tmp->next;
    }
    tmp = commonToSend;
    while (tmp)
    {
        msgs++;
        bytes += tmp->header->length + IP_UDP_HEADER;
        tmp = tmp->next;
    }
    leave();
}

void NetChannelBasic::cancelAllMessages()
{
    enter();
    Ref<NetMessage> ptr;
    Ref<NetMessage> tmp;
    lastUrgent = nullptr;
    ptr = urgentToSend;
    urgentToSend = nullptr;
    while (ptr)
    {
        tmp = ptr->next;
        ptr->cancel();
        ptr = tmp;
    }
    lastVIM = nullptr;
    ptr = vimToSend;
    vimToSend = nullptr;
    while (ptr)
    {
        tmp = ptr->next;
        ptr->cancel();
        ptr = tmp;
    }
    ptr = commonToSend;
    commonToSend = nullptr;
    while (ptr)
    {
        tmp = ptr->next;
        ptr->cancel();
        ptr = tmp;
    }
    leave();
}

void NetChannelBasic::close()
{
    if (!opened)
    {
        return;
    }
    opened = false;

    // recycle working messages (deferred, vimToSend, commonToSend, revisited):
    cancelAllMessages();
    if (peer)
    {
        if (!control)
        {
            peer->unregisterChannel(this);
        }
        peer = nullptr;
    }
    enter();
    processRoutine = nullptr;
    deferred.reset();
    revisited.reset();
#ifdef NET_LOG_CHANNEL
#ifdef NET_LOG_BRIEF
    NetLog("Ch(%u):close", getChannelId());
#else
    NetLog("Channel(%u)::close is in progress", getChannelId());
#endif
#endif
    leave();
    if (NetMessagePool::pool())
    {
        NetMessagePool::pool()->garbageCollect();
    }
}

NetChannelBasic::~NetChannelBasic()
{
#if defined(NET_LOG_DESTRUCTOR) || defined(NET_LOG_CHANNEL)
    enter();
#ifdef NET_LOG_BRIEF
    NetLog("Ch(%u):~(%d,%u,%d,%d,%u)", getChannelId(), (int)opened, deferred.card(), vimToSend ? 1 : 0,
           commonToSend ? 1 : 0, revisited.card());
#else
    NetLog("Channel(%u)::~NetChannelBasic: opened=%d, deferred=%u, vimToSend=%d, commonToSend=%d, revisited=%u",
           getChannelId(), (int)opened, deferred.card(), vimToSend ? 1 : 0, commonToSend ? 1 : 0, revisited.card());
#endif
    leave();
#endif
    close();
}
