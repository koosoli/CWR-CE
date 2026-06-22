#include <Poseidon/Network/NetworkConfig.hpp>
#include <Poseidon/Core/Config/EngineConfig.hpp>
#include <Poseidon/Foundation/Strings/RString.hpp>

const int DefaultNetworkPort = 1985;
const char* DefaultMasterServer = "https://papa-bear.cz";
static int NetworkPort = DefaultNetworkPort;
static int NetworkConnectPort = 0;
static RString NetworkAdvertiseAddress;
static RString NetworkPassword;
static RString NetworkMasterServer = DefaultMasterServer;
RString ClientIP;

int GetNetworkPort()
{
    return NetworkPort;
}

void SetNetworkPort(int port)
{
    NetworkPort = port;
}

int GetNetworkConnectPort()
{
    return NetworkConnectPort;
}

void SetNetworkConnectPort(int port)
{
    NetworkConnectPort = port;
}

RString GetNetworkAdvertiseAddress()
{
    return NetworkAdvertiseAddress;
}

void SetNetworkAdvertiseAddress(const RString& address)
{
    NetworkAdvertiseAddress = address;
}

RString GetNetworkPassword()
{
    return NetworkPassword;
}

void SetNetworkPassword(const RString& password)
{
    NetworkPassword = password;
}

RString GetNetworkMasterServer()
{
    return NetworkMasterServer;
}

void SetNetworkMasterServer(const RString& host)
{
    NetworkMasterServer = host;
}

static RString NetworkProxy;

RString GetNetworkProxy()
{
    return NetworkProxy;
}

void SetNetworkProxy(const RString& host)
{
    NetworkProxy = host;
}

bool IsDedicatedServer()
{
    return ENGINE_CONFIG.doCreateDedicatedServer;
}
