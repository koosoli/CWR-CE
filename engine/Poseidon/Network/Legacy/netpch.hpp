#pragma once

#include <Poseidon/Foundation/PoseidonPCH.hpp>

#include <atomic>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#if defined _WIN32
#include <windows.h>
#include <winsock2.h>
#else
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif

#include <Poseidon/Foundation/Types/Pointers.hpp>
#include <Poseidon/Foundation/Containers/Array.hpp>
#include <Poseidon/Foundation/Types/ScopeLock.hpp>

#include <Poseidon/Foundation/Framework/LogFlags.hpp>
#include <Poseidon/Foundation/Common/Global.hpp>
#include <Poseidon/Foundation/Threads/PoCritical.hpp>
#include <Poseidon/Foundation/Framework/NetLog.hpp>
#include <Poseidon/Foundation/Algorithms/Crc32.h>
#include <Poseidon/Foundation/Containers/Bitmask.hpp>
#include <Poseidon/Foundation/Containers/Maps.hpp>
#include <Poseidon/Foundation/Framework/PoTime.hpp>
#include <Poseidon/Foundation/Threads/PoThread.hpp>
#include <Poseidon/Foundation/Threads/PoSemaphore.hpp>

#include <Poseidon/Foundation/Common/NetGlobal.hpp>
#include <Poseidon/Network/Legacy/NetApi.hpp>
#include <Poseidon/Network/Legacy/NetMessage.hpp>
