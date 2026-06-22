#include "VersionCommand.hpp"
#include <iostream>
#include <sstream>
#include <CLI/App.hpp>
#include <cstdlib>
#include <functional>

namespace PoseidonTools
{

void VersionCommand::Setup(CLI::App& app)
{
    app.add_subcommand("version", "Display version information")->callback([]() { std::exit(Execute()); });
}

int VersionCommand::Execute()
{
    std::cout << GetVersionString() << std::endl;
    std::cout << GetBuildInfo() << std::endl;
    return 0;
}

std::string VersionCommand::GetVersionString()
{
    return "PoseidonTools v1.0.0";
}

std::string VersionCommand::GetBuildInfo()
{
    std::ostringstream oss;
    oss << "Build: " << __DATE__ << " " << __TIME__;

#ifdef _DEBUG
    oss << " (Debug)";
#else
    oss << " (Release)";
#endif

#ifdef _M_IX86
    oss << " x86";
#elif defined(_M_X64)
    oss << " x64";
#endif

#ifdef __clang__
    oss << " clang " << __clang_major__ << "." << __clang_minor__;
#elif defined(_MSC_VER)
    oss << " MSVC " << _MSC_VER;
#endif

    return oss.str();
}

} // namespace PoseidonTools
