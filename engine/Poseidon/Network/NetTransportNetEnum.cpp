#include <Poseidon/Network/NetTransportNetDecls.hpp>
#ifndef _WIN32
#include <arpa/inet.h>
#endif
#ifndef _WIN32
#include <netinet/in.h>
#endif
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <functional>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>
#include <Poseidon/Foundation/Common/Global.hpp>
#include <Poseidon/Foundation/Common/NetGlobal.hpp>
#include <Poseidon/Foundation/Containers/Array.hpp>
#include <Poseidon/Foundation/Containers/Bitmask.hpp>
#include <Poseidon/Foundation/Containers/Maps.hpp>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Poseidon/Foundation/Framework/Log.hpp>
#include <Poseidon/Foundation/Framework/PoTime.hpp>
#include <Poseidon/Foundation/Memory/CheckMem.hpp>
#include <Poseidon/Foundation/Memory/MemAlloc.hpp>
#include <Poseidon/Foundation/Strings/RString.hpp>
#include <Poseidon/Foundation/Strings/StrFormat.hpp>
#include <Poseidon/Foundation/Threads/PoCritical.hpp>
#include <Poseidon/Foundation/Types/Pointers.hpp>
#include <Poseidon/Foundation/platform.hpp>
#include <Poseidon/Network/NetTransportNetInternal.hpp>

// Defined in NetTransportNet.cpp.

namespace Poseidon
{

void decodeURLAddress(RString address, RString& ip, int& port);
void setEnum(NetSessionEnum* _en);

NetSessionEnum::NetSessionEnum() : LockInit(_cs, "NetSessionEnum::_cs", true)
{
    _running = false;
    magicApp = 0;
    setEnum(this);
#ifdef NET_LOG_SESSION_ENUM
    NetLog("Peer(%u): creating NetSessionEnum instance", NetTpInternal::getClientPeer()->getPeerId());
#endif
}

NetSessionEnum::~NetSessionEnum()
{
    setEnum(nullptr);
    Done();
#ifdef NET_LOG_SESSION_ENUM
    NetLog("Peer(%u): destroying NetSessionEnum instance [%08x]", NetTpInternal::getClientPeer()->getPeerId());
#endif
    NetTpInternal::checkPool();
}

RString NetSessionEnum::IPToGUID(RString ip, int port)
{
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "%s:%d", (const char*)ip, port);
    return buffer;
}

bool NetSessionEnum::Init(int magic)
{
#ifdef NET_LOG_SESSION_ENUM
    NetLog("Peer(%u): NetSessionEnum::Init(%d)", NetTpInternal::getClientPeer()->getPeerId(), magic);
#endif
    magicApp = magic;
    setEnum(this);
    return true;
}

void NetSessionEnum::Done()
{
    enter();
    StopEnumHosts();
    _sessions.Clear();
    leave();
}

bool NetSessionEnum::needsRequest(struct sockaddr_in& addr, unsigned16 port)
{
    bool needs = true;
    enter();
    unsigned32 ip = ntohl(addr.sin_addr.s_addr);
    for (int i = 0; i < _sessions.Size(); i++)
    {
        const NetSessionDescription& src = _sessions[i];
        if (src.ip == ip && src.port >= port && src.port < port + NUM_PORTS_TO_TRY * PORT_INTERVAL &&
            (src.port - port) % PORT_INTERVAL == 0)
        {
            unsigned32 now = (unsigned32)(getSystemTime() / 1000);
            if ((now - src.lastTime) < MIN_ENUM_RETRY)
            {
                needs = false;
            }
            break;
        }
    }
    leave();
    return needs;
}

void NetSessionEnum::sendRequest(NetChannel* br, struct sockaddr_in& addr, unsigned16 port)
{
    NET_ERROR(br);
    unsigned16 p;
    EnumPacket request;
    request.magic = MAGIC_ENUM_REQUEST;
    request.magicApplication = magicApp;
    int i;

    for (p = port, i = 0; i++ < NUM_PORTS_TO_TRY; p += PORT_INTERVAL)
    {
        addr.sin_port = htons(p);
        Ref<NetMessage> msg = NetMessagePool::pool()->newMessage(sizeof(EnumPacket), br);
        if (msg)
        {
            msg->setDistant(addr);
            msg->setFlags(MSG_ALL_FLAGS, MSG_TO_BCAST_FLAG | MSG_MAGIC_FLAG);
            msg->setData((unsigned8*)&request, sizeof(EnumPacket));
            msg->send();
        }
    }
}

bool NetSessionEnum::StartEnumHosts(RString ip, int port, AutoArray<RemoteHostAddress>* hosts)
{ // don't need to lock this..
    _running = true;
    bool anything = false;
    setEnum(this);
#ifdef NET_LOG_START_ENUM
    NetLog("Peer(%u): NetSessionEnum::StartEnumHosts(%s,%u,%d)", NetTpInternal::getClientPeer()->getPeerId(),
           (const char*)ip, (unsigned)port, hosts ? hosts->Size() : 0);
#endif
    NetPeer* peer = NetTpInternal::getClientPeer();
    if (!peer)
    {
        _running = false;
        return false;
    }
    NetChannel* br = peer->getBroadcastChannel();
    NET_ERROR(br);
    struct sockaddr_in addr;
    RString ipaddr;

    if (!ip.GetLength())
    { // if "ip" is not defined, try LAN broadcast first ..
        getHostAddress(addr, nullptr, port);
        if (needsRequest(addr, port))
        {
            sendRequest(br, addr, port); // broadcast
            anything = true;
        }
        getLocalAddress(addr, port);
        if (needsRequest(addr, port))
        {
            sendRequest(br, addr, port); // localhost (for XP)
            anything = true;
        }
    }
    else
    {
        decodeURLAddress(ip, ipaddr, port);
        if (getHostAddress(addr, ipaddr, port) && // .. else try explicit IP address
            needsRequest(addr, port))
        {
            sendRequest(br, addr, port);
            anything = true;
        }
    }

    int i;
    if (hosts)
    { // .. and finally try all addresses from the given list
        for (i = 0; _running && i < hosts->Size(); i++)
        {
            const RemoteHostAddress& address = hosts->Get(i);
            port = address.port;
            decodeURLAddress(address.ip, ipaddr, port);
            if (getHostAddress(addr, ipaddr, port) && needsRequest(addr, port))
            {
                sendRequest(br, addr, port);
                anything = true;
            }
        }
    }

    return anything;
}

void NetSessionEnum::StopEnumHosts()
{
#ifdef NET_LOG_STOP_ENUM
    NetLog("Peer(%u): NetSessionEnum::StopEnumHosts", NetTpInternal::getClientPeer()->getPeerId());
#endif
    _running = false;
}

int NetSessionEnum::NSessions()
{
    enter();
    int n = _sessions.Size();
    leave();
#ifdef NET_LOG_NSESSIONS
    NetLog("Peer(%u): NetSessionEnum::NSessions: N=%d", NetTpInternal::getClientPeer()->getPeerId(), n);
#endif
    return n;
}

void NetSessionEnum::GetSessions(AutoArray<SessionInfo>& sessions)
{
    enter();

    int i;
    int n = _sessions.Size();
    for (i = 0; i < n;)
    {
        const NetSessionDescription& desc = _sessions[i];
        unsigned32 age = (unsigned32)(getSystemTime() / 1000) - desc.lastTime;
        if (age > MAX_ENUM_AGE)
        { // message age in milliseconds
            _sessions.Delete(i);
            n--;
        }
        else
        {
            i++;
        }
    }

    sessions.Resize(n);
    for (i = 0; i < n; i++)
    {
        const NetSessionDescription& src = _sessions[i];
        SessionInfo& dst = sessions[i];
        dst.guid = src.address;
        dst.name = src.name;
        dst.lastTime = GetTickCount() - ((unsigned32)(getSystemTime() / 1000) - src.lastTime);
        dst.password = (src.password != 0);
        dst.actualVersion = src.actualVersion;
        dst.requiredVersion = src.requiredVersion;
        dst.badActualVersion = false;
        dst.badRequiredVersion = false;
        dst.gameState = src.gameState;
        dst.mission = src.mission;
        dst.numPlayers = src.numPlayers;
        dst.maxPlayers = src.maxPlayers;
        dst.ping = src.pingTime;
        dst.timeleft = 0;

        dst.mod = src.mod;
        dst.versionTag = src.versionTag;
        dst.equalModRequired = (src.equalModRequired & 1) != 0;
        dst.badMod = false;
    }

    leave();
}

} // namespace Poseidon
