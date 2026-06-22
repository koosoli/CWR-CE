#include <Poseidon/Network/MasterServerBrowser.hpp>

#include <Poseidon/Network/MasterServerProtocol.hpp>
#include <Poseidon/Network/MasterServerServiceClient.hpp>
#include <Poseidon/Network/NetworkConfig.hpp>
#include <Poseidon/Network/Network.hpp>

#include <atomic>
#include <thread>
#include <string>
#include <vector>
#include <Poseidon/Foundation/Framework/Log.hpp>
#include <Poseidon/Foundation/Strings/RString.hpp>
#include <Poseidon/Foundation/Strings/StrFormat.hpp>

namespace Poseidon
{

struct MasterServerBrowser
{
    void* instance = nullptr;
    MasterServerBrowserProgressCallback progressCallback = nullptr;
    std::string host;
    std::vector<MasterServerServiceSession> serviceSessions;
    MasterServerBrowserState serviceState = MasterServerBrowserState::Idle;

    // Async fetch: UpdateMasterServerBrowser() spawns `worker` to run the
    // blocking HTTP query off the main thread, filling `pending` and flipping
    // `fetchDone`; ThinkMasterServerBrowser() swaps it into serviceSessions on
    // the main thread once done, so the UI never stalls on the round-trip.
    std::thread worker;
    std::vector<MasterServerServiceSession> pending;
    std::atomic<bool> fetchDone{false};
    // Worker-owned snapshots so it never reads UI memory that may change/free
    // once UpdateMasterServerBrowser() returns.
    std::string filterServerName;
    std::string filterMissionName;
    std::string proxySnapshot;
    MasterServerBrowserFilter filterSnapshot{};

    void joinWorker()
    {
        if (worker.joinable())
            worker.join();
    }
};

namespace
{
RString BuildBrowserFilterString(const MasterServerBrowserFilter& filter)
{
    RString value;
    VisitMasterServerBrowserFilterTerms(
        filter, [&](const MasterServerBrowserFilterTerm& term)
        { AppendMasterServerBrowserFilterCondition(value, BuildMasterServerBrowserFilterCondition(term)); });

    return value;
}
} // namespace

const RString& GetMasterServerBrowserFilterConjunction()
{
    static const RString And(" and ");
    return And;
}

void AppendMasterServerBrowserFilterCondition(RString& filter, RString condition)
{
    if (filter.GetLength() > 0)
    {
        filter = filter + GetMasterServerBrowserFilterConjunction();
    }
    filter = filter + condition;
}

RString BuildMasterServerBrowserFilterCondition(const MasterServerBrowserFilterTerm& term)
{
    switch (term.kind)
    {
        case MasterServerBrowserFilterTermKind::ServerName:
            return Format("%s like '%%%s%%'", MasterServerFieldHostName, term.text);
        case MasterServerBrowserFilterTermKind::MissionName:
            return Format("%s like '%%%s%%'", MasterServerFieldGameType, term.text);
        case MasterServerBrowserFilterTermKind::MinPlayers:
            return Format("%s >= %d", MasterServerFieldNumPlayers, term.number);
        case MasterServerBrowserFilterTermKind::MaxPlayers:
            return Format("%s <= %d", MasterServerFieldNumPlayers, term.number);
        case MasterServerBrowserFilterTermKind::ExcludeFullServers:
            return Format("%s < %s", MasterServerFieldNumPlayers, MasterServerFieldMaxPlayers);
    }

    return {};
}

RString BuildMasterServerBrowserFilterString(const MasterServerBrowserFilter& filter)
{
    return BuildBrowserFilterString(filter);
}

const char* GetMasterServerBrowserProxyServer(const RString& proxyServer)
{
    return proxyServer.GetLength() > 0 ? static_cast<const char*>(proxyServer) : nullptr;
}

static auto& GetCurrentMasterServerBrowserServiceSessions(MasterServerBrowser& browser)
{
    return browser.serviceSessions;
}

template <class ReadFn>
static auto ReadCurrentMasterServerBrowserSessionAccess(MasterServerBrowser* browser, int index, ReadFn&& readSession)
{
    return std::forward<ReadFn>(readSession)(browser, index, GetCurrentMasterServerBrowserServiceSessions);
}

MasterServerBrowser* CreateMasterServerBrowser(const char* masterServerHost, void* instance,
                                               MasterServerBrowserProgressCallback progressCallback)
{
    MasterServerBrowser* browser = new MasterServerBrowser();
    browser->instance = instance;
    browser->progressCallback = progressCallback;
    browser->host = masterServerHost != nullptr ? masterServerHost : "";
    browser->serviceState = MasterServerBrowserState::Idle;
    return browser;
}

void DestroyMasterServerBrowser(MasterServerBrowser* browser)
{
    if (browser)
    {
        browser->joinWorker();
    }
    delete browser;
}

MasterServerBrowserState GetMasterServerBrowserState(MasterServerBrowser* browser)
{
    return browser != nullptr ? browser->serviceState : MasterServerBrowserState::Idle;
}

void HaltMasterServerBrowser(MasterServerBrowser* browser)
{
    if (browser != nullptr)
    {
        browser->serviceState = MasterServerBrowserState::Idle;
    }
}

void ClearMasterServerBrowser(MasterServerBrowser* browser)
{
    if (browser != nullptr)
    {
        browser->serviceSessions.clear();
    }
}

void UpdateMasterServerBrowser(MasterServerBrowser* browser, const MasterServerBrowserFilter& filter)
{
    if (browser == nullptr)
    {
        return;
    }

    // A fetch is already in flight — don't stack another; the in-progress one
    // will land via ThinkMasterServerBrowser().
    if (browser->serviceState == MasterServerBrowserState::ListTransfer ||
        browser->serviceState == MasterServerBrowserState::Querying)
    {
        return;
    }

    browser->joinWorker(); // reap any finished worker not yet collected by Think

    // Snapshot everything the worker needs so it never touches UI-owned memory.
    browser->filterServerName = filter.serverName != nullptr ? filter.serverName : "";
    browser->filterMissionName = filter.missionName != nullptr ? filter.missionName : "";
    browser->filterSnapshot = filter;
    browser->filterSnapshot.serverName = browser->filterServerName.c_str();
    browser->filterSnapshot.missionName = browser->filterMissionName.c_str();
    RString proxy = GetNetworkProxy();
    browser->proxySnapshot = proxy.GetLength() > 0 ? static_cast<const char*>(proxy) : "";

    browser->pending.clear();
    browser->fetchDone.store(false, std::memory_order_relaxed);
    browser->serviceState = MasterServerBrowserState::ListTransfer;

    const std::string host = browser->host;
    browser->worker = std::thread(
        [browser, host]()
        {
            MasterServerBrowserState state = MasterServerBrowserState::ListTransfer;
            const char* proxyServer = browser->proxySnapshot.empty() ? nullptr : browser->proxySnapshot.c_str();
            // No instance / progress callback off-thread — progress is reflected
            // to the UI via serviceState (ListTransfer → Idle), not a callback.
            if (!RefreshMasterServerServiceBrowserFromDirectory(host.c_str(), browser->filterSnapshot, proxyServer,
                                                                browser->pending, state, nullptr, nullptr))
            {
                LOG_WARN(Network, "Failed to refresh master server service browser from '{}'", host);
            }
            browser->fetchDone.store(true, std::memory_order_release);
        });
}

void ThinkMasterServerBrowser(MasterServerBrowser* browser)
{
    if (browser == nullptr)
    {
        return;
    }
    if (browser->serviceState != MasterServerBrowserState::ListTransfer &&
        browser->serviceState != MasterServerBrowserState::Querying)
    {
        return;
    }
    if (!browser->fetchDone.load(std::memory_order_acquire))
    {
        return; // still fetching
    }

    browser->joinWorker();
    browser->serviceSessions = std::move(browser->pending);
    browser->pending.clear();
    browser->serviceState = MasterServerBrowserState::Idle;
    browser->fetchDone.store(false, std::memory_order_relaxed);
}

int GetMasterServerBrowserCount(MasterServerBrowser* browser)
{
    return browser != nullptr ? static_cast<int>(browser->serviceSessions.size()) : 0;
}

bool TryGetMasterServerBrowserSession(MasterServerBrowser* browser, int index, MasterServerSessionInfo& session)
{
    return ReadCurrentMasterServerBrowserSessionAccess(
        browser, index,
        [&](auto* currentBrowser, int currentIndex, auto&& getSessions)
        {
            auto* serviceSession = GetMasterServerBrowserServiceSession(
                currentBrowser, currentIndex, std::forward<decltype(getSessions)>(getSessions));
            if (serviceSession == nullptr)
            {
                return false;
            }
            AssignMasterServerServiceSessionInfo(*serviceSession, session);
            return true;
        });
}

const char* GetMasterServerBrowserStringValue(MasterServerBrowser* browser, int index, const char* key,
                                              const char* fallback)
{
    return ReadCurrentMasterServerBrowserSessionAccess(
        browser, index,
        [&](auto* currentBrowser, int currentIndex, auto&& getSessions)
        {
            auto* serviceSession = GetMasterServerBrowserServiceSession(
                currentBrowser, currentIndex, std::forward<decltype(getSessions)>(getSessions));
            return serviceSession != nullptr ? GetMasterServerServiceSessionStringValue(*serviceSession, key, fallback)
                                             : fallback;
        });
}

int GetMasterServerBrowserIntValue(MasterServerBrowser* browser, int index, const char* key, int fallback)
{
    return ReadCurrentMasterServerBrowserSessionAccess(
        browser, index,
        [&](auto* currentBrowser, int currentIndex, auto&& getSessions)
        {
            auto* serviceSession = GetMasterServerBrowserServiceSession(
                currentBrowser, currentIndex, std::forward<decltype(getSessions)>(getSessions));
            return serviceSession != nullptr ? GetMasterServerServiceSessionIntValue(*serviceSession, key, fallback)
                                             : fallback;
        });
}

int GetMasterServerBrowserPing(MasterServerBrowser* browser, int index)
{
    return ReadCurrentMasterServerBrowserSessionAccess(
        browser, index,
        [&](auto* currentBrowser, int currentIndex, auto&& getSessions)
        {
            auto* serviceSession = GetMasterServerBrowserServiceSession(
                currentBrowser, currentIndex, std::forward<decltype(getSessions)>(getSessions));
            return serviceSession != nullptr ? GetMasterServerServiceSessionPing(*serviceSession) : 0;
        });
}

const char* GetMasterServerBrowserAddress(MasterServerBrowser* browser, int index)
{
    return ReadCurrentMasterServerBrowserSessionAccess(
        browser, index,
        [&](auto* currentBrowser, int currentIndex, auto&& getSessions)
        {
            auto* serviceSession = GetMasterServerBrowserServiceSession(
                currentBrowser, currentIndex, std::forward<decltype(getSessions)>(getSessions));
            return serviceSession != nullptr ? GetMasterServerServiceSessionAddress(*serviceSession) : "";
        });
}

} // namespace Poseidon
