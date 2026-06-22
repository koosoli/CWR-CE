#pragma once

#include <Poseidon/Network/NetTransportSessionPacketState.hpp>
#include <Poseidon/Network/NetTransport.hpp>

namespace Poseidon
{

template <class CreateAllocator, class DeleteAllocator>
void ApplyNetTransportPendingPlayerChanges(AutoArray<CreatePlayerInfo, CreateAllocator>& createPlayers,
                                           AutoArray<DeletePlayerInfo, DeleteAllocator>& deletePlayers,
                                           SessionPacket& session, CreatePlayerCallback* callbackCreate,
                                           DeletePlayerCallback* callbackDelete, void* context)
{
    for (int index = 0; index < deletePlayers.Size(); ++index)
    {
        DeletePlayerInfo& info = deletePlayers[index];
        callbackDelete(info.player, context);
        ApplyNetTransportSessionPlayerDelta(session, -1);
    }
    deletePlayers.Clear();

    for (int index = 0; index < createPlayers.Size(); ++index)
    {
        CreatePlayerInfo& info = createPlayers[index];
        callbackCreate(info.player, info.botClient, info.name, info.mod, context);
        ApplyNetTransportSessionPlayerDelta(session, 1);
    }
    createPlayers.Clear();
}

template <class CreateAllocator, class DeleteAllocator, class OnPendingFn>
void DrainNetTransportPendingPlayerChanges(AutoArray<CreatePlayerInfo, CreateAllocator>& createPlayers,
                                           AutoArray<DeletePlayerInfo, DeleteAllocator>& deletePlayers,
                                           SessionPacket& session, CreatePlayerCallback* callbackCreate,
                                           DeletePlayerCallback* callbackDelete, void* context, OnPendingFn&& onPending)
{
    if (deletePlayers.Size() || createPlayers.Size())
    {
        onPending(createPlayers.Size(), deletePlayers.Size());
    }

    ApplyNetTransportPendingPlayerChanges(createPlayers, deletePlayers, session, callbackCreate, callbackDelete,
                                          context);
}

template <class CreateAllocator, class DeleteAllocator, class EnterUsrFn, class LeaveUsrFn, class OnPendingFn>
void ProcessNetTransportPendingPlayerChanges(AutoArray<CreatePlayerInfo, CreateAllocator>& createPlayers,
                                             AutoArray<DeletePlayerInfo, DeleteAllocator>& deletePlayers,
                                             SessionPacket& session, CreatePlayerCallback* callbackCreate,
                                             DeletePlayerCallback* callbackDelete, void* context,
                                             EnterUsrFn&& enterUsers, LeaveUsrFn&& leaveUsers, OnPendingFn&& onPending)
{
    std::forward<EnterUsrFn>(enterUsers)();
    DrainNetTransportPendingPlayerChanges(createPlayers, deletePlayers, session, callbackCreate, callbackDelete,
                                          context, std::forward<OnPendingFn>(onPending));
    std::forward<LeaveUsrFn>(leaveUsers)();
}

template <class CreateAllocator, class DeleteAllocator, class EnterUsrFn, class LeaveUsrFn, class UserCountFn,
          class LogPendingFn>
void ProcessNetTransportServerPlayerChanges(AutoArray<CreatePlayerInfo, CreateAllocator>& createPlayers,
                                            AutoArray<DeletePlayerInfo, DeleteAllocator>& deletePlayers,
                                            SessionPacket& session, CreatePlayerCallback* callbackCreate,
                                            DeletePlayerCallback* callbackDelete, void* context,
                                            EnterUsrFn&& enterUsers, LeaveUsrFn&& leaveUsers,
                                            UserCountFn&& getUserCount, LogPendingFn&& logPending)
{
    ProcessNetTransportPendingPlayerChanges(
        createPlayers, deletePlayers, session, callbackCreate, callbackDelete, context,
        std::forward<EnterUsrFn>(enterUsers), std::forward<LeaveUsrFn>(leaveUsers),
        [&](int createdCount, int deletedCount)
        {
            std::forward<LogPendingFn>(logPending)(std::forward<UserCountFn>(getUserCount)(), session.numPlayers,
                                                   createdCount, deletedCount);
        });
}

template <class CreateAllocator, class DeleteAllocator, class EnterUsrFn, class LeaveUsrFn, class UserCountFn>
void ProcessNetTransportServerPlayerChangesWithLog(AutoArray<CreatePlayerInfo, CreateAllocator>& createPlayers,
                                                   AutoArray<DeletePlayerInfo, DeleteAllocator>& deletePlayers,
                                                   SessionPacket& session, CreatePlayerCallback* callbackCreate,
                                                   DeletePlayerCallback* callbackDelete, void* context,
                                                   EnterUsrFn&& enterUsers, LeaveUsrFn&& leaveUsers,
                                                   UserCountFn&& getUserCount)
{
    ProcessNetTransportServerPlayerChanges(
        createPlayers, deletePlayers, session, callbackCreate, callbackDelete, context,
        std::forward<EnterUsrFn>(enterUsers), std::forward<LeaveUsrFn>(leaveUsers),
        std::forward<UserCountFn>(getUserCount),
        [](unsigned userCount, int sessionPlayers, int createdCount, int deletedCount)
        {
            RptF("NetServer::ProcessPlayers(): users.card=%u, session.numPlayers=%d, created=%d, deleted=%d", userCount,
                 sessionPlayers, createdCount, deletedCount);
        });
}

template <class CreateAllocator, class DeleteAllocator>
void ClearNetTransportPendingPlayerChanges(AutoArray<CreatePlayerInfo, CreateAllocator>& createPlayers,
                                           AutoArray<DeletePlayerInfo, DeleteAllocator>& deletePlayers)
{
    createPlayers.Clear();
    deletePlayers.Clear();
}

template <class CreateAllocator, class DeleteAllocator, class EnterUsrFn, class LeaveUsrFn>
void RemoveNetTransportPendingPlayerChanges(AutoArray<CreatePlayerInfo, CreateAllocator>& createPlayers,
                                            AutoArray<DeletePlayerInfo, DeleteAllocator>& deletePlayers,
                                            EnterUsrFn&& enterUsers, LeaveUsrFn&& leaveUsers)
{
    std::forward<EnterUsrFn>(enterUsers)();
    ClearNetTransportPendingPlayerChanges(createPlayers, deletePlayers);
    std::forward<LeaveUsrFn>(leaveUsers)();
}

} // namespace Poseidon
