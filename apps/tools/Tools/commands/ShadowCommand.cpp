#include "ShadowCommand.hpp"

#include <Poseidon/Asset/Probes/ShadowInspect.hpp>
#include <Poseidon/Graphics/Shared/PNGWriter.hpp>

#include <iostream>
#include <string>
#include <vector>
#include <CLI/App.hpp>
#include <CLI/Error.hpp>
#include <CLI/Option.hpp>
#include <CLI/Validators.hpp>
#include <functional>

namespace PoseidonTools
{

namespace
{

static void setupShadowInspect(CLI::App& shadow)
{
    auto* cmd = shadow.add_subcommand(
        "inspect", "Render a P3D's depth from the sun and sample the ground shadow factor (offline, no GL)");

    static std::string inputPath;
    static std::string outPath;
    static std::string depthPath;
    static std::vector<double> sun;
    static int lod = 0;
    static int res = 512;
    static int grid = 128;
    static double margin = 2.0;
    static double biasScale = 1.5;

    cmd->add_option("input", inputPath, "Input P3D model file")->required()->check(CLI::ExistingFile);
    cmd->add_option("--sun", sun, "Sun azimuth,elevation in degrees (default 315 35)")->expected(2);
    cmd->add_option("-o,--out", outPath, "Write the ground shadow image (PNG)");
    cmd->add_option("--depth", depthPath, "Write the light depth map (PNG)");
    cmd->add_option("--lod", lod, "Caster LOD index (default 0)");
    cmd->add_option("--res", res, "Depth-map resolution (default 512)");
    cmd->add_option("--grid", grid, "Ground sample grid (default 128)");
    cmd->add_option("--margin", margin, "Ground half-extent multiplier (default 2.0)");
    cmd->add_option("--bias-scale", biasScale, "Depth-bias multiplier (default 1.5)");

    cmd->callback(
        []()
        {
            Poseidon::ShadowInspectOptions opts;
            if (sun.size() == 2)
            {
                opts.sunAzDeg = static_cast<float>(sun[0]);
                opts.sunElDeg = static_cast<float>(sun[1]);
            }
            opts.lodIndex = lod;
            opts.shadowRes = res;
            opts.groundGrid = grid;
            opts.groundMargin = static_cast<float>(margin);
            opts.biasScale = static_cast<float>(biasScale);
            opts.wantImages = !outPath.empty() || !depthPath.empty();

            Poseidon::ShadowInspectResult r = Poseidon::InspectShadow(inputPath, opts);
            if (!r.ok)
            {
                std::cerr << "Error: " << r.error << "\n";
                throw CLI::RuntimeError(1);
            }

            std::cout << "LOD " << r.lodUsed << " (resolution " << r.resolution << ")\n";
            std::cout << "vertices=" << r.vertexCount << " triangles=" << r.triangleCount << "\n";
            std::cout << "bounds min=(" << r.modelMin[0] << "," << r.modelMin[1] << "," << r.modelMin[2] << ") max=("
                      << r.modelMax[0] << "," << r.modelMax[1] << "," << r.modelMax[2] << ")\n";
            std::cout << "shadowedFraction=" << r.shadowedFraction << "\n";

            if (!outPath.empty())
            {
                if (!Poseidon::PNGWriter::WritePNG(outPath.c_str(), r.groundW, r.groundH, 1, r.groundGray.data()))
                {
                    std::cerr << "Error: failed to write " << outPath << "\n";
                    throw CLI::RuntimeError(1);
                }
                std::cout << "ground: " << outPath << " (" << r.groundW << "x" << r.groundH << ")\n";
            }
            if (!depthPath.empty())
            {
                if (!Poseidon::PNGWriter::WritePNG(depthPath.c_str(), r.depthW, r.depthH, 1, r.depthGray.data()))
                {
                    std::cerr << "Error: failed to write " << depthPath << "\n";
                    throw CLI::RuntimeError(1);
                }
                std::cout << "depth: " << depthPath << " (" << r.depthW << "x" << r.depthH << ")\n";
            }
        });
}

} // namespace

void ShadowCommand::Setup(CLI::App& app)
{
    auto* shadow = app.add_subcommand("shadow", "Shadow-map tools");
    shadow->require_subcommand(1);
    setupShadowInspect(*shadow);
}

} // namespace PoseidonTools
