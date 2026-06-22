#pragma once

#include <Poseidon/Foundation/Strings/RString.hpp>
#include <Poseidon/Foundation/Containers/Array.hpp>
#include <cstdint>

namespace Poseidon
{
// Parse a dotted IPv4 ("a.b.c.d") to a canonical host-order value
// ((a<<24)|(b<<16)|(c<<8)|d). Returns false unless the string is exactly four
// decimal octets (0..255) separated by dots, with only surrounding whitespace.
bool ParseIPv4(const char* s, uint32_t& out);

// Format a canonical value (as produced by ParseIPv4) back to "a.b.c.d".
RString FormatIPv4(uint32_t ip);

// Load / save an IPv4 ban list (one dotted address per line; blank and
// unparseable lines are skipped). Used for the server's ipban.txt.
void LoadIpBanList(RString path, FindArray<uint32_t>& out);
void SaveIpBanList(RString path, const FindArray<uint32_t>& list);

// What a runtime Ban() records. Login always checks both the id and IP ban
// lists regardless of this; it only governs what Ban() writes.
enum class BanMode
{
    Id,
    Ip,
    Both
};

// Parse a server.cfg banMode value: "id" / "ip" / "both" (case-insensitive).
// Anything else (including empty) is Both — the default.
BanMode ParseBanMode(const char* s);
} // namespace Poseidon
