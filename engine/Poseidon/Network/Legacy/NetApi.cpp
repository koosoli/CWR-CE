#include <Poseidon/Network/Legacy/netpch.hpp>
#include <Poseidon/Network/Legacy/NetPeer.hpp>
#include <Poseidon/Network/Legacy/NetChannel.hpp>
#ifndef _WIN32
#include <netinet/in.h>
#endif
#include <Poseidon/Foundation/Common/Global.hpp>
#include <Poseidon/Foundation/Common/NetGlobal.hpp>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Poseidon/Foundation/Types/Pointers.hpp>

unsigned64 channelKey(RefD<NetChannel>& ch)
{
    struct sockaddr_in addr;
    ch->getDistantAddress(addr);
    return sockaddrKey(addr);
}

// Static members for the ImplicitMap template specializations.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wglobal-constructors"
#pragma clang diagnostic ignored "-Wexit-time-destructors"

RefD<NetChannel> ImplicitMapTraits<RefD<NetChannel>>::zombie = (NetChannel*)1;
RefD<NetChannel> ImplicitMapTraits<RefD<NetChannel>>::null;

unsigned16 netPeerToPort(RefD<NetPeer>& peer)
{
    if (peer.IsNull())
    {
        return 0;
    }
    return peer->getPort();
}

RefD<NetPeer> ImplicitMapTraits<RefD<NetPeer>>::zombie = (NetPeer*)1;
RefD<NetPeer> ImplicitMapTraits<RefD<NetPeer>>::null;

#pragma clang diagnostic pop

#ifdef NET_LOG

// buf must hold at least 256 chars.
void NetworkParams::printParams1(char* buf)
{
    NET_ERROR(buf);
    snprintf(buf, sizeof(buf), "%.1f,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%.3f,%.1f,%.3f,%u,%u,%u",
             (double)winsockVersion, rcvBufSize, maxPacketSize, dropGap, ackTimeoutA, ackTimeoutB, ackRedundancy,
             initBandwidth, minBandwidth, maxBandwidth, minActivity, initLatency, minLatencyUpdate,
             (double)minLatencyMul, (double)minLatencyAdd, (double)goodAckBandFade, outWindow, ackWindow,
             maxChannelBitMask);
}

// buf must hold at least 256 chars.
void NetworkParams::printParams2(char* buf)
{
    NET_ERROR(buf);
    snprintf(buf, sizeof(buf),
             "%.2f,%.1f,%u,%u,%.4f,%.4f,%.4f,%.4f,%.2f,%.1f,%.2f,%.1f,%.2f,%.1f,%.2f,%.1f"
             ",%.2f,%.2f",
             (double)lostLatencyMul, (double)lostLatencyAdd, maxOutputAckMask, minAckHistory, (double)maxDropouts,
             (double)midDropouts, (double)okDropouts, (double)minDropouts, (double)latencyOverMul,
             (double)latencyOverAdd, (double)latencyWorseMul, (double)latencyWorseAdd, (double)latencyOkMul,
             (double)latencyOkAdd, (double)latencyBestMul, (double)latencyBestAdd, (double)maxBandOverGood,
             (double)safeMaxBandOverGood);
}

// buf must hold at least 256 chars.
void NetworkParams::printParams3(char* buf)
{
    NET_ERROR(buf);
    int i;
    for (i = 0; i < sizeof(grow) / sizeof(GrowCoefs); i++)
        buf += snprintf(buf, sizeof(buf), " [%.3f,%.1f]", (double)grow[i].mul, (double)grow[i].add);
}

#endif

#ifdef NET_LOG
unsigned NetChannel::nextId = 0;
#endif

NetChannel::NetChannel()
{
    timeout = 1000000; // 1 second; refined after the first ping
    processRoutine = nullptr;
    data = nullptr;
#ifdef NET_LOG
    id = nextId++;
#endif
}

#ifdef NET_LOG

char* NetChannel::getChannelInfo(char* buf) const
{
    NET_ERROR(buf);
    struct sockaddr_in local;
    if (peer)
        peer->getLocalAddress(local);
    else
        Zero(local);
    struct sockaddr_in distant;
    getDistantAddress(distant);
    snprintf(buf, sizeof(buf), "%u.%u.%u.%u:%u <-> %u.%u.%u.%u:%u", (unsigned)IP4(local), (unsigned)IP3(local),
             (unsigned)IP2(local), (unsigned)IP1(local), (unsigned)PORT(local), (unsigned)IP4(distant),
             (unsigned)IP3(distant), (unsigned)IP2(distant), (unsigned)IP1(distant), (unsigned)PORT(distant));
    return buf;
}

unsigned NetChannel::getChannelId() const
{
    return id;
}

#endif

unsigned NetChannel::maxMessageData() const
{
    return peer->maxMessageData();
}

bool NetChannel::isControl() const
{
    return control;
}

NetPeer* NetChannel::getPeer() const
{
    return peer;
}

NetPool* NetChannel::getPool() const
{
    NET_ERROR(peer);
    return (peer->getPool());
}

bool NetChannel::getInternalStatistics(ChannelStatistics& /*stat*/)
{
    return false;
}

void NetChannel::setProcessRoutine(NetCallBack* processR, void* _data)
{
    processRoutine = processR;
    data = _data;
}

NetChannel::~NetChannel()
{
#ifdef NET_LOG_DESTRUCTOR
    NetLog("Channel(%u)::~NetChannel", getChannelId());
#endif
}

#ifdef NET_LOG
unsigned NetPeer::nextId = 0;
#endif

NetPeer::NetPeer(NetPool* _pool)
{
    pool = _pool;
    port = 0;
    broadcastCh = nullptr;
#ifdef NET_LOG
    id = nextId++;
#endif
}

#ifdef NET_LOG

char* NetPeer::getPeerInfo(char* buf) const
{
    NET_ERROR(buf);
    struct sockaddr_in local;
    getLocalAddress(local);
    snprintf(buf, sizeof(buf), "%u.%u.%u.%u:%u", (unsigned)IP4(local), (unsigned)IP3(local), (unsigned)IP2(local),
             (unsigned)IP1(local), (unsigned)PORT(local));
    return buf;
}

unsigned NetPeer::getPeerId() const
{
    return id;
}

#endif

NetPool* NetPeer::getPool() const
{
    return pool;
}

unsigned16 NetPeer::getPort() const
{
    return port;
}

NetChannel* NetPeer::getBroadcastChannel() const
{
    return broadcastCh;
}

unsigned NetPeer::maxMessageData() const
{
    // max UDP datagram payload minus the whole header overhead
    return (1490 - IP_UDP_HEADER - MSG_HEADER_LEN);
}

unsigned16 NetPeer::getLocalPort() const
{
    return port;
}

NetPeer::~NetPeer()
{
    port = 0;
}

#ifdef NET_STRESS

RandomJames stressRnd;

#endif
