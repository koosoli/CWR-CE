#pragma once

#include <CLI/CLI.hpp>

namespace PoseidonTools
{

class InputCommand
{
  public:
    static void Setup(CLI::App& app);

    static int ExecuteCheck();
    static int ExecuteTest();
};

} // namespace PoseidonTools
