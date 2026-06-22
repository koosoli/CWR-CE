#include <Poseidon/Network/Legacy/SocketUtil.hpp>
#include <Poseidon/Network/Legacy/NetApi.hpp>
#include <Poseidon/Foundation/Containers/Bitmask.hpp>
#ifdef _MSC_VER
#pragma once
#endif

#ifndef _PEERFACTORY_H
#define _PEERFACTORY_H

// Creates UDP net-peers and net-channels.
class PeerChannelFactoryUDP : public PeerChannelFactory
{
  protected:
    static int instances; // live instance count (for WSAStartup/WSACleanup)

  public:
    PeerChannelFactoryUDP();

    // tryPorts are the local ports to try (nullptr => pool->getLocalPorts()). nullptr on failure.
    NetPeer* createPeer(NetPool* pool, BitMask* tryPorts) override;

    // control selects a control channel. nullptr on failure.
    NetChannel* createChannel(NetPool* pool, bool control = false) override;

    ~PeerChannelFactoryUDP() override;
};

#endif
