#include <Poseidon/Security/Serial.hpp>
#include <Poseidon/IO/Streams/QStream.hpp>

// CD-key / RSA validation is unused: these methods are no-op stubs. MP identity
// is handled by NetworkConfig and MultiplayerAuth.

namespace Poseidon
{

CDKey::CDKey()
{
#if DECODE_ON_GET
    for (int i = 0; i < KEY_BYTES + 4; i++)
        _key[i] = 0;
    for (int i = 0; i < KEY_BYTES; i++)
        _message[i] = 0;
#else
    for (int i = 0; i < KEY_BYTES; i++)
        _buffer[i] = 0;
#endif
}

void CDKey::Init(const unsigned char* /*cdKey*/, const unsigned char* /*publicKey*/) {}

bool CDKey::Check(int /*offset*/, const char* /*with*/)
{
    return true;
}

__int64 CDKey::GetValue(int /*offset*/, int /*size*/)
{
    return 0;
}

// No-op stub: CD-key encryption is unused, but the symbol is kept so the
// CD-key publishing path in NetworkServerMission still links.
void Encrypt(QOStream& /*out*/, const unsigned char* /*publicKey*/, int /*keySize*/) {}

} // namespace Poseidon
