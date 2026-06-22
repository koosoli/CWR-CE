#pragma once

#include <Poseidon/Network/NetTransportProtocol.hpp>

namespace Poseidon
{

constexpr int MaxNetTransportSessions = 16;

class NetTransportSessionCatalog
{
  public:
    NetTransportSessionCatalog() = default;

    int Size() const { return _size; }
    int Add();
    void Delete(int index);
    void Clear();
    int FindByEndpoint(unsigned32 ip, unsigned16 port) const;
    bool HasRecentPortCandidate(unsigned32 ip, unsigned16 port, unsigned16 portInterval, int portsToTry,
                                unsigned32 nowSeconds, unsigned32 minRetrySeconds) const;
    void RemoveExpired(unsigned32 nowSeconds, unsigned32 maxAgeSeconds);
    void ExportSessions(AutoArray<SessionInfo>& sessions, unsigned32 nowSeconds, DWORD nowTickCount) const;
    int UpsertFromResponse(unsigned32 ip, unsigned16 port, const char* address, const SessionPacket& packet,
                           const char* mod, const char* versionTag, bool equalModRequired, unsigned32 nowSeconds,
                           unsigned pingTime);

    NetSessionDescription& operator[](int index) { return _data[index]; }
    const NetSessionDescription& operator[](int index) const { return _data[index]; }

  private:
    int _size = 0;
    NetSessionDescription _data[MaxNetTransportSessions];
};

} // namespace Poseidon
