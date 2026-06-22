#pragma once

#include <Poseidon/Foundation/Strings/RString.hpp>
#include <cstdint>

namespace Poseidon
{
// Hash a raw OS machine-id string into a stable, positive 63-bit player id. The
// result fits a signed int64 so the server path (`_atoi64` + decimal
// `ban.txt`) round-trips it unchanged. Deterministic across runs; leading/trailing
// whitespace is trimmed (so the trailing newline in /etc/machine-id is harmless);
// empty/whitespace-only/null input returns 0. The values 0 and 999 are avoided
// because the server treats them as "no identity" sentinels.
uint64_t MachineIdToPlayerId(const char* rawMachineId);

// Read the OS machine identifier:
//   Windows: HKLM\SOFTWARE\Microsoft\Cryptography\MachineGuid
//   Linux:   /etc/machine-id, then /var/lib/dbus/machine-id
// Returns empty when no source is available.
RString ReadOsMachineId();
} // namespace Poseidon

// Multiplayer login identity string — a decimal player id stable per machine
// (see MachineIdToPlayerId). Falls back to "0" when no machine-id source exists,
// degrading to no-identity behavior. Declared at global scope to
// match the existing forward declarations at its call sites.
RString GetPublicKey();
