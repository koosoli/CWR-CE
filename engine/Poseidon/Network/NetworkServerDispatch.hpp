#pragma once

#include <cstdint>
#include <Poseidon/Network/NetworkMsgFormat.hpp>

// These live at global scope (their real definitions are global); forward-declaring
// them inside Poseidon would shadow the global types and make references ambiguous
// in consumers that use namespace Poseidon.
class NetworkServer;
class NetworkMessage;
class NetworkMessageContext;

namespace Poseidon
{

// Who may originate a server-bound message. The single authorization axis the
// dispatcher enforces centrally, before any handler runs.
enum class MsgAuth : uint8_t
{
    Public,       // any connected, non-kicked client
    GameMaster,   // dedicated admin/control (restart, mission, kick, lock, shutdown)
    ServerOrigin, // only the server may originate; reject any client-sent copy
    ObjectOwner,  // sender must own the referenced object
    Unconnected,  // pre-handshake control; rate-limited, never trusted with state
};

// How a message is forwarded after it passes authorization.
enum class MsgRouting : uint8_t
{
    Local,
    RelayAll,
    RelayExceptSender,
    RelayToOwner,
};

// Validated, decoded view handed to a focused handler. The seam that lets a
// single message handler be unit-tested without standing up the transport.
struct ServerMsgCtx
{
    int from;
    NetworkMessage* msg;
    NetworkMessageType type;
    NetworkServer& server;
    NetworkMessageContext& ctx;
};

// Static per-message security/routing contract. The table of these is the
// single source of truth for "who may send this and where it goes"; adding a
// message requires choosing its auth, so the contract can't be silently omitted.
struct MsgDescriptor
{
    NetworkMessageType type;
    const char* name;
    MsgAuth auth;
    MsgRouting routing;
    void (*handle)(ServerMsgCtx&); // null while the type still falls through to the OnMessage switch
};

// O(1) lookup into the descriptor table. Returns null for out-of-range types.
const MsgDescriptor* LookupMsgDescriptor(NetworkMessageType type);

} // namespace Poseidon
