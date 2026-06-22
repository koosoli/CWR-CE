#pragma once

// when DECODE_ON_GET is non-zero, key is stored encrypted
// and is decrypted only when needed

#define DECODE_ON_GET 1

#define KEY_BITS 120
#define KEY_BYTES KEY_BITS / 8

#include <Poseidon/Foundation/platform.hpp>
namespace Poseidon
{

class CDKey
{
protected:
  #if DECODE_ON_GET
    unsigned char _key[KEY_BYTES+4];
    unsigned char _message[KEY_BYTES];
  #else
    unsigned char _buffer[KEY_BYTES];
  #endif

public:
    CDKey();
    void Init(const unsigned char *cdKey, const unsigned char *publicKey);

    bool Check(int offset, const char *with);
    __int64 GetValue(int offset, int size);
};

} // namespace Poseidon
