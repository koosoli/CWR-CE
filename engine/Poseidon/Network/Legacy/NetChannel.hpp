#include <Poseidon/Network/Legacy/SocketUtil.hpp>
#include <Poseidon/Network/Legacy/NetApi.hpp>
#include <Poseidon/Foundation/Common/NetGlobal.hpp>
#include <Poseidon/Network/Legacy/NetMessage.hpp>
#include <Poseidon/Foundation/Common/Global.hpp>
#ifdef _MSC_VER
#pragma once
#endif

#ifndef _NETCHANNEL_H
#define _NETCHANNEL_H

extern MsgSerial netMessageToSerial(RefD<NetMessage>& msg);
extern MsgSerial netMessageDepend(RefD<NetMessage>& msg);

#define MAX_ACK_ARRAY 1024 // Size of the acknowledgement array (incoming traffic).

#define MAX_OLD_ACKS 8 // Size of the high-priority acknowledgement queue.

// Status of all active message dispatchers (on all channels). Helps individual
// NetChannelBasic instances decide whether to send a message in getPreparedMessage().
struct DispatcherStatusBasic : public DispatcherStatus
{
    unsigned channels; // number of active channels.

    unsigned channelsWithUrgentMessages; // channels with at least one urgent message prepared to send.

    unsigned totalUrgentMessages; // total number of urgent messages prepared in all channels.

    unsigned channelsWithVIMMessages; // channels with at least one VIM (but not urgent) message prepared.

    unsigned totalVIMMessages; // total number of VIM (but not urgent) messages prepared in all channels
};

// Each net-channel is responsible for one [localIP:port]-[distantIP:port] connection.
class NetChannelBasic : public NetChannel
{
  public:
    // Does no initialization; use open() to set up a connection. _control marks a control channel.
    NetChannelBasic(bool _control);

    // Establish the connection for this channel. May negotiate with the distant peer
    // (a small amount of data can be exchanged) and blocks program flow. Returns nsError or nsOK.
    NetStatus open(NetPeer* _peer, struct sockaddr_in& distant) override;

    // Reconnect this channel to distant, keeping the whole channel state. Returns nsError or nsOK.
    NetStatus reconnect(struct sockaddr_in& distant) override;

    // Set network tuning parameters.
    void setNetworkParams(const NetworkParams& p) override;

    // Set network tuning parameters.
    static void setGlobalNetworkParams(const NetworkParams& p);

    // Channel latency in microseconds (0 if not determined yet). actLat returns the
    // actual latency, minLat the minimal (best possible) one.
    unsigned getLatency(unsigned* actLat = nullptr, unsigned* minLat = nullptr) override;

    // Output channel bandwidth (throughput) in bytes/sec. data is an optional extra record.
    unsigned getOutputBandWidth(EnhancedBWInfo* data = nullptr) override;

    // Retrieve special internal NetChannel statistics (not all items are meaningful in
    // every implementation). Returns true if at least some items were filled.
    bool getInternalStatistics(ChannelStatistics& stat) override;

    // Last time any message (even a control one) was received, in getSystemTime() format.
    unsigned64 getLastMessageArrival() const override;

    // Output message-queue statistics (not-yet-sent messages only): common message count
    // and bytes, then VIM message count and bytes.
    void getOutputQueueStatistics(int& msgs, int& bytes, int& vimMsgs, int& vimBytes) override;

    // Associated distant network address (peer).
    void getDistantAddress(struct sockaddr_in& distant) const override;

    // Process incoming data (called asynchronously by the listener thread). hdr holds the
    // message length and the message data itself; distant is where the data came from.
    void processData(MsgHeader* hdr, const struct sockaddr_in& distant) override;

    // Dispatch a message for output.
    void dispatchMessage(NetMessage* msg, bool urgent = false) override;

    // Last VIM message dispatched so far.
    NetMessage* getLastVIM(bool urgent) override;

    // Collect dispatcher status for this channel into the pre-allocated data buffer.
    void nextDispatcherStatus(DispatcherStatus* data) override;

    // Prepare the next message from the send-queue into 'prepared'. data carries the
    // collected dispatcher status of all active channels. Returns true if a message is ready.
    bool getPreparedMessage(void* data) override;

    // Called immediately before 'prepared' is sent: sets timing variables, checks the
    // packet-bunch mechanism, etc. bunchStart is the send-bunch start time (0 starts a new
    // bunch). Returns the current system time in microseconds.
    unsigned64 preSend(unsigned64 bunchStart) override;

    // Called immediately after 'prepared' is sent: stores the message for future access
    // (acknowledgements, re-send, etc.).
    void postSend() override;

    // Reference time of the given sent message (0 if not found).
    unsigned64 getMessageTime(MsgSerial ser) override;

    static const unsigned64 RUN_INTERVAL;

    static const unsigned64 ADJUST_INTERVAL;

    // Cancel all messages waiting to be sent.
    void cancelAllMessages() override;

    // Check channel connectivity, sending a new "ping" request if necessary. Call
    // periodically. now is the current system time (may be 0).
    void checkConnectivity(unsigned64 now) override;

    // Has the connection dropped? Uses tolerant time constants to allow re-connection.
    bool dropped() override;

    // Periodic channel update.
    void tick() override;

    // Close the channel, cancelling all pending operations and discarding registrations.
    void close() override;

    static NetworkParams par; // actual network params.

    // MT-safe new/delete.
    static void* operator new(size_t size);

    static void* operator new(size_t size, const char* file, int line);

    static void operator delete(void* mem);

#ifdef __INTEL_COMPILER

    static void operator delete(void* mem, const char* file, int line);

#endif

    // Cancels all pending operations.
    ~NetChannelBasic() override;

  protected:
    bool opened;

    struct sockaddr_in
        dist; // distant network address (dist.sin_addr.s_addr == INADDR_BROADCAST iff in broadcast mode).

    // output

    MsgSerial serial;         // next outgoing-message serial number (must not be MSG_NULL)
    MsgSerial lastSerialSent; // serial of last message really sent

    Ref<NetMessage> vimToSend; // linked-list of VIM (but not-urgent!) messages to send.
    NetMessage* vimToSendEnd;  // end of the VIM queue.
    Ref<NetMessage> lastVIM;   // last VIM message that has been dispatched.

    Ref<NetMessage> urgentToSend; // linked-list of urgent messages to send.
    NetMessage* urgentToSendEnd;  // end of the urgent queue.
    Ref<NetMessage> lastUrgent;   // last urgent VIM message that has been dispatched.

    Ref<NetMessage> commonToSend; // linked list of common messages to send.
    NetMessage* commonToSendEnd;  // end of the common queue.

    BitMaskMTS
        outputAckMask; // negative mask of received acknowledgements: 1 if sent but not acked (yet).

    MsgSerial ackMax; // actual maximum of incoming-acknowledgement.

    MsgSerial ackMin; // actual minimum of incoming-acknowledgement (incremented by batch).

    BitMaskMTS
        newOutputAckMask; // fresh messages sent recently. Periodically copied into 'outputAckMask'.

    // Queue of sent-message chunks. [2k] .. time, [2k+1] .. summary in bytes.
    unsigned64 sendQueue[SLIDING_WINDOW_SEND];

    // Index into sendQueue. Must be even.
    int sendIndex;

    // Set of NetMessages waiting for acknowledgement/resend etc.
    ImplicitMap<MsgSerial, RefD<NetMessage>, netMessageToSerial, true, Poseidon::Foundation::MemAllocSafe> revisited;

    // Last time runRevisited() was called.
    unsigned64 runTime;

    // Check all revisited NetMessages for timeout.
    void runRevisited();

    // Set MsgHeader data for the given output message (acknowledgements etc.).
    void setOutputData(NetMessage* msg);

    // Insert the re-sent message at the correct place.
    void insertResend(NetMessage* msg);

    // Insert the message into the urgent-message queue.
    void insertUrgent(NetMessage* msg);

    // Insert the message into the urgent-message queue just after 'after'.
    void insertUrgentAfter(NetMessage* msg, NetMessage* after);

    // Insert the message into the VIM-message queue.
    void insertVIM(NetMessage* msg);

    // Insert the message into the common-message queue.
    void insertCommon(NetMessage* msg);

    // Assemble the big-acknowledgement message. Accepts a predefined message length.
    void setBigAckMessage(NetMessage* msg);

    // Prepare the first urgent message into 'prepared'.
    bool getUrgentMessage();

    // Prepare the first VIM message into 'prepared'.
    bool getVIMMessage();

    // Prepare the first common message into 'prepared'.
    bool getCommonMessage();

    // input

    // Ordered messages waiting for their predecessors. Maps a predecessor serial number to
    // a linked list of its successors.
    ImplicitMap<MsgSerial, RefD<NetMessage>, netMessageDepend, true, Poseidon::Foundation::MemAllocSafe> deferred;

    // Update input statistics for a newly received message.
    void inputStatistics(NetMessage* msg);

    // Process a duplicate (resent) incoming message.
    void inputResent(NetMessage* msg);

    // Process an incoming VIM message. Handles everything but message ordering, and
    // processes pending messages too. Called in the "enter()" state; calls "leave()".
    void processVIM(NetMessage* msg);

    // Set/create an instant-message (heart-beat) response to 'request'. Works in the
    // middle of "enter()" mode.
    void setDelayMessage(NetMessage* request);

    // Incoming messages to be acknowledged. 0xff .. not received yet, 0 .. needs no
    // acknowledgement, 0 < N < 0xff .. needs at least N acknowledgements.
    unsigned8 ack[MAX_ACK_ARRAY];

    // Short queue of old messages to be acknowledged, with the highest priority.
    MsgSerial oldAckQueue[MAX_OLD_ACKS];

    // Index of the next oldAckQueue item to be processed.
    int oldAckFirst;

    // Insertion point of oldAckQueue.
    int oldAckLast;

    // Reference times of all incoming messages (only 32-bit differences matter). Only for
    // packet-pairs! Indices match 'ack'.
    unsigned ackTime[MAX_ACK_ARRAY];

    int ackPtr; // index into 'ack' corresponding to the 'inputMax' message.

    MsgSerial inputMax; // actual maximum of incoming-message serial number (incremented by 1).

    MsgSerial inputMin; // actual minimum serial number stored in 'ack'.

    BitMaskMTS ackMask; // mask of received messages (very large - for safety).

    MsgSerial ackMaskMin; // the oldest serial number stored in ackMask.

    bool starvation; // acknowledgement starvation (many incoming messages, few outgoing).

    unsigned recentVIMs; // VIM messages received after the last message departure.

    inline bool received(MsgSerial ser)
    {
        if (ser < ackMaskMin)
        {
            return true;
        }
        return ackMask.get(ser);
    }

    BitMaskMTS procMask; // mask of messages actually processed (application side). VIM messages only!

    unsigned newAcks; // number of newly acknowledged messages (temporary).

    unsigned newBytes; // total number of newly acknowledged bytes (temporary).

    // Process a message acknowledgement. Must be called inside enter().
    void newAcknowledgement(MsgSerial s, NetMessage* msg);

    // timings, reliability

    unsigned64 lastMsgArrival; // time of last message (of any type) arrival.

    unsigned64 lastMsgDeparture; // time of last message (of any type) departure (send time).

    unsigned64 lastPingArrival; // time of last "ping" (MSG_DELAY) message arrival.

    unsigned64 lastPingDeparture; // time of last "ping" (MSG_INSTANT) message departure (request).

    unsigned64 nextBandwidthDeparture; // time to send the next RB-estimate of bandwidth back to the sender.

    unsigned64 lastPairDeparture; // time of last "packet-pair" (MSG_BUNCH) message departure.

    // Check channel connectivity. Internal routine: must be called inside enter() .. leave().
    // now is the current system time (may be 0).
    void checkConnectivityInternal(unsigned64 now);

    // Heart-beat gap (computed dynamically from "timeout", in microseconds).
    unsigned heartBeatGap;

    // Average latency in micro-seconds (0 if not computed yet).
    unsigned aveLatency;

    // Actual latency in micro-seconds (0 if not computed yet).
    unsigned actLatency;

    // Minimum latency in micro-seconds (INT_MAX if not computed yet).
    unsigned minLatency;

    // Time the minimum latency was set. As it ages, it is replaced by a new value even if
    // the new value is higher.
    unsigned64 minLatencyTime;

    // Long-term estimate of ack-bandwidth (fades slowly to 0 if no outgoing traffic is acknowledged).
    unsigned goodAckBandwidth;

    // Sliding upper-bound estimate of output bandwidth from data actually transferred (bytes/sec).
    unsigned maxBandwidth;

    // Receiver-based packet-bunch estimate of bandwidth (bytes/sec).
    unsigned rbeBandwidth;

    // Queue of acknowledged-message chunks. [2k] .. time, [2k+1] .. summary in bytes.
    unsigned64 ackStatQueue[SLIDING_WINDOW];

    // Index into ackStatQueue. Must be even.
    int ackStatIndex;

    // Current ack-bandwidth.
    float getAckBandwidth(unsigned64 windowSize);

    // Current sent-bandwidth.
    float getSentBandwidth(unsigned64 windowSize, unsigned64 now = 0);

    // adjustChannel state

    // Last time adjustChannel() was called.
    unsigned64 adjustTime;

    // Periodic channel adjustment (maxBandwidth etc.).
    void adjustChannel();

    // Grow state from the current ping.
    int growStatePing;

    // Grow state from the current packet-loss ratio.
    int growStateLost;

    // For stat-output only.
    int growState;

    // Cumulative packet-separation for RB-estimation of incoming bandwidth (micro-seconds).
    unsigned inDelay;

    // Cumulative packet size (in bytes).
    unsigned inSize;

    // Current (averaged) RB-estimation for the receiver.
    unsigned inRbe;

#ifdef NET_LOG_BANDWIDTH
    // Number of packet-pairs used for the estimation.
    unsigned inCounter;

    // Counter for getOutputBandwidth net-logging.
    unsigned getOutputBandWidthCounter;

#endif

#ifdef NET_LOG_CH_STATE

    // Maximum interval of channel-state-log (CSL).
    static const unsigned64 CH_INFO_INTERVAL;

    // Time of the last CSL record.
    unsigned64 lastChannelStateLog;

    // Number of packets sent since the last CSL.
    unsigned packetsOut;

    // Number of VIM packets sent since the last CSL.
    unsigned packetsOutVIM;

    // Number of bytes sent since the last CSL.
    unsigned bytesOut;

    // Number of packets received since the last CSL.
    unsigned packetsIn;

    // Number of VIM packets received since the last CSL.
    unsigned packetsInVIM;

    // Number of bytes received since the last CSL.
    unsigned bytesIn;

#endif
};

#endif
