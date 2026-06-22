#include <Poseidon/Foundation/platform.hpp>
#include <Poseidon/Foundation/Common/Global.hpp>
#ifdef _MSC_VER
#pragma once
#endif

#ifndef _NETGLOBAL_H
#define _NETGLOBAL_H

#include <cstdint>

#ifndef __GNUC__
#pragma pack(push, netGlobal, 1)
#endif

class NetMessage;

namespace Poseidon::Foundation
{
typedef unsigned32 MsgSerial;
/// Dummy message serial number
const MsgSerial MSG_SERIAL_NULL = 0;

#ifdef _WIN32

/// IP address from sockaddr_in
#define ADDR(addr) ntohl((addr).sin_addr.S_un.S_addr)

/// IP4 byte from sockaddr_in
#define IP4(addr) ((addr).sin_addr.S_un.S_un_b.s_b1)
/// IP3 byte from sockaddr_in
#define IP3(addr) ((addr).sin_addr.S_un.S_un_b.s_b2)
/// IP2 byte from sockaddr_in
#define IP2(addr) ((addr).sin_addr.S_un.S_un_b.s_b3)
/// IP1 byte from sockaddr_in
#define IP1(addr) ((addr).sin_addr.S_un.S_un_b.s_b4)

/// Socket handle type. Mirrors the winsock <code>::SOCKET</code> ABI (UINT_PTR)
/// so the qualified name <code>Poseidon::Foundation::SOCKET</code> resolves on
/// Windows exactly as the POSIX <code>int</code> form does below.
typedef std::uintptr_t SOCKET;

#else

/// IP address from sockaddr_in
#define ADDR(addr) ntohl((addr).sin_addr.s_addr)

/// IP4 byte from sockaddr_in
#define IP4(addr) ((addr).sin_addr.s_addr & 0xff)
/// IP3 byte from sockaddr_in
#define IP3(addr) (((addr).sin_addr.s_addr >> 8) & 0xff)
/// IP2 byte from sockaddr_in
#define IP2(addr) (((addr).sin_addr.s_addr >> 16) & 0xff)
/// IP1 byte from sockaddr_in
#define IP1(addr) (((addr).sin_addr.s_addr >> 24) & 0xff)

typedef int SOCKET;

/// Return value in case of error (general usage).
#define SOCKET_ERROR -1

/// Return value of socket() in case of error.
#define INVALID_SOCKET -1

/// Closing of socket handle is like closing anything else in POSIX..
#define closesocket(s) ::close(s)

#endif

#define PORT(addr) ntohs((addr).sin_port)

/// message-ID is included in MsgHeader (for debugging purposes only).
// #  define MSG_ID

// Fixed-size message header carried above UDP. The underlying protocol need not be
// reliable. All fields are in network-byte order.
// Note: some security mechanism against message-faking.
struct MsgHeader
{
    /// length of the message data (<strong>including</strong> this header). Can be <code>MSG_HEADER_LEN</code> (for
    /// ack).
    unsigned16 length;
    /// message flags
    unsigned16 flags;
    /// 32-bit CRC check-sum (including the header). ??? alternative: 16-bit CRC ???
    unsigned32 crc;
    /// serial number of the message (unique within one directed communication channel)
    MsgSerial serial;
    /// origin of the acknowledge-bitmask (serial number of the newest acknowledgement being transmitted)
    unsigned32 ackOrigin;
    union
    {
        /// acknowledge bit-mask (newer messages are acknowledged in low-significant bits, includes
        /// <code>MSG_NULL</code>)
        unsigned64 ackBitmask;
        struct
        {
            /// 1st part of an acknowledgement, any other control data
            unsigned32 control1;
            /// previous VIM message for VIM ordering, any other control data
            unsigned32 control2;
        } c;
    };
#ifdef MSG_ID
    /// Copy of NetMessage::id (for debugging purposes only)
    MsgSerial id;
#endif
} PACKED;

/// Length of message header in bytes (constant) .. 24 bytes.
#define MSG_HEADER_LEN sizeof(MsgHeader)
/// VIM (Very Important Message = guaranteed) flag.
#define MSG_VIM_FLAG 0x8000
/// Urgent message flag.
#define MSG_URGENT_FLAG 0x4000
/// VIM message ordering flag (<code>control2</code> contains serial number of previous VIM message).
#define MSG_ORDERED_FLAG 0x2000
/// From-broadcast flag (acknowledgement mask etc. should be ignored).
#define MSG_FROM_BCAST_FLAG 0x1000
/// To-broadcast flag (message will be processed by special /control/ channel).
#define MSG_TO_BCAST_FLAG 0x0800
/// Time-delay flag (<code>control2</code> contains time-delay value in micro-seconds ... refers to the newest
/// acknowledged message).
#define MSG_DELAY_FLAG 0x0400
/// Instant-reply flag (instant message reply is required - with <code>MSG_DELAY_FLAG</code> set).
#define MSG_INSTANT_FLAG 0x0200
/// Message header contains bandwidth value computed on the receiver (<code>control2</code>).
#define MSG_BANDWIDTH_FLAG 0x0100
/// Packet-bunch flag (this message was sent immediately after the previous one).
#define MSG_BUNCH_FLAG 0x0080
/// Dummy = internal packet (must be ignored by upper layers).
#define MSG_DUMMY_FLAG 0x0040
/// Part of a bigger message.
#define MSG_PART_FLAG 0x0020
/// Closing part of bigger message.
#define MSG_CLOSING_FLAG 0x0010
/// User flags
#define MSG_USER_FLAGS 0x000f
/// All flags
#define MSG_ALL_FLAGS 0xffff

/// Short (32-bit) acknowledgement bit-mask predicate.
#define SHORT_ACK(flags) (((flags) & (MSG_ORDERED_FLAG | MSG_DELAY_FLAG | MSG_BANDWIDTH_FLAG)) != 0)

/// Total headers' size of superior protocols (IP & UDP).
#define IP_UDP_HEADER 28

// Fixed-size header that follows MsgHeader in MSG_FRACTION_FLAG-tagged messages. Not
// implemented. All fields are in network-byte order.
struct MsgFractionHeader
{
    /// Total message length in bytes
    unsigned32 totalLen;

    /// Offset of this partial message (fraction) in bytes
    unsigned32 offset;

} PACKED;

/**
    Status codes for getStatus() routines (asynchronous receive* and send* result codes).
    <p>INPUT:
    <ul>
      <li><code>nsInvalidMessage</code> - I/O operation has not been initiated yet
      <li><code>nsInputPending</code> - Input operation was initiated but has not been finished yet
      <li><code>nsInputReceived</code> - Input (receive) operation was finished (but no acknowledgements has been sent
   yet) <li><code>nsInputPartialAck</code> - Small number of acknowledgements (but yet not enough) has been sent
      <li><code>nsInputAck</code> - Complete set of acknowledgements has been sent
      <li><code>nsInvalidMessage</code> - message was recycled (returned to the <code>NetMessagePool</code>)
    </ul>
    <p>Memo: for <b>non-VIM</b> messages there is no guarantee of <code>nsInputPartialAck</code> and
       <code>nsInputAck</code> states!
    <p>OUTPUT:
    <ul>
      <li><code>nsInvalidMessage</code> - I/O operation has not been initiated yet (it is possible to cancel the
   <b>send</b> operation) <li><code>nsOutputPending</code> - Output operation was initiated but has not been finished
   yet <li><code>nsOutputSent</code> - Output (send) operation was finished (but no acknowledgement has been received
   yet) <li>[<code>nsOutputTimeout</code>] - Message has been re-sent (at least once) after acknowledgement timeout
      <li><code>nsOutputAck</code> - Message acknowledgement has been received
      <li><code>nsInvalidMessage</code> - message was recycled (returned to the <code>NetMessagePool</code>)
    </ul>
    <p>Memo: for <b>non-VIM</b> messages there is no guarantee of <code>nsOutputTimeout</code> and
       <code>nsOutputAck</code> states!
*/
enum NetStatus
{
    nsError, ///< undetermined error has been occurred
    nsOK,    ///< operation finished successfully

    nsInvalidSharing, ///< invalid port/address sharing (while establishing a connection)
    nsInvalidMessage, ///< invalid message header/serial number (before I/O initiation call)

    nsInputPending,    ///< operation is in input-pending state
    nsInputReceived,   ///< message was received but no acknowledgement has been sent yet
    nsInputPartialAck, ///< message was received and a small number of acknowledgements has been sent
    nsInputAck,        ///< message was received and a complete set of acknowledgements has been sent

    nsOutputPending,  ///< operation is in output-pending state
    nsOutputSent,     ///< message has been sent (but no acknowledgement was received yet)
    nsOutputObsolete, ///< send-timeout of the message had been expired; the message was not sent
    nsOutputTimeout,  ///< message has been re-sent after acknowledgement timeout
    nsOutputAck,      ///< message acknowledgement has been received

    nsCancel,          ///< message has been cancelled
    nsNoMoreCallbacks, ///< no more callbacks are needed for this message (for nextEvent only)
};


/**
    Call-back routine for asynchronous network I/O.
    @param  msg NetMessage to be processed.
    @param  event Type of call-back event.
    @param  data Context data (non-mandatory).
    @return Next required call-back event type (with the same call-back routine).
*/
typedef NetStatus NetCallBack(::NetMessage* msg, NetStatus event, void* data);

#ifndef __GNUC__
#pragma pack(pop, netGlobal)
#endif

// Net-stress simulation (internal builds only) — uncomment to enable.
// #define NET_STRESS
/// Net-stress: packet drop probability.
#define NET_STRESS_DROP 0.2
/// Net-stress: packet delay (latency) in milliseconds.
#define NET_STRESS_LATENCY 5

#ifdef NET_STRESS
#include <Poseidon/Network/Legacy/RandomJames.h>
extern RandomJames stressRnd;
#endif

} // namespace Poseidon::Foundation

using ::Poseidon::Foundation::MsgSerial;
using ::Poseidon::Foundation::MsgHeader;
using ::Poseidon::Foundation::MsgFractionHeader;
using ::Poseidon::Foundation::NetStatus;
using ::Poseidon::Foundation::NetCallBack;

#endif
