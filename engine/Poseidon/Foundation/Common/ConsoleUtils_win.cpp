#include <Poseidon/Foundation/Common/ConsoleUtils.hpp>
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <cstdio>

namespace Poseidon::Foundation
{

static void reopenStream(DWORD nStdHandle, FILE* stream, const char* mode)
{
    HANDLE h = GetStdHandle(nStdHandle);
    if (h == NULL || h == INVALID_HANDLE_VALUE)
        return;
    int fd = _open_osfhandle(reinterpret_cast<intptr_t>(h), _O_TEXT);
    if (fd < 0)
        return;
    FILE* fp = _fdopen(fd, mode);
    if (!fp)
    {
        _close(fd);
        return;
    }
    *stream = *fp;
    setvbuf(stream, NULL, _IONBF, 0);
}

void attachParentConsole()
{
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != NULL && hOut != INVALID_HANDLE_VALUE)
    {
        // Pipes/redirects exist — reconnect C runtime to the OS handles
        reopenStream(STD_OUTPUT_HANDLE, stdout, "w");
        reopenStream(STD_ERROR_HANDLE, stderr, "w");
        return;
    }

    if (AttachConsole(ATTACH_PARENT_PROCESS))
    {
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
        freopen("CONIN$", "r", stdin);
    }
#ifdef _DEBUG
    else
    {
        AllocConsole();
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
        freopen("CONIN$", "r", stdin);
    }
#endif
}

} // namespace Poseidon::Foundation

