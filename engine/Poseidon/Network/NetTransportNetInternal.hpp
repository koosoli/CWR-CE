#pragma once

class NetPeer;

namespace Poseidon
{

// Accessors over the NetTransportNet peer/pool state (the backing statics live in
// NetTransportNet.cpp). Exposed so cohesive sections — e.g. the session enumerator —
// can compile as their own translation units without duplicating that state.
namespace NetTpInternal
{
NetPeer* getClientPeer();
void checkPool();
} // namespace NetTpInternal

} // namespace Poseidon
