#pragma once

#include <CLI/CLI.hpp>
#include <string>

namespace PoseidonTools
{

class LintCommand
{
  public:
    static void Setup(CLI::App& app);

  private:
    static int LintMission(const std::string& missionPath);
};

} // namespace PoseidonTools
