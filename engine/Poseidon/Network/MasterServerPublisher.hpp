#pragma once

#include <cstdint>
#include <string>

#include <Poseidon/Network/MasterServerServiceClient.hpp>

namespace Poseidon
{

struct MasterServerPublisherCallbacks
{
    bool (*serviceRegistrationQuery)(MasterServerServiceRegistration& out, void* userdata) = nullptr;
    void* userdata = nullptr;
};

bool StartMasterServerPublisher(const char* masterServerHost, const char* proxyServer, int queryPort,
                                const MasterServerPublisherCallbacks& callbacks);
void StopMasterServerPublisher();
void TickMasterServerPublisher();
void NotifyMasterServerStateChanged();

} // namespace Poseidon
