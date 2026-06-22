#include <Poseidon/Network/NetworkServerDispatch.hpp>

// The per-message authorization class — the single source of truth the central
// NetworkServer::Authorize consults. Most messages are Public (any participating
// client may originate them; the specific auth/bounds gates live at their handlers).
// The ServerOrigin set is server -> client only: the server emits these and a client
// must never originate them, so the server rejects any client-sent copy. These six
// have no server-side input handler — a client sending one was always illegitimate.

namespace Poseidon
{

static MsgAuth ClassifyAuth(NetworkMessageType type)
{
    switch (type)
    {
        case NMTPlayer:            // server confirms a client's connection
        case NMTLogout:            // server broadcasts a disconnect
        case NMTSquad:             // server distributes squad identity
        case NMTIntegrityQuestion: // server challenges a client (it handles IntegrityAnswer)
        case NMTPlayerState:       // server distributes player states
        case NMTChangeOwner:       // server announces an object-ownership change
            return MsgAuth::ServerOrigin;
        default:
            return MsgAuth::Public;
    }
}

// One descriptor row per message type, generated from the same NETWORK_MESSAGE_TYPES
// list that defines the NetworkMessageType enum, so type/format/auth stay co-located.
// auth comes from ClassifyAuth; routing/handler are placeholders (handlers still live
// in the OnMessage switch — the table currently drives authorization, not dispatch).
#define MSG_DESC_ROW(macro, cls, name, description, group) \
    {NMT##name, #name, ClassifyAuth(NMT##name), MsgRouting::Local, nullptr},

static const MsgDescriptor g_msgDescriptors[] = {NETWORK_MESSAGE_TYPES(MSG_DESC_ROW)};

#undef MSG_DESC_ROW

// The enum is `NMTNone = -1, <list...>, NMTN`, so the first listed enumerator is 0
// and NMTN is the count — the array, built from the same list, is indexed directly
// by the type value. This assert fires if the list and the enum ever drift apart.
static_assert(static_cast<int>(NMTN) == static_cast<int>(sizeof(g_msgDescriptors) / sizeof(g_msgDescriptors[0])),
              "descriptor table size must match the NetworkMessageType enum count");

const MsgDescriptor* LookupMsgDescriptor(NetworkMessageType type)
{
    if (type < 0 || type >= NMTN)
    {
        return nullptr;
    }
    return &g_msgDescriptors[static_cast<int>(type)];
}

} // namespace Poseidon
