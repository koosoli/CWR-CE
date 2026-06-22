#pragma once

// Integer types with explicit bit width.
namespace Poseidon::Foundation
{
typedef unsigned char unsigned8;
typedef short int16;
typedef unsigned short unsigned16;
typedef int int32;
typedef unsigned unsigned32;

#ifdef _WIN32

typedef __int64 int64;
typedef unsigned __int64 unsigned64;

#else

typedef long long int64;
typedef unsigned long long unsigned64;

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned int DWORD; // Linux only (Windows uses unsigned long from SDK)
typedef unsigned long long QWORD;
typedef unsigned int UINT;

#ifndef FALSE
typedef int BOOL;
#define FALSE 0
#define TRUE 1
#endif

#endif

#define HAS_GETTIMEOFDAY

#define Zero(x) memset((void*)&(x), 0, sizeof(x))

#undef MAP_STAT
#undef MAP_DEBUG

} // namespace Poseidon::Foundation

using ::Poseidon::Foundation::unsigned8;
using ::Poseidon::Foundation::int16;
using ::Poseidon::Foundation::unsigned16;
using ::Poseidon::Foundation::int32;
using ::Poseidon::Foundation::unsigned32;
using ::Poseidon::Foundation::int64;
using ::Poseidon::Foundation::unsigned64;

#ifndef _WIN32
using ::Poseidon::Foundation::BYTE;
using ::Poseidon::Foundation::WORD;
using ::Poseidon::Foundation::DWORD;
using ::Poseidon::Foundation::QWORD;
using ::Poseidon::Foundation::UINT;
#ifndef FALSE
using ::Poseidon::Foundation::BOOL;
#endif
#endif
