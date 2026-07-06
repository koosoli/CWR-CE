#include <catch2/catch_test_macros.hpp>

#include <PoseidonVK/MeshBuilderVK.hpp>

#include <cstdint>
#include <initializer_list>
#include <vector>

namespace
{

// Minimal stub satisfying the duck-typed shape interface MeshBuilderVK needs.
// Each "vertex" carries a position, normal, uv; faces index into them.
struct StubVec
{
    float x, y, z;
    float X() const { return x; }
    float Y() const { return y; }
    float Z() const { return z; }
};
struct StubUV
{
    float u, v;
};

struct StubPoly
{
    int n = 0;
    std::uint16_t v[8] = {}; // generous; engine MaxPoly is large post-clip
    int N() const { return n; }
    std::uint16_t GetVertex(int i) const { return v[i]; }
};

StubPoly MakePoly(std::initializer_list<std::uint16_t> verts)
{
    StubPoly p;
    p.n = static_cast<int>(verts.size());
    int i = 0;
    for (std::uint16_t vv : verts)
        p.v[i++] = vv;
    return p;
}

// Offset is just an int index into the faces vector.
struct StubShape
{
    std::vector<StubVec> pos;
    std::vector<StubVec> norm;
    std::vector<StubUV> uv;
    std::vector<StubPoly> faces;

    int NVertex() const { return static_cast<int>(pos.size()); }
    StubVec Pos(int i) const { return pos[i]; }
    StubVec Norm(int i) const { return norm[i]; }
    StubUV UV(int i) const { return uv[i]; }

    int BeginFaces() const { return 0; }
    int EndFaces() const { return static_cast<int>(faces.size()); }
    void NextFace(int& o) const { ++o; }
    StubPoly Face(int o) const { return faces[o]; }
};

StubShape MakeQuadShape()
{
    StubShape s;
    s.pos = {{-1, -1, 0}, {1, -1, 0}, {1, 1, 0}, {-1, 1, 0}};
    s.norm = {{0, 0, 1}, {0, 0, 1}, {0, 0, 1}, {0, 0, 1}};
    s.uv = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
    // One quad (4-gon) -> fan triangulates to 2 triangles.
    s.faces.push_back(MakePoly({0, 1, 2, 3}));
    return s;
}

} // namespace

TEST_CASE("Vulkan mesh builder extracts vertices with negated normals", "[vulkan][mesh-builder]")
{
    const StubShape shape = MakeQuadShape();
    const Poseidon::vk::MeshBuffersVK buffers = Poseidon::vk::BuildMeshBuffersVK(shape);

    REQUIRE(buffers.vertices.size() == 4);
    // Positions pass through unchanged.
    CHECK(buffers.vertices[0].position[0] == -1.0f);
    CHECK(buffers.vertices[2].position[0] == 1.0f);
    CHECK(buffers.vertices[2].position[2] == 0.0f);
    // Normals are negated (D3D11/GL33 convention).
    CHECK(buffers.vertices[0].normal[2] == -1.0f);
    CHECK(buffers.vertices[3].normal[2] == -1.0f);
    // UVs pass through unchanged.
    CHECK(buffers.vertices[0].texcoord[0] == 0.0f);
    CHECK(buffers.vertices[2].texcoord[1] == 1.0f);
}

TEST_CASE("Vulkan mesh builder fan-triangulates an n-gon", "[vulkan][mesh-builder]")
{
    const StubShape shape = MakeQuadShape();
    const Poseidon::vk::MeshBuffersVK buffers = Poseidon::vk::BuildMeshBuffersVK(shape);

    // One 4-gon -> 2 triangles -> 6 indices.
    REQUIRE(buffers.indices.size() == 6);
    // Fan from vertex 0: (0,1,2) then (0,2,3).
    const std::uint16_t expected[] = {0, 1, 2, 0, 2, 3};
    for (std::size_t i = 0; i < 6; ++i)
        CHECK(buffers.indices[i] == expected[i]);
}

TEST_CASE("Vulkan mesh builder counts indices before building", "[vulkan][mesh-builder]")
{
    const StubShape shape = MakeQuadShape();
    CHECK(Poseidon::vk::CountMeshIndicesVK(shape) == 6);
}

TEST_CASE("Vulkan mesh builder triangulates a triangle and a pentagon", "[vulkan][mesh-builder]")
{
    StubShape s;
    s.pos = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {1, 1, 0}, {2, 0, 0}};
    s.norm.resize(5, {0, 0, 1});
    s.uv.resize(5, {0, 0});
    // Triangle -> 1 tri (3 indices).
    s.faces.push_back(MakePoly({0, 1, 2}));
    // Pentagon -> 3 tris (9 indices).
    s.faces.push_back(MakePoly({0, 1, 3, 4, 2}));

    CHECK(Poseidon::vk::CountMeshIndicesVK(s) == 12);
    const Poseidon::vk::MeshBuffersVK buffers = Poseidon::vk::BuildMeshBuffersVK(s);
    REQUIRE(buffers.indices.size() == 12);
    // Triangle fan: (0,1,2).
    CHECK(buffers.indices[0] == 0);
    CHECK(buffers.indices[1] == 1);
    CHECK(buffers.indices[2] == 2);
    // Pentagon fan from vertex 0: (0,1,3),(0,3,4),(0,4,2).
    CHECK(buffers.indices[3] == 0);
    CHECK(buffers.indices[4] == 1);
    CHECK(buffers.indices[5] == 3);
    CHECK(buffers.indices[6] == 0);
    CHECK(buffers.indices[7] == 3);
    CHECK(buffers.indices[8] == 4);
    CHECK(buffers.indices[9] == 0);
    CHECK(buffers.indices[10] == 4);
    CHECK(buffers.indices[11] == 2);
}

TEST_CASE("Vulkan mesh builder handles an empty shape", "[vulkan][mesh-builder]")
{
    StubShape s;
    CHECK(Poseidon::vk::CountMeshIndicesVK(s) == 0);
    const Poseidon::vk::MeshBuffersVK buffers = Poseidon::vk::BuildMeshBuffersVK(s);
    CHECK(buffers.vertices.empty());
    CHECK(buffers.indices.empty());
}

TEST_CASE("Vulkan mesh vertex matches the scene vertex stride", "[vulkan][mesh-builder]")
{
    STATIC_REQUIRE(sizeof(Poseidon::vk::MeshVertexVK) == Poseidon::vk::kSceneVertexStride);
    STATIC_REQUIRE(sizeof(Poseidon::vk::MeshVertexVK) == 32);
}
