#ifdef _MSC_VER
#  pragma once
#endif

#include <Poseidon/Foundation/Common/Global.hpp>

// CRC32 checksum of buf[0..len), seeded with crc.

namespace Poseidon::Foundation
{
extern unsigned32 crc32 ( unsigned32 crc, const unsigned8 *buf, int64 len );

} // namespace Poseidon::Foundation
