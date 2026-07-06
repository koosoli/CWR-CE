#pragma once

#include <PoseidonVK/VertexLayoutVK.hpp>

#include <cstdint>
#include <vector>

namespace Poseidon::vk
{

// Scene-mesh vertex matching the SVertex contract (pos, norm, uv) that the
// Vulkan scene pipeline reads via VertexLayoutVK. Kept here so the mesh
// builder is self-contained and device-free.
struct MeshVertexVK
{
    float position[3] = {};
    float normal[3] = {};
    float texcoord[2] = {};
};

static_assert(sizeof(MeshVertexVK) == kSceneVertexStride);

// Built CPU-side mesh data ready to upload into Vulkan vertex/index buffers.
// Produced by BuildMeshBuffersVK from any shape-like source.
struct MeshBuffersVK
{
    std::vector<MeshVertexVK> vertices;
    std::vector<std::uint16_t> indices;
};

// Count the triangulated index count of a shape-like source (fan
// triangulation: an N-gon yields N-2 triangles = (N-2)*3 indices). Mirrors the
// GL33 index-count pass in VertexBufferGL33::Init.
template <typename ShapeLike>
inline std::uint32_t CountMeshIndicesVK(const ShapeLike& shape) noexcept
{
    std::uint32_t indices = 0;
    for (auto o = shape.BeginFaces(); o < shape.EndFaces(); shape.NextFace(o))
    {
        const int n = shape.Face(o).N();
        if (n >= 3)
            indices += static_cast<std::uint32_t>((n - 2) * 3);
    }
    return indices;
}

// Build the CPU-side vertex + index arrays from a shape-like source.
//
// The source must expose (matching Poseidon::Shape / VertexTable):
//   int NVertex() const;
//   Pos(i) -> { X(), Y(), Z() } world position
//   Norm(i) -> { X(), Y(), Z() } world normal
//   UV(i) -> { u, v } texcoord
//   BeginFaces()/EndFaces()/NextFace(o) face iteration
//   Face(o).N() / Face(o).GetVertex(i) polygon access
//
// Normals are negated to match the D3D11/GL33 convention used by the existing
// backends (see VertexBufferGL33::CopyVertices). Index generation is fan
// triangulation identical to VertexBufferGL33::Init.
template <typename ShapeLike>
inline MeshBuffersVK BuildMeshBuffersVK(const ShapeLike& shape)
{
    MeshBuffersVK out;
    const int vertexCount = shape.NVertex();
    if (vertexCount <= 0)
        return out;

    out.vertices.resize(static_cast<std::size_t>(vertexCount));
    for (int i = 0; i < vertexCount; ++i)
    {
        MeshVertexVK& v = out.vertices[static_cast<std::size_t>(i)];
        const auto& pos = shape.Pos(i);
        const auto& norm = shape.Norm(i);
        const auto& uv = shape.UV(i);
        v.position[0] = pos.X();
        v.position[1] = pos.Y();
        v.position[2] = pos.Z();
        // Normals negated to match the D3D11/GL33 convention.
        v.normal[0] = -norm.X();
        v.normal[1] = -norm.Y();
        v.normal[2] = -norm.Z();
        v.texcoord[0] = uv.u;
        v.texcoord[1] = uv.v;
    }

    out.indices.reserve(CountMeshIndicesVK(shape));
    for (auto o = shape.BeginFaces(); o < shape.EndFaces(); shape.NextFace(o))
    {
        const auto& poly = shape.Face(o);
        const int n = poly.N();
        for (int i = 2; i < n; ++i)
        {
            out.indices.push_back(static_cast<std::uint16_t>(poly.GetVertex(0)));
            out.indices.push_back(static_cast<std::uint16_t>(poly.GetVertex(i - 1)));
            out.indices.push_back(static_cast<std::uint16_t>(poly.GetVertex(i)));
        }
    }
    return out;
}

} // namespace Poseidon::vk
