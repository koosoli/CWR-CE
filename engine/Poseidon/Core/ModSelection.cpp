#include <Poseidon/Core/ModSelection.hpp>

#include <filesystem>
#include <fstream>
#include <system_error>

namespace Poseidon
{
namespace
{
std::string Trim(const std::string& value)
{
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
    {
        return {};
    }
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}
} // namespace

std::vector<std::string> LoadModSelection(const std::string& cfgPath)
{
    std::vector<std::string> mods;
    std::ifstream in(cfgPath);
    if (!in)
    {
        return mods;
    }

    std::string line;
    while (std::getline(in, line))
    {
        const std::string trimmed = Trim(line);
        if (trimmed.empty() || trimmed.rfind("//", 0) == 0 || trimmed[0] == '#')
        {
            continue;
        }
        mods.push_back(trimmed);
    }
    return mods;
}

bool SaveModSelection(const std::string& cfgPath, const std::vector<std::string>& mods)
{
    namespace fs = std::filesystem;
    std::error_code ec;
    const fs::path parent = fs::path(cfgPath).parent_path();
    if (!parent.empty())
    {
        fs::create_directories(parent, ec);
    }

    std::ofstream out(cfgPath, std::ios::binary | std::ios::trunc);
    if (!out)
    {
        return false;
    }

    out << "// Active mods, one @modId per line. Managed by the MODS screen.\n";
    for (const auto& mod : mods)
    {
        out << mod << "\n";
    }
    return out.good();
}
} // namespace Poseidon
