// ModelRenderer.cpp - Render P3D model wireframes to pixel buffers

#include <Poseidon/Graphics/Textures/ModelRenderer.hpp>
#include <Poseidon/Graphics/Rendering/Shape/Shape.hpp>
#include <Poseidon/Asset/Formats/Common/FormatDetector.hpp>
#include <stdlib.h>
#include <exception>
#include <iterator>
#include <utility>
#include <Poseidon/Foundation/Containers/StreamArray.hpp>
#include <Poseidon/Foundation/Math/Math3DP.hpp>

namespace Poseidon
{
using Poseidon::Asset::Formats::FormatInfo;
using Poseidon::Asset::Formats::P3DFormatDetector;
} // namespace Poseidon
#include <Poseidon/Asset/Formats/P3D/ODOLLoader.hpp>
#include <Poseidon/Asset/Formats/P3D/MLODLoader.hpp>
#include <Poseidon/World/Model/Model.hpp>
#include <vector>
#include <set>
#include <algorithm>
#include <cmath>
#include <cfloat>
namespace Poseidon
{

} // namespace Poseidon
namespace Poseidon
{

static ModelRendererLogFunc g_mrLog = nullptr;
void SetModelRendererLog(ModelRendererLogFunc func)
{
    g_mrLog = func;
}
static void MRLog(const char* msg)
{
    if (g_mrLog)
        g_mrLog(msg);
}

struct ViewParams
{
    int axisU, axisV;
    bool flipU, flipV;
};

static ViewParams getViewParams(const std::string& view)
{
    if (view == "back")
        return {0, 1, true, false};
    if (view == "top")
        return {0, 2, false, false};
    if (view == "bottom")
        return {0, 2, false, true};
    if (view == "right")
        return {2, 1, true, false};
    if (view == "left")
        return {2, 1, false, false};
    return {0, 1, false, false}; // front
}

static void drawLine(std::vector<uint8_t>& buf, int w, int h, int x0, int y0, int x1, int y1, uint8_t r, uint8_t g,
                     uint8_t b)
{
    int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    for (;;)
    {
        if (x0 >= 0 && x0 < w && y0 >= 0 && y0 < h)
        {
            int idx = (y0 * w + x0) * 3;
            buf[idx] = r;
            buf[idx + 1] = g;
            buf[idx + 2] = b;
        }
        if (x0 == x1 && y0 == y1)
            break;
        int e2 = 2 * err;
        if (e2 >= dy)
        {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx)
        {
            err += dx;
            y0 += sy;
        }
    }
}

// Vertex accessor for Shape* (used by RenderModelWireframe)
struct ShapeVA
{
    Shape* lod;
    int count() const { return lod->NPos(); }
    float get(int i, int axis) const { return lod->Pos(i)[axis]; }
};

// Vertex accessor for Model::Mesh (lightweight, no engine globals needed)
struct MeshVA
{
    const Poseidon::Model::Mesh* mesh;
    int count() const { return static_cast<int>(mesh->vertices.size()); }
    float get(int i, int axis) const
    {
        const auto& p = mesh->vertices[i].position;
        return axis == 0 ? p.x : (axis == 1 ? p.y : p.z);
    }
};

template <typename VA>
static void renderOrthoT(const VA& va, const std::set<std::pair<int, int>>& edges, std::vector<uint8_t>& px, int bufW,
                         int bufH, int rx, int ry, int rw, int rh, const ViewParams& vp)
{
    int nv = va.count();
    if (nv == 0)
        return;
    float minU = FLT_MAX, maxU = -FLT_MAX, minV = FLT_MAX, maxV = -FLT_MAX;
    for (int i = 0; i < nv; ++i)
    {
        float u = va.get(i, vp.axisU), v = va.get(i, vp.axisV);
        if (u < minU)
            minU = u;
        if (u > maxU)
            maxU = u;
        if (v < minV)
            minV = v;
        if (v > maxV)
            maxV = v;
    }
    float rangeU = maxU - minU, rangeV = maxV - minV;
    if (rangeU < 1e-6f)
        rangeU = 1e-6f;
    if (rangeV < 1e-6f)
        rangeV = 1e-6f;
    int margin = 8;
    float scU = (rw - 2 * margin) / rangeU, scV = (rh - 2 * margin) / rangeV;
    float scale = (scU < scV) ? scU : scV;
    float offU = rx + margin + ((rw - 2 * margin) - rangeU * scale) * 0.5f;
    float offV = ry + margin + ((rh - 2 * margin) - rangeV * scale) * 0.5f;
    for (auto& [a, b] : edges)
    {
        if (a >= nv || b >= nv)
            continue;
        float aU = va.get(a, vp.axisU), aV = va.get(a, vp.axisV);
        float bU = va.get(b, vp.axisU), bV = va.get(b, vp.axisV);
        int x0 = (int)(offU + (vp.flipU ? maxU - aU : aU - minU) * scale);
        int y0 = (int)(offV + (vp.flipV ? aV - minV : maxV - aV) * scale);
        int x1 = (int)(offU + (vp.flipU ? maxU - bU : bU - minU) * scale);
        int y1 = (int)(offV + (vp.flipV ? bV - minV : maxV - bV) * scale);
        drawLine(px, bufW, bufH, x0, y0, x1, y1, 200, 220, 255);
    }
}

template <typename VA>
static void renderPerspT(const VA& va, const std::set<std::pair<int, int>>& edges, std::vector<uint8_t>& px, int bufW,
                         int bufH, int rx, int ry, int rw, int rh)
{
    int nv = va.count();
    if (nv == 0)
        return;
    float minB[3] = {FLT_MAX, FLT_MAX, FLT_MAX}, maxB[3] = {-FLT_MAX, -FLT_MAX, -FLT_MAX};
    for (int i = 0; i < nv; ++i)
        for (int a = 0; a < 3; a++)
        {
            float v = va.get(i, a);
            if (v < minB[a])
                minB[a] = v;
            if (v > maxB[a])
                maxB[a] = v;
        }
    float cx = (minB[0] + maxB[0]) * 0.5f, cy = (minB[1] + maxB[1]) * 0.5f, cz = (minB[2] + maxB[2]) * 0.5f;
    float radius = 0;
    for (int i = 0; i < nv; ++i)
    {
        float dx = va.get(i, 0) - cx, dy = va.get(i, 1) - cy, dz = va.get(i, 2) - cz;
        float r2 = dx * dx + dy * dy + dz * dz;
        if (r2 > radius)
            radius = r2;
    }
    radius = std::sqrt(radius);
    if (radius < 1e-6f)
        radius = 1.0f;

    constexpr float PI = 3.14159265f;
    float azim = 35 * PI / 180, elev = 25 * PI / 180, dist = radius * 2.8f;
    float camX = cx + std::sin(azim) * std::cos(elev) * dist;
    float camY = cy + std::sin(elev) * dist;
    float camZ = cz - std::cos(azim) * std::cos(elev) * dist;
    float fX = cx - camX, fY = cy - camY, fZ = cz - camZ;
    float fL = std::sqrt(fX * fX + fY * fY + fZ * fZ);
    fX /= fL;
    fY /= fL;
    fZ /= fL;
    float rX = -fZ, rY = 0, rZ = fX;
    float rL = std::sqrt(rX * rX + rZ * rZ);
    rX /= rL;
    rZ /= rL;
    float uX = rY * fZ - rZ * fY, uY = rZ * fX - rX * fZ, uZ = rX * fY - rY * fX;
    float fov = 45 * PI / 180;
    int mn = (rw < rh) ? rw : rh;
    float focal = (mn * 0.5f) / std::tan(fov * 0.5f);
    float scrCX = rx + rw * 0.5f, scrCY = ry + rh * 0.5f;

    for (auto& [a, b] : edges)
    {
        if (a >= nv || b >= nv)
            continue;
        float d0x = va.get(a, 0) - camX, d0y = va.get(a, 1) - camY, d0z = va.get(a, 2) - camZ;
        float dp0 = d0x * fX + d0y * fY + d0z * fZ;
        if (dp0 < 0.01f)
            continue;
        float d1x = va.get(b, 0) - camX, d1y = va.get(b, 1) - camY, d1z = va.get(b, 2) - camZ;
        float dp1 = d1x * fX + d1y * fY + d1z * fZ;
        if (dp1 < 0.01f)
            continue;
        int px0 = (int)(scrCX + focal * (d0x * rX + d0y * rY + d0z * rZ) / dp0);
        int py0 = (int)(scrCY - focal * (d0x * uX + d0y * uY + d0z * uZ) / dp0);
        int px1 = (int)(scrCX + focal * (d1x * rX + d1y * rY + d1z * rZ) / dp1);
        int py1 = (int)(scrCY - focal * (d1x * uX + d1y * uY + d1z * uZ) / dp1);
        drawLine(px, bufW, bufH, px0, py0, px1, py1, 200, 220, 255);
    }
}

template <typename VA>
static RenderedModel renderWireframeT(const VA& va, const std::set<std::pair<int, int>>& edges, int imgW, int imgH,
                                      const std::string& view, uint8_t bgR, uint8_t bgG, uint8_t bgB)
{
    RenderedModel result;
    if (va.count() == 0)
        return result;
    result.width = imgW;
    result.height = imgH;
    result.rgb.resize(imgW * imgH * 3);
    for (int i = 0; i < imgW * imgH; i++)
    {
        result.rgb[i * 3] = bgR;
        result.rgb[i * 3 + 1] = bgG;
        result.rgb[i * 3 + 2] = bgB;
    }
    if (view == "quad")
    {
        int hW = imgW / 2, hH = imgH / 2;
        for (int x = 0; x < imgW; x++)
        {
            int i = (hH * imgW + x) * 3;
            result.rgb[i] = result.rgb[i + 1] = result.rgb[i + 2] = 64;
        }
        for (int y = 0; y < imgH; y++)
        {
            int i = (y * imgW + hW) * 3;
            result.rgb[i] = result.rgb[i + 1] = result.rgb[i + 2] = 64;
        }
        renderOrthoT(va, edges, result.rgb, imgW, imgH, 0, 0, hW, hH, getViewParams("front"));
        renderOrthoT(va, edges, result.rgb, imgW, imgH, hW, 0, hW, hH, getViewParams("left"));
        renderOrthoT(va, edges, result.rgb, imgW, imgH, 0, hH, hW, hH, getViewParams("top"));
        renderPerspT(va, edges, result.rgb, imgW, imgH, hW, hH, hW, hH);
    }
    else if (view == "3d")
    {
        renderPerspT(va, edges, result.rgb, imgW, imgH, 0, 0, imgW, imgH);
    }
    else
    {
        renderOrthoT(va, edges, result.rgb, imgW, imgH, 0, 0, imgW, imgH, getViewParams(view));
    }
    return result;
}

// Collect edges from Shape*
static std::set<std::pair<int, int>> collectEdges(Shape* lod)
{
    std::set<std::pair<int, int>> edges;
    for (Offset o = lod->BeginFaces(); o < lod->EndFaces(); lod->NextFace(o))
    {
        const Poly& face = lod->Face(o);
        int n = face.N();
        for (int i = 0; i < n; ++i)
        {
            int a = face.GetVertex(i), b = face.GetVertex((i + 1) % n);
            if (a > b)
                std::swap(a, b);
            edges.insert({a, b});
        }
    }
    return edges;
}

// Collect edges from Model::Mesh directly (no Shape/engine globals needed)
static std::set<std::pair<int, int>> collectMeshEdges(const Poseidon::Model::Mesh& mesh)
{
    std::set<std::pair<int, int>> edges;
    for (const auto& tri : mesh.triangles)
    {
        for (int i = 0; i < 3; ++i)
        {
            int a = (int)tri.indices[i], b = (int)tri.indices[(i + 1) % 3];
            if (a > b)
                std::swap(a, b);
            edges.insert({a, b});
        }
    }
    for (const auto& q : mesh.quads)
    {
        for (int i = 0; i < 4; ++i)
        {
            int a = (int)q.indices[i], b = (int)q.indices[(i + 1) % 4];
            if (a > b)
                std::swap(a, b);
            edges.insert({a, b});
        }
    }
    return edges;
}

RenderedModel RenderModelWireframe(Shape* lod, int imgW, int imgH, const std::string& view, uint8_t bgR, uint8_t bgG,
                                   uint8_t bgB)
{
    if (!lod || lod->NPos() == 0)
        return {};
    ShapeVA va{lod};
    auto edges = collectEdges(lod);
    return renderWireframeT(va, edges, imgW, imgH, view, bgR, bgG, bgB);
}

// Render P3D directly from Model mesh data, no ShapeAdapter/engine globals needed
RenderedModel RenderP3DFile(const std::string& path, int imgW, int imgH, const std::string& view, int lodIndex,
                            uint8_t bgR, uint8_t bgG, uint8_t bgB)
{
    RenderedModel result;
    try
    {
        MRLog("DetectFormat");
        auto info = P3DFormatDetector::DetectFormat(path);
        MRLog(("sig=" + info.signature + " ok=" + (info.isSupported ? "y" : "n")).c_str());
        if (!info.isSupported)
            return result;

        Poseidon::Model::Model model;
        if (info.signature == "ODOL")
        {
            MRLog("ODOLLoader::load");
            model = Poseidon::Asset::Formats::ODOLLoader::load(path);
        }
        else if (info.signature == "MLOD")
        {
            MRLog("MLODLoader::load");
            model = Poseidon::Asset::Formats::MLODLoader::load(path);
        }
        else
        {
            return result;
        }
        MRLog("load done");

        model.compile();
        MRLog("compile done");

        int nLods = static_cast<int>(model.lodLevels.size());
        MRLog(("nLods=" + std::to_string(nLods)).c_str());
        if (lodIndex >= nLods)
            return result;

        const auto& mesh = model.lodLevels[lodIndex].mesh;
        int nv = static_cast<int>(mesh.vertices.size());
        MRLog(("verts=" + std::to_string(nv) + " tris=" + std::to_string(mesh.triangles.size()) +
               " quads=" + std::to_string(mesh.quads.size()))
                  .c_str());

        auto edges = collectMeshEdges(mesh);
        MRLog(("edges=" + std::to_string(edges.size())).c_str());

        MeshVA va{&mesh};
        result = renderWireframeT(va, edges, imgW, imgH, view, bgR, bgG, bgB);
        MRLog("render done");
    }
    catch (const std::exception& e)
    {
        MRLog(("EXCEPTION: " + std::string(e.what())).c_str());
    }
    catch (...)
    {
        MRLog("EXCEPTION: unknown");
    }
    return result;
}

} // namespace Poseidon
