#pragma once

#include <array>

#include <Poseidon/Network/NetTransportProtocol.hpp>
#include <Poseidon/Network/NetTransportSessionCatalog.hpp>

namespace Poseidon
{

constexpr int NetTransportEnumPortsToTry = 15;
constexpr unsigned16 NetTransportEnumPortInterval = 12;
constexpr unsigned32 NetTransportMinEnumRetryMs = 4000;

using NetTransportEnumPortList = std::array<unsigned16, NetTransportEnumPortsToTry>;

bool NetTransportNeedsEnumRequest(const NetTransportSessionCatalog& sessions, unsigned32 ip, unsigned16 port,
                                  unsigned32 now);
NetTransportEnumPortList BuildNetTransportEnumPortList(unsigned16 firstPort);
EnumPacket BuildNetTransportEnumRequest(int32 magicApp);

} // namespace Poseidon
