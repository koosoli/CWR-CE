#ifndef POSEIDON_TEST_HARNESS_COMMAND_QUEUE_HPP
#define POSEIDON_TEST_HARNESS_COMMAND_QUEUE_HPP

// Thread-safe command queue bridging harness TCP thread → main game loop.

#ifndef _WIN32
#include <pthread.h>
#endif
#include <Poseidon/Foundation/Threads/PoCritical.hpp>
#include <functional>
#include <string>
#include <vector>

namespace Poseidon::Dev
{

/// A parsed command from the harness TCP client.
struct HarnessCommand
{
    std::string name;   // "ping", "key", "exit", etc.
    std::string raw;    // Full JSON line (for command-specific field extraction)
    int id = 0;         // Sequence number for response correlation
};

/// A response to send back to the harness client.
struct HarnessResponse
{
    int id = 0;
    std::string json;   // Pre-formatted JSON line (with trailing \n)
};

/// A pushed event to send to the harness client.
struct HarnessEvent
{
    std::string json;   // Pre-formatted JSON line (with trailing \n)
};

/// Thread-safe queue for commands (TCP thread → main) and responses (main → TCP).
class HarnessCommandQueue
{
  public:
    HarnessCommandQueue() = default;

    // --- Command queue (TCP thread pushes, main thread pops) ---

    void PushCommand(HarnessCommand cmd)
    {
        _cmdLock.Lock();
        _commands.push_back(std::move(cmd));
        _cmdLock.Unlock();
    }

    bool HasPendingCommand()
    {
        _cmdLock.Lock();
        bool has = !_commands.empty();
        _cmdLock.Unlock();
        return has;
    }

    bool PopCommand(HarnessCommand& out)
    {
        _cmdLock.Lock();
        if (_commands.empty())
        {
            _cmdLock.Unlock();
            return false;
        }
        out = std::move(_commands.front());
        _commands.erase(_commands.begin());
        _cmdLock.Unlock();
        return true;
    }

    // --- Response queue (main thread pushes, TCP thread pops) ---

    void PushResponse(HarnessResponse resp)
    {
        _respLock.Lock();
        _responses.push_back(std::move(resp));
        _respLock.Unlock();
    }

    bool PopResponse(HarnessResponse& out)
    {
        _respLock.Lock();
        if (_responses.empty())
        {
            _respLock.Unlock();
            return false;
        }
        out = std::move(_responses.front());
        _responses.erase(_responses.begin());
        _respLock.Unlock();
        return true;
    }

    // --- Event queue (main thread pushes, TCP thread drains) ---

    void PushEvent(HarnessEvent evt)
    {
        _evtLock.Lock();
        _events.push_back(std::move(evt));
        _evtLock.Unlock();
    }

    std::vector<HarnessEvent> DrainEvents()
    {
        std::vector<HarnessEvent> out;
        _evtLock.Lock();
        out.swap(_events);
        _evtLock.Unlock();
        return out;
    }

  private:
    Foundation::PoCriticalSection _cmdLock;
    std::vector<HarnessCommand> _commands;

    Foundation::PoCriticalSection _respLock;
    std::vector<HarnessResponse> _responses;

    Foundation::PoCriticalSection _evtLock;
    std::vector<HarnessEvent> _events;
};

} // namespace Poseidon::Dev

#endif // POSEIDON_TEST_HARNESS_COMMAND_QUEUE_HPP
