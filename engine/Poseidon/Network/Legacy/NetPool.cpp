#include <Poseidon/Network/Legacy/netpch.hpp>
#include <Poseidon/Network/Legacy/NetPeer.hpp>
#include <Poseidon/Network/Legacy/NetChannel.hpp>
#include <Poseidon/Foundation/Common/Global.hpp>
#include <Poseidon/Foundation/Common/NetGlobal.hpp>
#include <Poseidon/Foundation/Containers/Bitmask.hpp>
#include <Poseidon/Foundation/Containers/Maps.hpp>
#include <Poseidon/Foundation/Memory/MemAlloc.hpp>
#include <Poseidon/Foundation/Threads/PoCritical.hpp>
#include <Poseidon/Foundation/Types/Pointers.hpp>

using Poseidon::Foundation::IteratorState;
using Poseidon::Foundation::nsOK;

NetPool::NetPool(PeerChannelFactory* fact, NetNegotiator* neg, BitMask* ports)
{
    LockRegister(lock, "NetPool");
#ifdef NET_LOG_DESTRUCTOR
    NetLog("NetPool[%08x]: neg=%x, ports=%d", (unsigned)this, (unsigned)neg, ports ? ports->card() : 0);
#endif
    factory = fact;
    if (ports)
    {
        localPorts = new BitMask(*ports);
    }
    if (neg)
    {                         // initial network negotiation
        neg->negotiate(this); // creates the first NetChannel (including its control-channel),
                              // establishes connection to neighbour computers (server[s] [and clients])
    }
}

BitMask* NetPool::getLocalPorts() const
{
    return localPorts;
}

PeerChannelFactory* NetPool::getFactory() const
{
    return factory;
}

NetPeer* NetPool::createPeer(BitMask* tryPorts)
{
    if (factory.IsNull())
    {
        return nullptr;
    }
    // tryPorts has higher priority than localPorts
    NetPeer* newPeer = factory->createPeer(this, tryPorts);
    if (!newPeer)
    {
        return nullptr;
    }
    peers.put(newPeer);
    return newPeer;
}

NetPeer* NetPool::findPeer(unsigned16 localPort)
{
    RefD<NetPeer> result;
    peers.get(localPort, result);
    return result;
}

void NetPool::deletePeer(NetPeer* peer)
{
    if (!peer)
    {
        return;
    }
    peer->close();
    peers.remove(peer);
}

NetChannel* NetPool::createChannel(struct sockaddr_in& distant, NetPeer* peer)
{
    IteratorState it;
    RefD<NetPeer> pee = peer;
    if (!pee)
    {
        peers.getFirst(it, pee); // Note: load-balancing
    }
    if (!pee || factory.IsNull())
    {
        return nullptr;
    }
    NetChannel* ch = factory->createChannel(this, false);
    if (!ch)
    {
        return nullptr;
    }
    if (ch->open(pee, distant) != nsOK)
    {
        delete ch;
        return nullptr;
    }
    channels.put(ch);
    return ch;
}

NetChannel* NetPool::findChannel(struct sockaddr_in& distant)
{
    RefD<NetChannel> result;
    channels.get(sockaddrKey(distant), result);
    return result;
}

NetChannel* NetPool::getBroadcastChannel()
{
    IteratorState it;
    RefD<NetPeer> pe;
    if (!peers.getFirst(it, pe))
    {
        return nullptr;
    }
    return pe->getBroadcastChannel();
}

void NetPool::deleteChannel(NetChannel* channel)
{
    if (!channel)
    {
        return;
    }
    channel->close();
    channels.remove(channel);
}

NetPool::~NetPool()
{
#ifdef NET_LOG_DESTRUCTOR
    NetLog("~NetPool[%08x]: peers=%u, channels=%u", (unsigned)this, peers.card(), channels.card());
#endif
}
