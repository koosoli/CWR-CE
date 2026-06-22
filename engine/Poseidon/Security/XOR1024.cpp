#include <Poseidon/IO/Streams/QBStream.hpp>
#include <string.h>

namespace Poseidon
{

class FilebankEncryptionXOR1024 : public IFilebankEncryption
{
    char _password;

  public:
    FilebankEncryptionXOR1024(char password);
    bool Decode(char* dst, long lensb, QIStream& in);
    void Encode(QOStream& out, const char* dst, long lensb);
};

FilebankEncryptionXOR1024::FilebankEncryptionXOR1024(char password) : _password(password) {}

bool FilebankEncryptionXOR1024::Decode(char* dst, long lensb, QIStream& in)
{
    memcpy(dst, in.act(), lensb);
    for (int i = 0; i < lensb; i++)
    {
        dst[i] ^= _password + i;
    }
    return true;
}

void FilebankEncryptionXOR1024::Encode(QOStream& out, const char* dst, long lensb)
{
    int i;
    for (i = 0; i < lensb; i++)
    {
        out.put(dst[i] ^ (_password + i));
    }
    const int mask = 1024 - 1;
    while ((i & mask) != 0)
    {
        out.put(0);
        i++;
    }
}

IFilebankEncryption* CreateEncryptXOR1024(const void* /*context*/)
{
    // XOR uses a fixed key; context is ignored
    char key = 'a';
    return new FilebankEncryptionXOR1024(key);
}

} // namespace Poseidon
