#include <Poseidon/Network/MasterServerPublisher.hpp>

#include <Poseidon/Network/MasterServerServiceClient.hpp>
#include <Poseidon/Network/Network.hpp>
#include <Poseidon/Network/NetworkServerCommon.hpp>
#include <Poseidon/Foundation/Framework/PoTime.hpp>

#include <string>
#include <Poseidon/Foundation/Strings/RString.hpp>

using Poseidon::Foundation::getSystemTime;

namespace Poseidon
{

namespace
{
constexpr uint64_t ServicePublishIntervalMs = 30000;
constexpr uint64_t ServiceRetryIntervalMs = 5000;

MasterServerPublisherCallbacks GCallbacks{};
std::string GMasterServerHost;
std::string GProxyServer;
std::string GPublishedServerId;
uint64_t GNextPublishMs = 0;

uint64_t NowMs()
{
    return getSystemTime() / 1000;
}

const char* ProxyServerArg()
{
    return GProxyServer.empty() ? nullptr : GProxyServer.c_str();
}

void ResetPublisherState()
{
    GCallbacks = {};
    GMasterServerHost.clear();
    GProxyServer.clear();
    GPublishedServerId.clear();
    GNextPublishMs = 0;
}

bool QueryRegistration(MasterServerServiceRegistration& registration)
{
    if (GCallbacks.serviceRegistrationQuery == nullptr ||
        !GCallbacks.serviceRegistrationQuery(registration, GCallbacks.userdata))
    {
        return false;
    }

    if (registration.serverId.empty() && !registration.address.empty() && registration.hostPort > 0)
    {
        registration.serverId = BuildMasterServerServiceServerId(registration.address, registration.hostPort);
    }
    return !registration.serverId.empty();
}

bool PublishState(bool heartbeat)
{
    MasterServerServiceRegistration registration;
    if (!QueryRegistration(registration))
    {
        return false;
    }

    const bool ok =
        PublishMasterServerServiceRegistration(GMasterServerHost.c_str(), ProxyServerArg(), registration, heartbeat);
    if (ok)
    {
        GPublishedServerId = registration.serverId;
    }
    GNextPublishMs = NowMs() + (ok ? ServicePublishIntervalMs : ServiceRetryIntervalMs);
    return ok;
}
} // namespace

bool StartMasterServerPublisher(const char* masterServerHost, const char* proxyServer, int queryPort,
                                const MasterServerPublisherCallbacks& callbacks)
{
    // Persist the server token per local port, so it survives restarts (the master then
    // recognizes us on re-register instead of treating it as a takeover).
    std::string tokenDir = Poseidon::GetUserDirectory().Data();
    if (!tokenDir.empty() && tokenDir.back() != '/' && tokenDir.back() != '\\')
    {
        tokenDir += '/';
    }
    const std::string tokenPath = tokenDir + "papabear_token_" + std::to_string(queryPort);
    SetMasterServerServiceTokenStore(tokenPath.c_str());

    ResetPublisherState();
    GCallbacks = callbacks;
    GMasterServerHost = masterServerHost != nullptr ? masterServerHost : "";
    GProxyServer = proxyServer != nullptr ? proxyServer : "";
    return PublishState(false);
}

void StopMasterServerPublisher()
{
    if (!GPublishedServerId.empty())
    {
        UnregisterMasterServerService(GMasterServerHost.c_str(), ProxyServerArg(), GPublishedServerId.c_str());
    }
    ResetPublisherState();
}

void TickMasterServerPublisher()
{
    if (NowMs() < GNextPublishMs)
    {
        return;
    }
    PublishState(!GPublishedServerId.empty());
}

void NotifyMasterServerStateChanged()
{
    PublishState(!GPublishedServerId.empty());
}

} // namespace Poseidon
