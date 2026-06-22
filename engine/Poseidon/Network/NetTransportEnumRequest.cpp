#include <Poseidon/Network/NetTransportEnumRequest.hpp>
#include <stddef.h>

namespace Poseidon
{

bool NetTransportNeedsEnumRequest(const NetTransportSessionCatalog& sessions, unsigned32 ip, unsigned16 port,
                                  unsigned32 now)
{
    return !sessions.HasRecentPortCandidate(ip, port, NetTransportEnumPortInterval, NetTransportEnumPortsToTry, now,
                                            NetTransportMinEnumRetryMs);
}

NetTransportEnumPortList BuildNetTransportEnumPortList(unsigned16 firstPort)
{
    NetTransportEnumPortList ports{};
    for (int i = 0; i < NetTransportEnumPortsToTry; ++i)
    {
        ports[static_cast<size_t>(i)] = firstPort + i * NetTransportEnumPortInterval;
    }
    return ports;
}

EnumPacket BuildNetTransportEnumRequest(int32 magicApp)
{
    EnumPacket request{};
    request.magic = MAGIC_ENUM_REQUEST;
    request.magicApplication = magicApp;
    return request;
}

} // namespace Poseidon
