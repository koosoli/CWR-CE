#pragma once

#include <CLI/CLI.hpp>
#include <string>

namespace PoseidonTools
{

class VersionCommand
{
  public:
    static void Setup(CLI::App& app);
    static int Execute();

  private:
    static std::string GetVersionString();
    static std::string GetBuildInfo();
};

} // namespace PoseidonTools
