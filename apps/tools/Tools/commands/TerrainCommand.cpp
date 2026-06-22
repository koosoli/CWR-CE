#include "TerrainCommand.hpp"
#include "../SDLPreview.hpp"
#include <Poseidon/World/Terrain/WrpReader.hpp>
#include <Poseidon/Asset/Probes/AssetInfo.hpp>
#include <Poseidon/Asset/Probes/AssetPreview.hpp>
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <vector>
#include <CLI/App.hpp>
#include <CLI/Option.hpp>
#include <CLI/Validators.hpp>
#include <cstdio>
#include <functional>
#include <string>
#include <Poseidon/Foundation/Math/Math3DP.hpp>

using namespace Poseidon;

#ifdef GetObject
#undef GetObject
#endif

namespace PoseidonTools
{

void TerrainCommand::Setup(CLI::App& app)
{
    auto* terrain = app.add_subcommand("terrain", "Inspect and analyze WRP terrain files");
    terrain->require_subcommand(1);
    {
        static std::string inputFile;
        auto* cmd = terrain->add_subcommand("inspect", "Show terrain file information");
        cmd->add_option("input", inputFile, "WRP terrain file")->required()->check(CLI::ExistingFile);
        cmd->callback(
            []()
            {
                auto info = Poseidon::InspectTerrain(inputFile);
                if (!info.valid)
                {
                    std::cerr << "Error: Failed to load terrain: " << inputFile << "\n";
                    std::exit(1);
                }

                std::cout << "Format:      " << info.formatName << "\n";
                std::cout << "Grid:        " << info.gridX << " x " << info.gridZ << "\n";
                std::cout << "Terrain:     " << info.terrainX << " x " << info.terrainZ << "\n";
                std::cout << "Heightmap:   " << info.heightmapSize << " cells\n";
                std::cout << "Elevation:   " << std::fixed << std::setprecision(1) << info.minHeight << " - "
                          << info.maxHeight << " m\n";
                std::cout << "Textures:    " << info.textureCount << "\n";
                std::cout << "Objects:     " << info.objectCount << "\n";
                if (info.objectNameCount > 0)
                    std::cout << "Obj classes: " << info.objectNameCount << "\n";

                std::exit(0);
            });
    }
    {
        static std::string inputFile;
        auto* cmd = terrain->add_subcommand("textures", "List terrain texture assignments");
        cmd->add_option("input", inputFile, "WRP terrain file")->required()->check(CLI::ExistingFile);
        cmd->callback(
            []()
            {
                WrpReader reader;
                if (!reader.Load(inputFile.c_str()))
                {
                    std::cerr << "Error: " << (reader.GetError() ? reader.GetError() : "Unknown error") << "\n";
                    std::exit(1);
                }

                if (reader.GetTextureCount() == 0)
                {
                    std::cout << "No textures.\n";
                    std::exit(0);
                }

                std::cout << std::left << std::setw(6) << "Index" << "Texture\n";
                std::cout << std::string(40, '-') << "\n";
                for (int i = 0; i < reader.GetTextureCount(); i++)
                {
                    std::cout << std::left << std::setw(6) << i << reader.GetTextureName(i).Data() << "\n";
                }

                std::exit(0);
            });
    }
    {
        static std::string inputFile;
        static std::vector<double> nearArg;
        auto* cmd = terrain->add_subcommand("objects", "List placed objects with positions");
        cmd->add_option("input", inputFile, "WRP terrain file")->required()->check(CLI::ExistingFile);
        cmd->add_option("--near", nearArg, "Only objects within R metres of map point (X,Z): --near X Z R")
            ->expected(3);
        cmd->callback(
            []()
            {
                WrpReader reader;
                if (!reader.Load(inputFile.c_str()))
                {
                    std::cerr << "Error: " << (reader.GetError() ? reader.GetError() : "Unknown error") << "\n";
                    std::exit(1);
                }

                if (reader.GetObjectCount() == 0)
                {
                    std::cout << "No objects.\n";
                    std::exit(0);
                }

                // --near X Z R: horizontal (X,Z) distance filter — Y is height. r2 < 0 disables.
                const bool filter = nearArg.size() == 3;
                const double nx = filter ? nearArg[0] : 0.0;
                const double nz = filter ? nearArg[1] : 0.0;
                const double r2 = filter ? nearArg[2] * nearArg[2] : -1.0;

                std::cout << std::left << std::setw(8) << "ID" << std::setw(40) << "Class" << std::setw(12) << "X"
                          << std::setw(12) << "Y" << std::setw(12) << "Z"
                          << "\n";
                std::cout << std::string(84, '-') << "\n";

                int shown = 0;
                for (int i = 0; i < reader.GetObjectCount(); i++)
                {
                    const auto& obj = reader.GetObject(i);
                    if (filter)
                    {
                        const double dx = obj.position.X() - nx;
                        const double dz = obj.position.Z() - nz;
                        if (dx * dx + dz * dz > r2)
                            continue;
                    }
                    std::cout << std::left << std::setw(8) << obj.id << std::setw(40) << obj.name.Data() << std::fixed
                              << std::setprecision(1) << std::setw(12) << obj.position.X() << std::setw(12)
                              << obj.position.Y() << std::setw(12) << obj.position.Z() << "\n";
                    shown++;
                }

                if (filter)
                    std::cout << "\n"
                              << shown << " object(s) within " << nearArg[2] << " m of (" << nx << ", " << nz << ").\n";

                std::exit(0);
            });
    }
    {
        static std::string inputFile;
        static std::string outputFile;
        auto* cmd = terrain->add_subcommand("render", "Render heightmap to PNG image");
        cmd->add_option("input", inputFile, "WRP terrain file")->required()->check(CLI::ExistingFile);
        cmd->add_option("-o,--output", outputFile, "Output PNG file path")->required();
        cmd->callback(
            []()
            {
                auto preview = Poseidon::PreviewTerrain(inputFile);
                if (!preview.valid())
                {
                    std::cerr << "Error: Failed to load terrain: " << inputFile << "\n";
                    std::exit(1);
                }

                if (!preview.saveToFile(outputFile))
                {
                    std::cerr << "Error: Failed to write output" << "\n";
                    std::exit(1);
                }

                auto info = Poseidon::InspectTerrain(inputFile);
                std::cout << "Rendered: " << outputFile << " (" << preview.width << "x" << preview.height << ")"
                          << "\n";
                if (info.valid)
                    std::cout << "Elevation: " << std::fixed << std::setprecision(1) << info.minHeight << " - "
                              << info.maxHeight << " m" << "\n";
                std::exit(0);
            });
    }
    {
        static std::string inputFile;
        static std::string screenshotPath;
        auto* cmd = terrain->add_subcommand("show", "Display heightmap in an SDL window");
        cmd->add_option("input", inputFile, "WRP terrain file")->required()->check(CLI::ExistingFile);
        cmd->add_option("--screenshot", screenshotPath, "Save screenshot to file and exit");
        cmd->callback(
            []()
            {
                auto preview = Poseidon::PreviewTerrain(inputFile);
                if (!preview.valid())
                {
                    std::cerr << "Error: Failed to load terrain: " << inputFile << "\n";
                    std::exit(1);
                }

                int w = preview.width;
                int h = preview.height;

                if (!screenshotPath.empty())
                {
                    if (!preview.saveToFile(screenshotPath))
                    {
                        std::cerr << "Error: Failed to write screenshot" << "\n";
                        std::exit(1);
                    }
                    std::cout << "Screenshot: " << screenshotPath << " (" << w << "x" << h << ")" << "\n";
                    std::exit(0);
                }

                auto info = Poseidon::InspectTerrain(inputFile);
                char title[256];
                std::snprintf(title, sizeof(title), "Terrain - %s (%dx%d, %.1f-%.1fm)", inputFile.c_str(), w, h,
                              info.minHeight, info.maxHeight);
                DisplayWindowRGBA(title, w, h, preview.data.data());
                std::exit(0);
            });
    }
}

} // namespace PoseidonTools
