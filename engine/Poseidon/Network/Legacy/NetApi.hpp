#include <Poseidon/Network/Legacy/SocketUtil.hpp>
#include <Poseidon/Foundation/Containers/Bitmask.hpp>
#include <Poseidon/Foundation/Containers/Maps.hpp>
#include <Poseidon/Foundation/Threads/PoThread.hpp>
#include <Poseidon/Foundation/Common/NetGlobal.hpp>
#ifdef _MSC_VER
#pragma once
#endif

#ifndef _NETAPI_H
#define _NETAPI_H

// Further parts of the network API live in NetGlobal.hpp and NetMessage.hpp.

class NetPeer;
class NetChannel;
class NetPool;
class NetMessage;

// Enhanced info returned by NetChannel::getOutputBandWidth().
struct EnhancedBWInfo
{
    unsigned actBW;   // actual (acknowledged) bandwidth in bytes per second
    unsigned goodBW;  // goodAckBandwidth
    unsigned sentBW;  // output bandwidth in bytes per second
    unsigned outRB;   // bandwidth from received packet-pairs (averaged value sent from the receiver)
    unsigned inRB;    // bandwidth from received packet-pairs (actual value)
    int growMode;     // 0 = steady, -2 = slow down, 2 = growing fast
    int growModeLost; // grow-mode from lost-packet ratio
    int growModePing; // grow-mode from actual ping
};

struct DispatcherStatus
{
    unsigned structLen; // length of the structure
};

struct ChannelStatistics
{
    unsigned revisitedNo;     // number of revisited messages
    unsigned revisitedAveAge; // average age of revisited messages
    unsigned revisitedMaxAge; // maximum age of revisited messages
    unsigned ackTotal;        // total acknowledged messages (approx. window size: 400)
    unsigned ackLost;         // messages considered "lost"
};

// Constants for the maxBandwidth generator. Recomputed roughly every 1000ms.
struct GrowCoefs
{
    float mul;
    float add;
};

// Grow states range over -MAX_GROW_STATE .. 0 .. MAX_GROW_STATE+1.
#define MAX_GROW_STATE 2

// Socket tuning parameters.
struct NetworkParams
{
    float winsockVersion;       // WSAStartup required WinSock version
    unsigned rcvBufSize;        // SO_RCVBUF socket parameter
    unsigned maxPacketSize;     // maximum (gross) packet size over the net
    unsigned dropGap;           // channel-drop interval in seconds
    unsigned ackTimeoutA;       // multiplicative coefficient for ack-timeout
    unsigned ackTimeoutB;       // additive coefficient for ack-timeout (microseconds)
    unsigned ackRedundancy;     // repeat count for each VIM acknowledgement
    unsigned initBandwidth;     // initial bandwidth in bytes/sec
    unsigned minBandwidth;      // maxBandwidth never drops below this (bytes/sec)
    unsigned maxBandwidth;      // maxBandwidth never rises above this (bytes/sec)
    unsigned minActivity;       // minimum channel traffic for maxBandwidth growth
    unsigned initLatency;       // initial latency in microseconds
    unsigned minLatencyUpdate;  // update interval for minLatency
    float minLatencyMul;        // multiplicative coefficient for periodic minLatency update
    float minLatencyAdd;        // additive coefficient for periodic minLatency update (microseconds)
    float goodAckBandFade;      // fading coefficient for goodAckBandwidth
    unsigned outWindow;         // sent-bandwidth computation window (microseconds)
    unsigned ackWindow;         // window for ack-based bandwidth estimation (microseconds)
    unsigned maxChannelBitMask; // maximum size of internal NetChannel bit-masks
    float lostLatencyMul;       // message-lost threshold (multiplicative coef. for aveLatency)
    float lostLatencyAdd;       // message-lost threshold (additive coef. in microseconds)
    unsigned maxOutputAckMask;  // maximum size of outputAckMask (also time-restricted via outWindow)
    unsigned minAckHistory;     // minimum ack-history size (messages) for dropout restrictions
    float maxDropouts;          // maximum packet-dropout ratio (most radical slow-down)
    float midDropouts;          // packet-dropout ratio for slow-down
    float okDropouts;           // early warning => leave the optimistic grow-mode
    float minDropouts;          // dropout ratio treated as noise
    float latencyOverMul;       // multiplicative upper bound of latency over minLatency
    float latencyOverAdd;       // additive upper bound of latency over minLatency
    float latencyWorseMul;      // "small restrictions" upper bound (multiplicative)
    float latencyWorseAdd;      // "small restrictions" upper bound (additive)
    float latencyOkMul;         // "OK" state upper bound (multiplicative)
    float latencyOkAdd;         // "OK" state upper bound (additive)
    float latencyBestMul;       // best grow-state upper bound (multiplicative)
    float latencyBestAdd;       // best grow-state upper bound (additive)
    float maxBandOverGood;      // maximum maxBandwidth / goodAckBandwidth ratio
    float safeMaxBandOverGood;  // safe over-estimation of maxBandwidth over goodAckBandwidth (dropout alert)

    // Grow-states for periodic maxBandwidth update (0 = constant, <0 = slow-down, >0 = speed-up).
    GrowCoefs grow[2 * MAX_GROW_STATE + 2];

#ifdef NET_LOG
    // Each printParamsN prints one third of the parameters; buf must hold at least 256 chars.
    void printParams1(char* buf);
    void printParams2(char* buf);
    void printParams3(char* buf);
#endif
};

extern const NetworkParams defaultNetworkParams;

// Net-channel: handles one peer-to-peer link [localIP:port]-[distantIP:port].
class NetChannel : public Poseidon::Foundation::RefCountSafe
{
  protected:
    NetPeer* peer;

    // Incoming-message handler. Must be set before any data is received.
    NetCallBack* processRoutine;

    void* data; // context passed to processRoutine

    unsigned timeout; // ack timeout in microseconds (VIM only)

    bool control;

    // Non-public default constructor; does no init, use open() to set up a connection.
    NetChannel();

#ifdef NET_LOG
    static unsigned nextId;
    unsigned id;
#endif

  public:
    // Establish the connection. May negotiate with the distant peer (small data exchange);
    // blocks. Very limited use on client computers. distant uses network endian.
    // Returns nsError or nsOK.
    virtual NetStatus open(NetPeer* _peer, struct sockaddr_in& distant) = 0;

    // Reconnect to the given address, keeping channel state. distant uses network endian.
    virtual NetStatus reconnect(struct sockaddr_in& distant) = 0;

    virtual unsigned maxMessageData() const;

    // Retrieve the associated distant address (network endian).
    virtual void getDistantAddress(struct sockaddr_in& distant) const = 0;

#ifdef NET_LOG
    // Print verbose channel info, e.g. "localIP:port<->distantIP:port".
    virtual char* getChannelInfo(char* buf) const;
    unsigned getChannelId() const;
#endif

    virtual bool isControl() const;
    virtual NetPeer* getPeer() const;
    virtual NetPool* getPool() const;

    virtual void setNetworkParams(const NetworkParams& /*p*/) {}

    // Channel latency in microseconds (0 if not determined yet); optionally returns
    // actual and minimal latency through the out-params.
    virtual unsigned getLatency(unsigned* actLat = nullptr, unsigned* minLat = nullptr) = 0;

    // Output bandwidth (throughput) in bytes/sec; fills the optional info record.
    virtual unsigned getOutputBandWidth(EnhancedBWInfo* data = nullptr) = 0;

    // Some internal stats; not all items are meaningful in every implementation.
    // Returns true if at least some items were filled.
    virtual bool getInternalStatistics(ChannelStatistics& stat);

    // Last time any message was received (getSystemTime() format).
    virtual unsigned64 getLastMessageArrival() const = 0;

    // Output-queue stats for not-yet-sent messages (common and VIM).
    virtual void getOutputQueueStatistics(int& msgs, int& bytes, int& vimMsgs, int& vimBytes) = 0;

    // Process incoming data; called asynchronously by the listener thread.
    // hdr carries the message length then the data; distant uses network endian.
    virtual void processData(MsgHeader* hdr, const struct sockaddr_in& distant) = 0;

    // Set the incoming-data callback. processR may be nullptr for a dummy input channel.
    virtual void setProcessRoutine(NetCallBack* processR, void* _data = nullptr);

    virtual void dispatchMessage(NetMessage* msg, bool urgent = false) = 0;

    // Message prepared to send. Set by getUrgentMessage() / getCommonMessage().
    Poseidon::Foundation::Ref<NetMessage> prepared;

    virtual NetMessage* getLastVIM(bool urgent) = 0;

    virtual void nextDispatcherStatus(DispatcherStatus* data) = 0;

    // Prepare the next send-queue message into 'prepared'. data is the collected dispatcher
    // status of all active channels (incl. broadcast). Returns true if 'prepared' is ready.
    virtual bool getPreparedMessage(void* data) = 0;

    // Called just before 'prepared' is sent. bunchStart is the send-bunch start time
    // (0 starts a new bunch). Returns the current system time in microseconds.
    virtual unsigned64 preSend(unsigned64 bunchStart) = 0;

    // Called just after 'prepared' was sent; stores it for acks / re-send.
    virtual void postSend() = 0;

    // Reference time of the given sent message, or 0 if not found.
    virtual unsigned64 getMessageTime(MsgSerial ser) = 0;

    virtual void cancelAllMessages() = 0;

    // Check connectivity; call periodically. Sends a new ping if necessary. now may be 0.
    virtual void checkConnectivity(unsigned64 now) = 0;

    // Whether the connection has dropped. Uses tolerant constants to allow re-connection.
    virtual bool dropped() = 0;

    virtual void tick() = 0;

    // Cancels all pending operations and drops all associations/registrations.
    virtual void close() = 0;

    ~NetChannel() override;
};

// Net-peer: bound to one local UDP port, drives all duplex traffic through it
// ([localIP:port]-[*:*]). Individual links are handled by net-channels.
class NetPeer : public Poseidon::Foundation::RefCountSafe
{
  protected:
    NetPool* pool;

    unsigned16 port; // local port (host endian)

    // Broadcast channel; processes broadcast (control) messages.
    Poseidon::Foundation::Ref<NetChannel> broadcastCh;

    // Non-public default constructor; descendants must do the initialization.
    NetPeer(NetPool* _pool);

#ifdef NET_LOG
    static unsigned nextId;
    unsigned id;
#endif

  public:
    virtual NetPool* getPool() const;

    // Local port (host endian), or 0 on failure.
    virtual unsigned16 getPort() const;

    // Local address (network endian).
    virtual void getLocalAddress(struct sockaddr_in& local) const = 0;

#ifdef NET_LOG
    // Print verbose peer info, e.g. "localIP:port".
    virtual char* getPeerInfo(char* buf) const;
    virtual unsigned getPeerId() const;
#endif

    virtual NetChannel* getBroadcastChannel() const;

    // Register a new net-channel. Two channels must not share the same distant address+port.
    // Client computers can use this only in a very limited way. If distant port is 0, the
    // control port is contacted and a distant port is assigned dynamically.
    virtual bool registerChannel(struct sockaddr_in& distant, NetChannel* ch) = 0;

    // Unregister a previously registered channel. Limited use on client computers.
    virtual void unregisterChannel(NetChannel* ch) = 0;

    // Find the net-channel bound to the given address; nullptr if none.
    virtual NetChannel* findChannel(const struct sockaddr_in& distant) = 0;

    // Max UDP datagram payload minus the whole overhead.
    virtual unsigned maxMessageData() const;

    virtual unsigned16 getLocalPort() const;

    // Connectivity-check counter, driven from outside this object.
    unsigned checkCounter;

    virtual void close() = 0;

    // Process incoming data; called asynchronously by the listener thread. Must not run
    // channel processing — peer statistics only. hdr carries length then data.
    virtual void processData(MsgHeader* hdr, const struct sockaddr_in& distant) = 0;

    // Send data to the given address. distant may be Zero() for broadcast.
    virtual NetStatus sendData(MsgHeader* hdr, struct sockaddr_in distant) = 0;

    virtual void cancelAllMessages() = 0;

    // Initialize dispatcher status. If data is nullptr, only the required length is returned.
    virtual unsigned initDispatcherStatus(DispatcherStatus* data) = 0;

    ~NetPeer() override;
};

// Negotiation interface: create a new game instance, or join an existing one.
class NetNegotiator : public Poseidon::Foundation::RefCountSafe
{
  public:
    // Initial network negotiation. Local ports etc. come from the given pool.
    // Returns true on success.
    virtual bool negotiate(NetPool* pool) = 0;

    ~NetNegotiator() override = default;
};

// Factory for NetPeer and NetChannel instances.
class PeerChannelFactory : public Poseidon::Foundation::RefCountSafe
{
  public:
    // Create a new net-peer; tryPorts are the local ports to try. nullptr on failure.
    virtual NetPeer* createPeer(NetPool* pool, Poseidon::Foundation::BitMask* tryPorts) = 0;

    // Create a new net-channel; broadcast selects a broadcast channel. nullptr on failure.
    virtual NetChannel* createChannel(NetPool* pool, bool broadcast = false) = 0;

    ~PeerChannelFactory() override = default;
};

inline unsigned64 sockaddrKey(const struct sockaddr_in& addr)
{
    return ((unsigned64)ADDR(addr) | ((unsigned64)PORT(addr) << 32));
}

extern unsigned64 channelKey(Poseidon::Foundation::RefD<NetChannel>& ch);

namespace Poseidon::Foundation
{
template <>
struct ImplicitMapTraits<RefD<NetChannel>>
{
    static RefD<NetChannel> zombie;
    static RefD<NetChannel> null;
};

template <>
struct ImplicitMapTraits<RefD<NetPeer>>
{
    static RefD<NetPeer> zombie;
    static RefD<NetPeer> null;
};
} // namespace Poseidon::Foundation

extern unsigned16 netPeerToPort(Poseidon::Foundation::RefD<NetPeer>& peer);

// Global network pool. Client and server implementations differ slightly;
// the default implementation is for client computers.
class NetPool : public Poseidon::Foundation::RefCountSafe
{
  protected:
    // The single factory asked for new peers and channels.
    Poseidon::Foundation::Ref<PeerChannelFactory> factory;

    Poseidon::Foundation::Ref<Poseidon::Foundation::BitMask> localPorts;

    // port -> NetPeer
    Poseidon::Foundation::ImplicitMap<unsigned16, Poseidon::Foundation::RefD<NetPeer>, netPeerToPort, true,
                                      Poseidon::Foundation::MemAllocSafe>
        peers;

    // [distantIP:port] -> NetChannel routing table
    Poseidon::Foundation::ImplicitMap<unsigned64, Poseidon::Foundation::RefD<NetChannel>, channelKey, true,
                                      Poseidon::Foundation::MemAllocSafe>
        channels;

  public:
    // Establishes network connections. ports is the default set of local ports to try.
    NetPool(PeerChannelFactory* fact, NetNegotiator* neg, Poseidon::Foundation::BitMask* ports);

    virtual Poseidon::Foundation::BitMask* getLocalPorts() const;

    virtual PeerChannelFactory* getFactory() const;

    // Create a new net-peer; tryPorts defaults to localPorts. No two peers may share a
    // local port. nullptr on failure.
    virtual NetPeer* createPeer(Poseidon::Foundation::BitMask* tryPorts = nullptr);

    // Find the net-peer on the given local port (host endian); nullptr if none.
    virtual NetPeer* findPeer(unsigned16 localPort);

    // Destroy a net-peer; all its pending operations and channels are cancelled too.
    virtual void deletePeer(NetPeer* peer);

    // Create a new net-channel to distant (network endian); peer may be nullptr.
    virtual NetChannel* createChannel(struct sockaddr_in& distant, NetPeer* peer = nullptr);

    // Find the net-channel to distant (network endian); nullptr if none.
    virtual NetChannel* findChannel(struct sockaddr_in& distant);

    // A net-channel able to process broadcast messages.
    virtual NetChannel* getBroadcastChannel();

    // Destroy a net-channel (not broadcast channels); pending operations are cancelled.
    virtual void deleteChannel(NetChannel* channel);

    // Tears down all network communication; call only during shutdown.
    ~NetPool() override;
};

#endif
