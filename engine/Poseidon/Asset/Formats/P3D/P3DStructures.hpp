#pragma once

#include <Poseidon/Asset/Formats/BISBinaryStream.hpp>
#include <Poseidon/Asset/Formats/BISStructures.hpp>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>

namespace Poseidon::Asset::Formats
{
namespace P3D
{

// Read a list element count and reject one that cannot be backed by the remaining
// input (every element occupies at least one byte on the wire), so the reserve()
// and read loop below cannot be driven to a huge allocation by a malformed count.
inline uint32_t readListCount(BinaryReader& reader)
{
    uint32_t count = reader.read<uint32_t>();
    if (count > static_cast<uint32_t>(reader.remaining()))
        throw std::runtime_error("List count exceeds remaining input");
    return count;
}

// ODOL v7 file header (12 bytes).
// Signature "ODOL" (0x4C4F444F), version 7 for Arma: Cold War Assault.
struct P3DHeader
{
    char     signature[4] = {0}; // "ODOL"
    uint32_t version      = 0;
    uint32_t lodCount     = 0;

    bool isValid() const { return std::memcmp(signature, "ODOL", 4) == 0 && version == 7; }
    bool isODOL() const { return std::memcmp(signature, "ODOL", 4) == 0; }
};

inline P3DHeader readHeader(BinaryReader& reader)
{
    P3DHeader header;
    uint32_t  sig = reader.read<uint32_t>();
    std::memcpy(header.signature, &sig, 4);
    header.version  = reader.read<uint32_t>();
    header.lodCount = reader.read<uint32_t>();

    if (std::memcmp(header.signature, "ODOL", 4) != 0)
        throw std::runtime_error("Invalid P3D signature: expected 'ODOL', got '" + std::string(header.signature, 4) + "'");
    if (header.version != 7)
        throw std::runtime_error("Unsupported ODOL version: " + std::to_string(header.version) + " (only v7 supported)");
    if (header.lodCount == 0 || header.lodCount > 100)
        throw std::runtime_error("Invalid LOD count: " + std::to_string(header.lodCount) + " (expected 1-100)");

    return header;
}

// Vertex table: pre-computed clip flags, UV coords, positions, normals.
// Flags and UVs are compressed if byte size >= COMPRESSION_THRESHOLD.
struct VertexTable
{
    std::vector<uint32_t> clipFlags;
    std::vector<Vector2>  uvCoords;
    std::vector<Vector3>  points;
    std::vector<Vector3>  normals;
};

inline VertexTable readVertexTable(BinaryReader& reader)
{
    VertexTable vtable;
    vtable.clipFlags = readCompressedArray<uint32_t>(reader);
    vtable.uvCoords  = readCompressedArray<Vector2>(reader);
    vtable.points    = readArray<Vector3>(reader);
    vtable.normals   = readArray<Vector3>(reader);
    return vtable;
}

struct LODBounds
{
    int32_t orHints  = 0;
    int32_t andHints = 0;
    Vector3 minPos;
    Vector3 maxPos;
    Vector3 bCenter;
    float   bRadius = 0.0f;
};

inline LODBounds readLODBounds(BinaryReader& reader)
{
    LODBounds bounds;
    bounds.orHints    = reader.read<int32_t>();
    bounds.andHints   = reader.read<int32_t>();
    bounds.minPos.x   = reader.read<float>();
    bounds.minPos.y   = reader.read<float>();
    bounds.minPos.z   = reader.read<float>();
    bounds.maxPos.x   = reader.read<float>();
    bounds.maxPos.y   = reader.read<float>();
    bounds.maxPos.z   = reader.read<float>();
    bounds.bCenter.x  = reader.read<float>();
    bounds.bCenter.y  = reader.read<float>();
    bounds.bCenter.z  = reader.read<float>();
    bounds.bRadius    = reader.read<float>();
    return bounds;
}

struct Textures
{
    std::vector<std::string> texturePaths;
};

inline Textures readTextures(BinaryReader& reader)
{
    Textures  textures;
    uint32_t  count = readListCount(reader);
    textures.texturePaths.reserve(count);
    for (uint32_t i = 0; i < count; ++i)
        textures.texturePaths.push_back(reader.readAsciiz());
    return textures;
}

struct LODEdge
{
    std::vector<uint16_t> edges;
};

struct LODEdges
{
    LODEdge mlodIndex;   // lod_edge1
    LODEdge vertexIndex; // lod_edge2
};

inline LODEdge readLODEdge(BinaryReader& reader)
{
    LODEdge edge;
    edge.edges = readCompressedArray<uint16_t>(reader);
    return edge;
}

inline LODEdges readLODEdges(BinaryReader& reader)
{
    LODEdges edges;
    edges.mlodIndex   = readLODEdge(reader);
    edges.vertexIndex = readLODEdge(reader);
    return edges;
}

struct Face
{
    uint32_t              flags;
    int16_t               textureIndex;
    uint8_t               vertexCount;
    std::vector<uint16_t> vertexIndices;
};

inline Face readFace(BinaryReader& reader)
{
    Face face;
    face.flags       = reader.read<uint32_t>();
    face.textureIndex = reader.read<int16_t>();
    face.vertexCount  = reader.read<uint8_t>();
    face.vertexIndices.reserve(face.vertexCount);
    for (uint8_t i = 0; i < face.vertexCount; ++i)
        face.vertexIndices.push_back(reader.read<uint16_t>());
    return face;
}

struct Faces
{
    uint32_t         count;
    uint32_t         offsetToSections;
    std::vector<Face> faces;
};

inline Faces readFaces(BinaryReader& reader)
{
    Faces    faces;
    faces.count            = readListCount(reader);
    faces.offsetToSections = reader.read<uint32_t>();
    faces.faces.reserve(faces.count);
    for (uint32_t i = 0; i < faces.count; ++i)
        faces.faces.push_back(readFace(reader));
    return faces;
}

struct Section
{
    uint32_t faceIndexLowerBound;
    uint32_t faceIndexUpperBound;
    int32_t  material;
    int16_t  textureIndex;
    int32_t  special;
};

inline Section readSection(BinaryReader& reader)
{
    Section section;
    section.faceIndexLowerBound = reader.read<uint32_t>();
    section.faceIndexUpperBound = reader.read<uint32_t>();
    section.material            = reader.read<int32_t>();
    section.textureIndex        = reader.read<int16_t>();
    section.special             = reader.read<int32_t>();
    return section;
}

struct Sections
{
    std::vector<Section> sections;
};

inline Sections readSections(BinaryReader& reader)
{
    Sections sections;
    uint32_t count = readListCount(reader);
    sections.sections.reserve(count);
    for (uint32_t i = 0; i < count; ++i)
        sections.sections.push_back(readSection(reader));
    return sections;
}

struct NamedSelection
{
    std::string           name;
    std::vector<uint16_t> faceIndices;
    std::vector<uint8_t>  faceWeights;
    std::vector<uint32_t> faceSelectionIndices;
    std::vector<uint32_t> faceSelectionIndices2;
    bool                  needSelection;
    std::vector<uint16_t> vertexIndices;
    std::vector<uint8_t>  vertexWeights;
};

inline NamedSelection readNamedSelection(BinaryReader& reader)
{
    NamedSelection sel;
    sel.name                 = reader.readAsciiz();
    sel.faceIndices          = reader.readCompressedArray<uint16_t>();
    sel.faceWeights          = reader.readCompressedArray<uint8_t>();
    sel.faceSelectionIndices = reader.readCompressedArray<uint32_t>();
    sel.needSelection        = reader.read<uint8_t>() != 0;
    sel.faceSelectionIndices2 = reader.readCompressedArray<uint32_t>();
    sel.vertexIndices        = reader.readCompressedArray<uint16_t>();
    sel.vertexWeights        = reader.readCompressedArray<uint8_t>();
    return sel;
}

struct NamedSelections
{
    std::vector<NamedSelection> selections;
};

inline NamedSelections readNamedSelections(BinaryReader& reader)
{
    NamedSelections selections;
    uint32_t        count = readListCount(reader);
    selections.selections.reserve(count);
    for (uint32_t i = 0; i < count; ++i)
        selections.selections.push_back(readNamedSelection(reader));
    return selections;
}

struct NamedProperty
{
    std::string property;
    std::string value;
};

inline NamedProperty readNamedProperty(BinaryReader& reader)
{
    NamedProperty prop;
    prop.property = reader.readAsciiz();
    prop.value    = reader.readAsciiz();
    return prop;
}

struct NamedProperties
{
    std::vector<NamedProperty> properties;
};

inline NamedProperties readNamedProperties(BinaryReader& reader)
{
    NamedProperties properties;
    uint32_t        count = readListCount(reader);
    properties.properties.reserve(count);
    for (uint32_t i = 0; i < count; ++i)
        properties.properties.push_back(readNamedProperty(reader));
    return properties;
}

struct Frame
{
    float               frameTime;
    std::vector<Vector3> bonePositions;
};

inline Frame readFrame(BinaryReader& reader)
{
    Frame    frame;
    frame.frameTime = reader.read<float>();
    uint32_t boneCount = readListCount(reader);
    frame.bonePositions.reserve(boneCount);
    for (uint32_t i = 0; i < boneCount; ++i)
    {
        Vector3 pos;
        pos.x = reader.read<float>();
        pos.y = reader.read<float>();
        pos.z = reader.read<float>();
        frame.bonePositions.push_back(pos);
    }
    return frame;
}

struct Frames
{
    std::vector<Frame> frames;
};

inline Frames readFrames(BinaryReader& reader)
{
    Frames   frames;
    uint32_t count = readListCount(reader);
    frames.frames.reserve(count);
    for (uint32_t i = 0; i < count; ++i)
        frames.frames.push_back(readFrame(reader));
    return frames;
}

struct LODEndData
{
    uint32_t colorTop;
    uint32_t color;
    uint32_t special;
};

inline LODEndData readLODEndData(BinaryReader& reader)
{
    LODEndData data;
    data.colorTop = reader.read<uint32_t>();
    data.color    = reader.read<uint32_t>();
    data.special  = reader.read<uint32_t>();
    return data;
}

struct Proxy
{
    std::string name;
    Vector3     transform[4]; // [aside, up, dir, pos] — 4x3 matrix as 4 Vector3s
    uint32_t    sequenceId;
    uint32_t    namedSelectionIndex;
};

inline Proxy readProxy(BinaryReader& reader)
{
    Proxy proxy;
    proxy.name = reader.readAsciiz();
    for (int i = 0; i < 4; ++i)
    {
        proxy.transform[i].x = reader.read<float>();
        proxy.transform[i].y = reader.read<float>();
        proxy.transform[i].z = reader.read<float>();
    }
    proxy.sequenceId          = reader.read<uint32_t>();
    proxy.namedSelectionIndex = reader.read<uint32_t>();
    return proxy;
}

struct Proxies
{
    std::vector<Proxy> proxies;
};

inline Proxies readProxies(BinaryReader& reader)
{
    Proxies  proxies;
    uint32_t count = readListCount(reader);
    proxies.proxies.reserve(count);
    for (uint32_t i = 0; i < count; ++i)
        proxies.proxies.push_back(readProxy(reader));
    return proxies;
}

struct CompleteLOD
{
    VertexTable      vertexTable;
    LODBounds        bounds;
    Textures         textures;
    LODEdges         edges;
    Faces            faces;
    Sections         sections;
    NamedSelections  namedSelections;
    NamedProperties  namedProperties;
    Frames           frames;
    LODEndData       endData;
    Proxies          proxies;
};

inline CompleteLOD readCompleteLOD(BinaryReader& reader)
{
    CompleteLOD lod;
    lod.vertexTable      = readVertexTable(reader);
    lod.bounds           = readLODBounds(reader);
    lod.textures         = readTextures(reader);
    lod.edges            = readLODEdges(reader);
    lod.faces            = readFaces(reader);
    lod.sections         = readSections(reader);
    lod.namedSelections  = readNamedSelections(reader);
    lod.namedProperties  = readNamedProperties(reader);
    lod.frames           = readFrames(reader);
    lod.endData          = readLODEndData(reader);
    lod.proxies          = readProxies(reader);
    return lod;
}

struct Model
{
    P3DHeader              header;
    std::vector<CompleteLOD> lods;
    std::vector<float>     resolutions; // per-LOD, read after LODs

    // Model-wide metadata (field order matches LODShape::SerializeBin in shapeFile.cpp)
    int32_t  special       = 0;
    float    boundingSphere = 0.0f;
    float    geometrySphere = 0.0f;
    int32_t  remarks       = 0;
    int32_t  andHints      = 0;
    int32_t  orHints       = 0;
    Vector3  aimingCenter;
    uint32_t color         = 0xFFFFFFFF;
    uint32_t colorTop      = 0xFFFFFFFF;
    float    viewDensity   = 0.0f;
    Vector3  minMax[2];
    Vector3  boundingCenter;
    Vector3  geometryCenter;
    Vector3  centerOfMass;
    float    invInertia[9] = {0};
    bool     autoCenter    = false;
    bool     lockAutoCenter = false;
    bool     canOcclude    = false;
    bool     canBeOccluded = false;
    bool     allowAnimation = false;
    int8_t   mapType       = 0;

    // Mass/armor (shapeFile.cpp:541-547)
    std::vector<float> massArray;
    float              mass     = 0.0f;
    float              invMass  = 0.0f;
    float              armor    = 0.0f;
    float              invArmor = 0.0f;

    // LOD indices (shapeFile.cpp:560-571)
    int8_t memory                = -1;
    int8_t geometry              = -1;
    int8_t geometryFire          = -1;
    int8_t geometryView          = -1;
    int8_t geometryViewPilot     = -1;
    int8_t geometryViewGunner    = -1;
    int8_t geometryViewCommander = -1;
    int8_t geometryViewCargo     = -1;
    int8_t landContact           = -1;
    int8_t roadway               = -1;
    int8_t paths                 = -1;
    int8_t hitpoints             = -1;
};

inline Model readModel(BinaryReader& reader)
{
    Model model;
    model.header = readHeader(reader);

    model.lods.reserve(model.header.lodCount);
    for (uint32_t i = 0; i < model.header.lodCount; ++i)
        model.lods.push_back(readCompleteLOD(reader));

    model.resolutions.reserve(model.header.lodCount);
    for (uint32_t i = 0; i < model.header.lodCount; ++i)
        model.resolutions.push_back(reader.read<float>());

    auto readVec3 = [&reader]() -> Vector3
    {
        Vector3 v;
        reader.readBytes(&v, sizeof(v));
        return v;
    };

    model.special          = reader.read<int32_t>();
    model.boundingSphere   = reader.read<float>();
    model.geometrySphere   = reader.read<float>();
    model.remarks          = reader.read<int32_t>();
    model.andHints         = reader.read<int32_t>();
    model.orHints          = reader.read<int32_t>();
    model.aimingCenter     = readVec3();
    model.color            = reader.read<uint32_t>();
    model.colorTop         = reader.read<uint32_t>();
    model.viewDensity      = reader.read<float>();
    model.minMax[0]        = readVec3();
    model.minMax[1]        = readVec3();
    model.boundingCenter   = readVec3();
    model.geometryCenter   = readVec3();
    model.centerOfMass     = readVec3();
    reader.readBytes(model.invInertia, sizeof(model.invInertia));
    model.autoCenter       = reader.read<bool>();
    model.lockAutoCenter   = reader.read<bool>();
    model.canOcclude       = reader.read<bool>();
    model.canBeOccluded    = reader.read<bool>();
    model.allowAnimation   = reader.read<bool>();
    model.mapType          = reader.read<int8_t>();

    model.massArray        = reader.readCompressedArray<float>();
    model.mass             = reader.read<float>();
    model.invMass          = reader.read<float>();
    model.armor            = reader.read<float>();
    model.invArmor         = reader.read<float>();

    model.memory                = reader.read<int8_t>();
    model.geometry              = reader.read<int8_t>();
    model.geometryFire          = reader.read<int8_t>();
    model.geometryView          = reader.read<int8_t>();
    model.geometryViewPilot     = reader.read<int8_t>();
    model.geometryViewGunner    = reader.read<int8_t>();
    model.geometryViewCommander = reader.read<int8_t>();
    model.geometryViewCargo     = reader.read<int8_t>();
    model.landContact           = reader.read<int8_t>();
    model.roadway               = reader.read<int8_t>();
    model.paths                 = reader.read<int8_t>();
    model.hitpoints             = reader.read<int8_t>();

    return model;
}

} // namespace P3D
} // namespace Poseidon::Asset::Formats
