#include <Poseidon/Core/Application.hpp>
#include <Poseidon/Foundation/Common/Win.h>
#include <Poseidon/Foundation/Threads/MultiSync.hpp>
#include <Poseidon/IO/Serialization/ThreadSync.hpp>
#include <Poseidon/Core/Global.hpp>

#ifdef _WIN32

namespace Poseidon
{
int Poseidon::Foundation::SignaledObject::WaitForMultiple(Poseidon::Foundation::SignaledObject* events[], int n,
                                                          int timeout)
{
    const int maxHandles = 32;
    HANDLE handles[maxHandles];
    PoseidonAssert(n < maxHandles);
    for (int i = 0; i < n; i++)
    {
        handles[i] = events[i]->_handle;
    }
    if (timeout < 0)
    {
        timeout = INFINITE;
    }
    DWORD ret = ::WaitForMultipleObjects(n, handles,
                                         FALSE,  // wait for all or wait for one
                                         timeout // time-out interval in milliseconds
    );
    if (ret < WAIT_OBJECT_0 || ret >= WAIT_OBJECT_0 + n)
    {
        return -1;
    }
    return ret - WAIT_OBJECT_0;
}

} // namespace Poseidon

#endif
