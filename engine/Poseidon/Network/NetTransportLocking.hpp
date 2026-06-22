#pragma once

#include <type_traits>
#include <utility>

namespace Poseidon
{

template <class EnterFn, class LeaveFn, class Action>
decltype(auto) AccessNetTransportLocked(EnterFn&& enterLock, LeaveFn&& leaveLock, Action&& action)
{
    enterLock();
    if constexpr (std::is_void_v<std::invoke_result_t<Action>>)
    {
        action();
        leaveLock();
    }
    else
    {
        auto result = action();
        leaveLock();
        return result;
    }
}

} // namespace Poseidon
