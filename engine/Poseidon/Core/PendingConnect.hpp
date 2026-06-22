#pragma once

#include <string>

namespace Poseidon
{

/// A server connection deferred across a mod-apply re-mount. Joining a modded
/// server tears the game down and rebuilds it with the new mod set, so the target
/// address must survive that boundary: the join flow Arm()s it before requesting
/// the re-mount, and the fresh boot Consume()s it once the menu is back up and then
/// connects. Distinct from the boot `--connect` auto-connect (which fires mid-intro
/// from a CLI arg) — this is an in-process, one-shot handoff.
class PendingConnect
{
  public:
    void Arm(std::string address, int port, std::string password)
    {
        _address = std::move(address);
        _port = port;
        _password = std::move(password);
        _armed = true;
    }

    bool IsArmed() const { return _armed; }
    const std::string& Address() const { return _address; }
    int Port() const { return _port; }
    const std::string& Password() const { return _password; }

    /// Disarm and forget the target. Call after consuming (or on cancel) so a later
    /// boot never reconnects by accident.
    void Disarm()
    {
        _armed = false;
        _address.clear();
        _port = 0;
        _password.clear();
    }

  private:
    bool _armed = false;
    std::string _address;
    int _port = 0;
    std::string _password;
};

/// Process-wide pending-connect, consumed by the post-re-mount boot hook.
PendingConnect& GPendingConnect();

} // namespace Poseidon
