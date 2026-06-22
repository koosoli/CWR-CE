#pragma once
#include <string>

namespace Poseidon
{

struct StudioConfig
{
    int winX = -1;
    int winY = -1;
    int winW = 1280;
    int winH = 800;
    int maximized = 0;
    int sidebarPct = 30;

    void load();
    void save() const;
    static std::string configPath();
};

} // namespace Poseidon
