#pragma once

#include "GameApplication.hpp"

class GameDemoApplication : public GameApplication
{
  public:
    GameDemoApplication() = default;

    bool IsDemo() const override { return true; }

  protected:
    void RegisterGameModules() override;
};
