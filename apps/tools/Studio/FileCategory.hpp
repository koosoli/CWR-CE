#pragma once
#include <algorithm>
#include <string>

namespace Poseidon
{

enum class FileCategory
{
    All,
    Models,
    Textures,
    Sounds,
    Animations,
    Scripts,
    Configs,
    Archives,
    Terrains,
    Fonts,
    Other
};

inline const char* FileCategoryName(FileCategory cat)
{
    switch (cat)
    {
        case FileCategory::All:
            return "All";
        case FileCategory::Models:
            return "Models";
        case FileCategory::Textures:
            return "Textures";
        case FileCategory::Sounds:
            return "Sounds";
        case FileCategory::Animations:
            return "Animations";
        case FileCategory::Scripts:
            return "Scripts";
        case FileCategory::Configs:
            return "Configs";
        case FileCategory::Archives:
            return "Archives";
        case FileCategory::Terrains:
            return "Terrains";
        case FileCategory::Fonts:
            return "Fonts";
        case FileCategory::Other:
            return "Other";
    }
    return "?";
}

constexpr int FileCategoryCount = 11;

inline FileCategory CategorizeExtension(const std::string& ext)
{
    if (ext.empty())
        return FileCategory::Other;

    std::string e = ext;
    std::transform(e.begin(), e.end(), e.begin(), ::tolower);

    if (e == ".p3d" || e == ".3ds" || e == ".obj")
        return FileCategory::Models;
    if (e == ".paa" || e == ".pac" || e == ".png" || e == ".bmp" || e == ".tga" || e == ".jpg" || e == ".jpeg")
        return FileCategory::Textures;
    if (e == ".wav" || e == ".wss" || e == ".ogg" || e == ".lip")
        return FileCategory::Sounds;
    if (e == ".rtm" || e == ".xob")
        return FileCategory::Animations;
    if (e == ".sqs" || e == ".sqf" || e == ".fsm")
        return FileCategory::Scripts;
    if (e == ".cpp" || e == ".hpp" || e == ".ext" || e == ".cfg" || e == ".bin")
        return FileCategory::Configs;
    if (e == ".pbo")
        return FileCategory::Archives;
    if (e == ".wrp")
        return FileCategory::Terrains;
    if (e == ".fxy")
        return FileCategory::Fonts;

    return FileCategory::Other;
}

} // namespace Poseidon
