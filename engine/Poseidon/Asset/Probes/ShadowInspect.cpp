#include <Poseidon/Asset/Probes/ShadowInspect.hpp>

#include <Poseidon/Graphics/Shadow/ShadowMath.hpp>
#include <Poseidon/World/Model/Model.hpp>
#include <Poseidon/World/Model/ModelCache.hpp>

#include <algorithm>
#include <cmath>
#include <stddef.h>
#include <memory>

namespace Poseidon
{

namespace
{
constexpr float kPi = 3.14159265358979323846f;

shadow::Vec3 SunDirection(float azDeg, float elDeg)
{
    float el = std::max(1.0f, std::min(89.0f, elDeg)) * kPi / 180.0f;
    float az = azDeg * kPi / 180.0f;
    // Direction the light travels (from the sun toward the ground), Y up.
    float ce = std::cos(el);
    return shadow::Normalize({-ce * std::sin(az), -std::sin(el), -ce * std::cos(az)});
}

uint8_t ToByte(float v01)
{
    float c = std::max(0.0f, std::min(1.0f, v01));
    return static_cast<uint8_t>(c * 255.0f + 0.5f);
}
} // namespace

ShadowInspectResult InspectShadow(const std::string& p3dPath, const ShadowInspectOptions& opts)
{
    ShadowInspectResult r;

    ModelCache cache;
    std::shared_ptr<Model::Model> model = cache.load(p3dPath);
    if (!model)
    {
        r.error = "failed to load model: " + p3dPath;
        return r;
    }
    if (!model->isCompiled())
    {
        model->compile();
    }
    int nLods = static_cast<int>(model->lodLevels.size());
    if (nLods == 0)
    {
        r.error = "model has no LODs";
        return r;
    }

    int lod = std::max(0, std::min(opts.lodIndex, nLods - 1));
    r.lodUsed = lod;
    r.resolution = model->lodLevels[lod].resolution;

    const Model::Mesh& mesh = model->lodLevels[lod].mesh;
    const int nv = static_cast<int>(mesh.vertices.size());
    r.vertexCount = nv;
    if (nv == 0)
    {
        r.error = "selected LOD has no vertices";
        return r;
    }

    auto pos = [&](uint32_t i) -> shadow::Vec3
    {
        const Model::Vector3& p = mesh.vertices[i].position;
        return {p.x, p.y, p.z};
    };
    auto valid = [&](uint32_t i) { return i < static_cast<uint32_t>(nv); };

    std::vector<shadow::Tri> tris;
    tris.reserve(mesh.triangles.size() + mesh.quads.size() * 2);
    for (const Model::Triangle& t : mesh.triangles)
    {
        if (valid(t.indices[0]) && valid(t.indices[1]) && valid(t.indices[2]))
        {
            tris.push_back({pos(t.indices[0]), pos(t.indices[1]), pos(t.indices[2])});
        }
    }
    for (const Model::Quad& q : mesh.quads)
    {
        if (valid(q.indices[0]) && valid(q.indices[1]) && valid(q.indices[2]) && valid(q.indices[3]))
        {
            tris.push_back({pos(q.indices[0]), pos(q.indices[1]), pos(q.indices[2])});
            tris.push_back({pos(q.indices[0]), pos(q.indices[2]), pos(q.indices[3])});
        }
    }
    r.triangleCount = static_cast<int>(tris.size());
    if (tris.empty())
    {
        r.error = "selected LOD has no faces";
        return r;
    }

    // Model AABB from the actual vertices.
    shadow::Vec3 mn = pos(0);
    shadow::Vec3 mx = mn;
    for (int i = 1; i < nv; i++)
    {
        shadow::Vec3 p = pos(static_cast<uint32_t>(i));
        mn = shadow::Min(mn, p);
        mx = shadow::Max(mx, p);
    }
    r.modelMin[0] = mn.x;
    r.modelMin[1] = mn.y;
    r.modelMin[2] = mn.z;
    r.modelMax[0] = mx.x;
    r.modelMax[1] = mx.y;
    r.modelMax[2] = mx.z;

    // Ground receiver plane sits at the model's base (min Y), centred on its XZ.
    float groundY = mn.y;
    float cx = (mn.x + mx.x) * 0.5f;
    float cz = (mn.z + mx.z) * 0.5f;
    float halfX = (mx.x - mn.x) * 0.5f;
    float halfZ = (mx.z - mn.z) * 0.5f;
    float half = std::max({halfX, halfZ, 0.5f}) * std::max(1.0f, opts.groundMargin);

    shadow::Vec3 sunDir = SunDirection(opts.sunAzDeg, opts.sunElDeg);
    shadow::Vec3 up = (std::fabs(sunDir.y) > 0.99f) ? shadow::Vec3{0.0f, 0.0f, 1.0f} : shadow::Vec3{0.0f, 1.0f, 0.0f};
    shadow::Mat4 lightView = shadow::LightView(sunDir, up);

    // Fit the ortho frustum over both the caster (model AABB) and the receivers
    // (ground plane corners), then snap to the texel grid.
    std::vector<shadow::Vec3> scenePts;
    scenePts.reserve(12);
    for (int xi = 0; xi < 2; xi++)
    {
        for (int yi = 0; yi < 2; yi++)
        {
            for (int zi = 0; zi < 2; zi++)
            {
                scenePts.push_back({xi ? mx.x : mn.x, yi ? mx.y : mn.y, zi ? mx.z : mn.z});
            }
        }
    }
    scenePts.push_back({cx - half, groundY, cz - half});
    scenePts.push_back({cx + half, groundY, cz - half});
    scenePts.push_back({cx + half, groundY, cz + half});
    scenePts.push_back({cx - half, groundY, cz + half});

    int shadowRes = std::max(16, opts.shadowRes);
    shadow::OrthoFit fit = shadow::FitOrtho(lightView, scenePts.data(), static_cast<int>(scenePts.size()));
    fit = shadow::SnapToTexelGrid(fit, shadowRes);
    shadow::Mat4 lightVP = shadow::Mul(fit.proj, lightView);

    shadow::DepthMap depthMap = shadow::CpuRasterDepth(tris.data(), static_cast<int>(tris.size()), lightVP, shadowRes);

    float texelWorld = (fit.maxB.x - fit.minB.x) / static_cast<float>(shadowRes);
    shadow::Vec3 groundNormal{0.0f, 1.0f, 0.0f};
    shadow::Bias bias = shadow::ShadowBias(groundNormal, sunDir, texelWorld);
    float depthBias = bias.depthBias * std::max(0.0f, opts.biasScale);

    int grid = std::max(2, opts.groundGrid);
    int shadowed = 0;
    if (opts.wantImages)
    {
        r.groundW = grid;
        r.groundH = grid;
        r.groundGray.assign(static_cast<size_t>(grid) * grid, 0);
    }
    for (int gz = 0; gz < grid; gz++)
    {
        for (int gx = 0; gx < grid; gx++)
        {
            float fx = (static_cast<float>(gx) + 0.5f) / static_cast<float>(grid);
            float fz = (static_cast<float>(gz) + 0.5f) / static_cast<float>(grid);
            shadow::Vec3 worldP{cx - half + 2.0f * half * fx, groundY, cz - half + 2.0f * half * fz};
            shadow::Vec3 sampleP = worldP + groundNormal * bias.normalOffset;
            float v = shadow::SampleShadow(depthMap, lightVP, sampleP, depthBias, 1);
            if (v < 0.5f)
            {
                shadowed++;
            }
            if (opts.wantImages)
            {
                r.groundGray[static_cast<size_t>(gz) * grid + gx] = ToByte(v);
            }
        }
    }
    r.shadowedFraction = static_cast<float>(shadowed) / static_cast<float>(grid * grid);

    if (opts.wantImages)
    {
        r.depthW = depthMap.w;
        r.depthH = depthMap.h;
        r.depthGray.assign(static_cast<size_t>(depthMap.w) * depthMap.h, 0);
        for (size_t i = 0; i < r.depthGray.size(); i++)
        {
            float d = depthMap.depth[i];
            // Empty texels (1.0) stay dark; closer surfaces are brighter.
            r.depthGray[i] = (d >= 0.999f) ? static_cast<uint8_t>(35) : ToByte(0.15f + (1.0f - d) * 0.85f);
        }
    }

    r.ok = true;
    return r;
}

} // namespace Poseidon
