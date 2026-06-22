#include "LintCommand.hpp"

#include <Poseidon/IO/ParamFile/ParamFile.hpp>

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <CLI/App.hpp>
#include <CLI/Option.hpp>
#include <cstdlib>
#include <functional>
#include <Poseidon/Foundation/Strings/RString.hpp>

namespace fs = std::filesystem;

namespace PoseidonTools
{

namespace
{

struct SectionLintResult
{
    std::string name;
    bool parsed = false;
    bool bootable = false;
    int groupCount = 0;
    int playerCount = 0;
    int vehicleCount = 0;
    int sensorCount = 0;
    int markerCount = 0;
};

const ParamClass* GetChildClass(const ParamEntry& parent, const char* name)
{
    ParamEntry* child = parent.FindEntryNoInheritance(name);
    if (!child || !child->IsClass())
    {
        return nullptr;
    }

    return child->GetClassInterface();
}

int CountNestedClasses(const ParamEntry& entry)
{
    const ParamClass* cls = entry.GetClassInterface();
    if (!cls)
    {
        return 0;
    }

    int count = 0;
    for (int i = 0; i < cls->GetEntryCount(); ++i)
    {
        const ParamEntry& child = cls->GetEntry(i);
        if (!child.IsClass())
        {
            continue;
        }

        ++count;
        count += CountNestedClasses(child);
    }

    return count;
}

bool HasPlayerAssignment(const ParamEntry& entry)
{
    const ParamEntry* player = entry.FindEntryNoInheritance("player");
    if (player)
    {
        RStringB value = player->GetValue();
        if (!!value && value[0])
        {
            return true;
        }
    }

    const ParamClass* cls = entry.GetClassInterface();
    if (!cls)
    {
        return false;
    }

    for (int i = 0; i < cls->GetEntryCount(); ++i)
    {
        const ParamEntry& child = cls->GetEntry(i);
        if (child.IsClass() && HasPlayerAssignment(child))
        {
            return true;
        }
    }

    return false;
}

SectionLintResult LintMissionSection(const ParamFile& sqm, const char* sectionName)
{
    SectionLintResult result;
    result.name = sectionName;

    ParamEntry* sectionEntry = sqm.FindEntryNoInheritance(sectionName);
    if (!sectionEntry || !sectionEntry->IsClass())
    {
        return result;
    }

    const ParamClass* section = sectionEntry->GetClassInterface();
    result.parsed = true;

    if (const ParamClass* groups = GetChildClass(*section, "Groups"))
    {
        result.groupCount = groups->GetEntryCount();
        result.playerCount = HasPlayerAssignment(*groups) ? 1 : 0;
    }

    if (const ParamClass* vehicles = GetChildClass(*section, "Vehicles"))
    {
        result.vehicleCount = CountNestedClasses(*vehicles);
        if (result.playerCount == 0 && HasPlayerAssignment(*vehicles))
        {
            result.playerCount = 1;
        }
    }

    if (const ParamClass* sensors = GetChildClass(*section, "Sensors"))
    {
        result.sensorCount = CountNestedClasses(*sensors);
    }

    if (const ParamClass* markers = GetChildClass(*section, "Markers"))
    {
        result.markerCount = CountNestedClasses(*markers);
    }

    result.bootable = result.groupCount > 0 || result.playerCount > 0 || result.vehicleCount > 0;
    return result;
}

void PrintSection(const SectionLintResult& section)
{
    std::cout << section.name << ": ";
    if (!section.parsed)
    {
        std::cout << "missing or invalid\n";
        return;
    }

    std::cout << "parsed"
              << ", groups=" << section.groupCount << ", players=" << section.playerCount
              << ", vehicles=" << section.vehicleCount << ", sensors=" << section.sensorCount
              << ", markers=" << section.markerCount << ", bootable=" << (section.bootable ? "yes" : "no") << "\n";
}

} // namespace

void LintCommand::Setup(CLI::App& app)
{
    auto* lint = app.add_subcommand("lint", "Lint content files");
    lint->require_subcommand(1);

    auto* mission = lint->add_subcommand("mission", "Lint a mission folder (mission.sqm and description.ext)");
    static std::string missionPath;
    mission->add_option("path", missionPath, "Mission directory path")->required();
    mission->callback([&]() { std::exit(LintMission(missionPath)); });
}

int LintCommand::LintMission(const std::string& missionPath)
{
    fs::path dir = fs::u8path(missionPath);
    if (!fs::exists(dir) || !fs::is_directory(dir))
    {
        std::cerr << "Error: mission directory not found: " << missionPath << std::endl;
        return 1;
    }

    fs::path sqmPath = dir / "mission.sqm";
    if (!fs::exists(sqmPath))
    {
        std::cerr << "Error: mission.sqm not found: " << sqmPath.string() << std::endl;
        return 1;
    }

    bool ok = true;

    fs::path descriptionPath = dir / "description.ext";
    std::cout << "Mission: " << dir.filename().string() << "\n";
    std::cout << "Path: " << fs::absolute(dir).string() << "\n";

    if (fs::exists(descriptionPath))
    {
        ParamFile cfg;
        if (cfg.Parse(descriptionPath.string().c_str()) != LSOK)
        {
            std::cout << "description.ext: invalid\n";
            ok = false;
        }
        else
        {
            std::cout << "description.ext: parseable\n";
        }
    }
    else
    {
        std::cout << "description.ext: missing\n";
    }

    ParamFile sqm;
    if (sqm.Parse(sqmPath.string().c_str()) != LSOK)
    {
        std::cerr << "Error: mission.sqm is invalid: " << sqmPath.string() << std::endl;
        return 1;
    }

    std::vector<SectionLintResult> sections;
    sections.push_back(LintMissionSection(sqm, "Mission"));
    sections.push_back(LintMissionSection(sqm, "Intro"));
    sections.push_back(LintMissionSection(sqm, "OutroWin"));
    sections.push_back(LintMissionSection(sqm, "OutroLoose"));

    bool anyBootable = false;
    for (const SectionLintResult& section : sections)
    {
        PrintSection(section);
        anyBootable = anyBootable || section.bootable;
    }

    if (!anyBootable)
    {
        std::cout << "Result: no bootable mission/introduction/outro section found\n";
        ok = false;
    }
    else
    {
        std::cout << "Result: at least one bootable section found\n";
    }

    return ok ? 0 : 1;
}

} // namespace PoseidonTools
