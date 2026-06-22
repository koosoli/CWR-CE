#include "ServerApplication.hpp"
#include <Poseidon/Foundation/Platform/CrashHandler.hpp>

#ifdef _WIN32
#include <windows.h>

int main(int argc, char* argv[])
{
    Poseidon::Foundation::InstallCrashHandler(nullptr);

    std::string cmdLine;
    for (int i = 1; i < argc; ++i)
    {
        if (i > 1)
            cmdLine += " ";
        cmdLine += argv[i];
    }

    ServerApplication app;
    return app.Run(GetModuleHandle(nullptr), const_cast<LPSTR>(cmdLine.c_str()), SW_SHOWNORMAL);
}
#else
int main(int argc, char* argv[])
{
    Poseidon::Foundation::InstallCrashHandler(nullptr);
    ServerApplication app;
    return app.Run(argc, argv);
}
#endif
