#pragma once

#include <cstddef>

#include <Poseidon/Foundation/platform.hpp>
#include <Poseidon/Foundation/Common/Global.hpp>
#include <Poseidon/Foundation/Common/NetGlobal.hpp>
#include <Poseidon/Network/NetTransport.hpp>

namespace Poseidon
{

constexpr unsigned MSG_MAGIC_FLAG = 0x0001;

constexpr unsigned32 MAGIC_ENUM_REQUEST = 0xeee191ae;
constexpr unsigned32 MAGIC_ENUM_RESPONSE = 0xfff1e8ac;
constexpr unsigned32 MAGIC_ENUM_RESPONSE_LEGACY = 0xfff4fe12;
constexpr unsigned32 MAGIC_CREATE_PLAYER = 0xccca1e12;
constexpr unsigned32 MAGIC_ACK_PLAYER = 0xaaa51a7e;
constexpr unsigned32 MAGIC_RECONNECT_PLAYER = 0x111044ec;
constexpr unsigned32 MAGIC_TERMINATE_SESSION = 0x777814a1;
constexpr unsigned32 MAGIC_DESTROY_PLAYER = 0xddd15072;

constexpr int LEN_SESSION_NAME = 256;
constexpr int LEN_MISSION_NAME = 40;
constexpr int LEN_PLAYER_NAME = 40;
constexpr int LEN_PASSWORD_NAME = 40;

#pragma pack(push, netPackets, 1)

struct MagicPacket
{
    unsigned32 magic;
} PACKED;

struct EnumPacket : public MagicPacket
{
    int32 magicApplication;
} PACKED;

struct SessionPacket : public MagicPacket
{
    char name[LEN_SESSION_NAME];
    int32 actualVersion;
    int32 requiredVersion;
    char mission[LEN_MISSION_NAME];
    int32 gameState;
    int32 maxPlayers;
    int16 password;
    int16 port;
    int32 numPlayers;
    MsgSerial request;
    char mod[MOD_LENGTH];
    int16 equalModRequired;
    char versionTag[VERSION_TAG_LENGTH];
} PACKED;

constexpr size_t SESSION_PACKET_1 = offsetof(SessionPacket, numPlayers);
constexpr size_t SESSION_PACKET_2 = offsetof(SessionPacket, mod);
constexpr size_t SESSION_PACKET_3 = offsetof(SessionPacket, versionTag);
constexpr size_t SESSION_PACKET_4 = sizeof(SessionPacket);
constexpr size_t SESSION_PACKET_SIZE = SESSION_PACKET_4;

struct NetSessionDescription : public SessionPacket
{
    char address[32];
    unsigned32 ip;
    unsigned32 lastTime;
    unsigned32 pingTime;
} PACKED;

struct CreatePlayerPacket : public MagicPacket
{
    int32 magicApplication;
    char name[LEN_PLAYER_NAME];
    char password[LEN_PASSWORD_NAME];
    char mod[MOD_LENGTH];
    int32 actualVersion;
    int32 requiredVersion;
    int16 botClient;
    int32 uniqueID;
    char versionTag[VERSION_TAG_LENGTH];
} PACKED;

struct AckPlayerPacket : public MagicPacket
{
    int32 result;
    int32 playerNo;
} PACKED;

struct ReconnectPlayerPacket : public MagicPacket
{
    int32 playerNo;
} PACKED;

struct TerminateSessionPacket : public MagicPacket
{
    unsigned32 reason;
} PACKED;

struct DestroyPlayerPacket : public MagicPacket
{
} PACKED;

#pragma pack(pop, netPackets)

} // namespace Poseidon
