#include <Poseidon/Foundation/Threads/PoThread.hpp>

namespace Poseidon::Foundation
{

#ifdef _WIN32

bool poThreadCreate(ThreadId* id, long stackSize, THREAD_PROC_RETURN(THREAD_PROC_MODE* threadProc)(void*), void* arg)
{
    if (!threadProc)
    {
        return false;
    }
    if (stackSize < 0L)
    {
        stackSize = 0L;
    }
    DWORD thid; // thread id (unused)
    HANDLE tid = CreateThread(nullptr, stackSize, threadProc, arg, 0, &thid);
    if (id)
    {
        *id = tid;
    }
    return true;
}

bool poThreadJoin(ThreadId id, THREAD_PROC_RETURN* result)
{
    if (id == nullptr)
    {
        return false;
    }
    if (WaitForSingleObject(id, INFINITE) != WAIT_OBJECT_0)
    {
        return false;
    }
    if (!result)
    {
        return true;
    }
    return (GetExitCodeThread(id, result) != FALSE);
}

void poThreadId(ThreadId& id)
{
    id = reinterpret_cast<ThreadId>(static_cast<uintptr_t>(GetCurrentThreadId()));
}

const int PRIO[] = {
    THREAD_PRIORITY_LOWEST,       // -2 and below
    THREAD_PRIORITY_BELOW_NORMAL, // -1
    THREAD_PRIORITY_NORMAL,       //  0
    THREAD_PRIORITY_ABOVE_NORMAL, //  1
    THREAD_PRIORITY_HIGHEST       //  2 and more
};

bool poSetPriority(ThreadId id, int delta)
{
    if (delta < -2)
    {
        delta = -2;
    }
    else if (delta > 2)
    {
        delta = 2;
    }
    return (SetThreadPriority(id, PRIO[delta + 2]) != 0);
}

bool poSetMyPriority(int delta)
{
    return poSetPriority(reinterpret_cast<ThreadId>(static_cast<uintptr_t>(GetCurrentThreadId())), delta);
}

#else

bool poThreadCreate(ThreadId* id, long stackSize, THREAD_PROC_RETURN(THREAD_PROC_MODE* threadProc)(void*), void* arg)
{
    if (!threadProc)
        return false;
    // stackSize is ignored on POSIX
    pthread_t tid;
    if (pthread_create(&tid, nullptr, threadProc, arg) != 0)
        return false;
    if (id)
        *id = tid;
    return true;
}

bool poThreadJoin(ThreadId id, THREAD_PROC_RETURN* result)
{
    return (pthread_join(id, result) == 0);
}

void poThreadId(ThreadId& id)
{
    id = (ThreadId)pthread_self();
}

bool poSetPriority(ThreadId id, int delta)
{
    // priority control not implemented on POSIX
    return true;
}

bool poSetMyPriority(int delta)
{
    return poSetPriority((ThreadId)pthread_self(), delta);
}

#endif

RefCountSafe::~RefCountSafe() = default;

double RefCountSafe::GetMemoryUsed() const
{
    return 0.0;
}

} // namespace Poseidon::Foundation
