#include <Poseidon/Core/Application.hpp>
#include <Poseidon/Foundation/Common/Win.h>
#include <Poseidon/Foundation/Framework/GlobalAlive.hpp>
#if defined(_M_X64) || defined(_M_AMD64)
#include <intrin.h>
#elif defined(__x86_64__)
#include <x86intrin.h>
#endif

#ifdef _WIN32
#include <Poseidon/Foundation/Threads/MultiSync.hpp>
#endif

#include <Poseidon/Dev/Debug/DebugTrap.hpp>

void DDTerm();

namespace Poseidon::Dev
{

#ifdef _WIN32

class DebugThreadWatch
{
    mutable Foundation::CriticalSection _lock;
    int _timeOut;
    int _enable; // <0 disabled, >=0 enabled

    HANDLE _mainThreadHandle;
    HANDLE _myThreadHandle;
    DWORD _myThreadId;
    Foundation::Event _terminate;
    Foundation::Event _keepAlive;

  public:
    DebugThreadWatch();
    ~DebugThreadWatch();

    DWORD Execute();

    static DWORD WINAPI ExecuteHelper(void* watch) { return ((DebugThreadWatch*)watch)->Execute(); }

    // external access functions
    void Terminate() { _terminate.Set(); }
    void KeepAlive() { _keepAlive.Set(); }
    void AliveEnable(int count)
    {
        Foundation::ScopeLockSection lock(_lock);
        _enable += count;
    }
    int GetAliveEnabled() const
    {
        Foundation::ScopeLockSection lock(_lock);
        return _enable;
    }

    void SetAliveTimeout(int timeMs)
    {
        Foundation::ScopeLockSection lock(_lock);
        _timeOut = timeMs;
    }
};

DebugThreadWatch::DebugThreadWatch()
{
    Foundation::ScopeLockSection lock(_lock);
    _timeOut = -1;
    _enable = 0;

    ::DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &_mainThreadHandle, 0, false,
                      DUPLICATE_SAME_ACCESS);

    DWORD threadId;
    _myThreadId;
    _myThreadHandle = ::CreateThread(nullptr, 64 * 1024, ExecuteHelper, this, CREATE_SUSPENDED, &threadId);

    if (_myThreadHandle)
    {
        ::ResumeThread(_myThreadHandle);
    }
}

DWORD DebugThreadWatch::Execute()
{
    DWORD lastAlive = ::GetTickCount();
    for (;;)
    {
        Foundation::SignaledObject* wait[] = {&_terminate, &_keepAlive};
        int waitTime, waitTime0;
        {
            Foundation::ScopeLockSection lock(_lock);
            waitTime0 = waitTime = _timeOut;
            if (waitTime < 0 || waitTime > 10000)
            {
                waitTime = 10000;
            }
        }
        DWORD start = ::GetTickCount();
        int which = Foundation::SignaledObject::WaitForMultiple(wait, sizeof(wait) / sizeof(*wait), waitTime);
        if (which == 0)
        {
            return 0; // _terminate event
        }
        else if (which == 1) // alive event
        {
            lastAlive = ::GetTickCount();
            DWORD delay = lastAlive - start;
            if (delay > static_cast<DWORD>(waitTime - 2000) ||
                delay > 2500) // waitTime clamped to [0,10000], safe to cast
            {
                LOG_DEBUG(Core, "No alive in {} of {} ms", delay, waitTime);
                LOG_DEBUG(Core, "  check another threads -- add ProgressRefresh()");
            }
            // no more event expected until told by NextAlive
        }
        else
        {
            if (waitTime0 >= 0 && waitTime0 < 20000 && _enable > 0)
            {
                LOG_DEBUG(Core, "Freeze detected: no alive in {} ms", waitTime);
                ::SuspendThread(_mainThreadHandle);
                if (::GetTickCount() - lastAlive >= 30000)
                {
                    // fatal situation - terminate
                    ::DDTerm();
                    exit(1);
                }
                else
                {
                    ::ResumeThread(_mainThreadHandle);
                }
            }
        }
    }
    // Unreachable - infinite loop above always returns via _terminate event
}

DebugThreadWatch::~DebugThreadWatch() = default;

Debugger::Debugger()
{
    _isDebugger = false;

    HMODULE kernel32 = LoadLibrary("kernel32.dll");
    if (kernel32)
    {
        typedef BOOL IsDebuggerPresentProc(VOID);
        IsDebuggerPresentProc* debPresent = (IsDebuggerPresentProc*)(GetProcAddress(kernel32, "IsDebuggerPresent"));
        if (debPresent)
        {
            _isDebugger = debPresent() != FALSE;
            if (_isDebugger)
            {
                LOG_DEBUG(Core, "Starting debugger session");
            }
        }
    }
}

void Debugger::ForceLogging()
{
    _isDebugger = false;
}

void Debugger::NextAliveExpected(int timeout)
{
    if (_watch)
    {
        _watch->SetAliveTimeout(timeout);
        _watch->KeepAlive();
    }
}

bool Debugger::CheckingAlivePaused()
{
    if (_watch)
    {
        return _watch->GetAliveEnabled() >= 0;
    }
    return false;
}

void Debugger::PauseCheckingAlive()
{
    if (_watch)
    {
        _watch->AliveEnable(-1);
    }
}

void Debugger::ResumeCheckingAlive()
{
    if (_watch)
    {
        _watch->AliveEnable(+1);
    }
    I_AM_ALIVE();
}

void Debugger::ProcessAlive()
{
    static int lastAlive = GetTickCount();

    int time = GetTickCount();
    int delay = time - lastAlive;
    lastAlive = time;

    if (delay > 1000)
    {
        __nop();
    }
    if (_watch)
    {
        _watch->KeepAlive();
    }
}

Debugger::~Debugger()
{
    if (_watch)
    {
        _watch->Terminate();
    }

    if (_isDebugger)
    {
        LOG_DEBUG(Core, "Closing debugger session");
    }
}

#else // ifdef _WIN32

class DebugThreadWatch
{
};

Debugger::Debugger()
{
    _isDebugger = false;
}

void Debugger::ForceLogging()
{
    _isDebugger = false;
}

void Debugger::NextAliveExpected(int timeout) {}

bool Debugger::CheckingAlivePaused()
{
    return false;
}

void Debugger::PauseCheckingAlive() {}

void Debugger::ResumeCheckingAlive() {}

void Debugger::ProcessAlive() {}

Debugger::~Debugger() {}

#endif

Debugger GDebugger;

} // namespace Poseidon::Dev
