
#include <Poseidon/Network/NetworkImpl.hpp>
#include <Poseidon/Network/NetworkMessages.hpp>

#include <cstddef>
#include <cstdint>

// libFuzzer harness for the dedicated server's wire-decode path.
//
// Target: NetworkComponent::DecodeMessage(from, raw), the server-side call below
// CRC validation. DecodeMessage reads the message type and time header, then for
// NMTMessages walks an untrusted sub-message count. Each nested DecodeMsg /
// DecodeMsgItem call can consume attacker-controlled array and string counts that
// drive Resize() and read loops. This is the parsing surface; per-message
// OnMessage handling is intentionally a no-op so the harness stays focused on the
// decoder instead of authenticated server state.
//
// Build with a fuzz preset so Release/NDEBUG, ASan, and SanitizerCoverage are all
// active. NDEBUG matches the dedicated server: PoseidonAssert and AutoArray bounds
// checks compile out, so out-of-bounds defects are visible instead of masked by
// debug assertions.

namespace
{
class FuzzComponent : public NetworkComponent
{
  public:
    FuzzComponent() : NetworkComponent(nullptr) {}

    // Match the server's wire-type validation, including the negative-type guard.
    NetworkMessageFormatBase* GetFormat(int type) override
    {
        if (type < 0 || type >= NMTN)
        {
            return nullptr;
        }
        return GMsgFormats[type];
    }

    // Parser-only harness: accept and drop decoded messages, never dispatch.
    void OnMessage(int from, NetworkMessage* msg, NetworkMessageType type) override {}
    void OnSimulate() override {}
    unsigned CleanUpMemory() override { return 0; }
    const char* GetDebugName() const override { return "fuzz"; }
    bool DXSendMsg(int to, NetworkMessageRaw& rawMsg, DWORD& msgID, NetMsgFlags dwFlags) override { return false; }
    void EnqueueMsg(int to, NetworkMessage* msg, NetworkMessageType type) override {}
    void EnqueueMsgNonGuaranteed(int to, NetworkMessage* msg, NetworkMessageType type) override {}
    // Object resolution only happens during message handling; the NDTRef decode
    // path just reads a NetworkId, so nullptr is safe for a parser-only harness.
    NetworkObject* GetObject(NetworkId& id) override { return nullptr; }

    void FuzzOne(const uint8_t* data, size_t size)
    {
        // NetworkMessageRaw's external-buffer ctor only ever reads from the buffer;
        // the const_cast is safe. from=2 mimics a connected client id.
        NetworkMessageRaw raw(reinterpret_cast<char*>(const_cast<uint8_t*>(data)), static_cast<int>(size));
        DecodeMessage(2, raw);
    }
};

FuzzComponent* g_comp = nullptr;
} // namespace

extern "C" int LLVMFuzzerInitialize(int*, char***)
{
    InitMsgFormats();
    g_comp = new FuzzComponent();
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    g_comp->FuzzOne(data, size);
    return 0;
}
