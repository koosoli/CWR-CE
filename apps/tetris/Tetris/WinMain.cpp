#ifdef _WIN32

#include <windows.h>
#include <cstring>
#include "TetrisApplication.hpp"
#include <Poseidon/Foundation/Common/ConsoleUtils.hpp>

int PASCAL WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR szCmdLine, int sw)
{
    if (!strstr(szCmdLine, "--check"))
        Poseidon::Foundation::attachParentConsole();
    TetrisApplication app;
    return app.Run(hInst, szCmdLine, sw);
}

#else // Linux

#include "TetrisApplication.hpp"

int main(int argc, char* argv[])
{
    TetrisApplication app;
    return app.Run(argc, argv);
}

#endif
