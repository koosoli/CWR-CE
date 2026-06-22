#pragma once

#include <Poseidon/Asset/Formats/BISBinaryStream.hpp>
#include <Poseidon/Asset/Formats/BISStructures.hpp>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>

namespace Poseidon::Asset::Formats
{
namespace MLOD
{

// Reject an element count that cannot be backed by the remaining input (each
// element occupies at least one byte on the wire) before it drives a reserve/
// resize into a huge allocation. MLOD counts come straight from the file.
inline void validateCount(BinaryReader& reader, int32_t count, const char* what)
{
    if (count < 0 || count > reader.remaining())
        throw std::runtime_error(std::string("MLOD ") + what + " count out of range");
}

// MLOD file header (12 bytes).
// Signature "MLOD" (0x4D4C4F44), version encoded as major.minor bytes.
struct MLODHeader
{
    char     signature[4] = {0}; // "MLOD"
    uint8_t  versionMajor = 0;
    uint8_t  versionMinor = 0;
    uint16_t padding      = 0;
    uint32_t lodCount     = 0;

    bool isValid() const { return std::memcmp(signature, "MLOD", 4) == 0; }
    bool isMLOD() const { return std::memcmp(signature, "MLOD", 4) == 0; }
};

inline MLODHeader readHeader(BinaryReader& reader)
{
    MLODHeader header;
    uint32_t   sig = reader.read<uint32_t>();
    std::memcpy(header.signature, &sig, 4);
    header.versionMajor = reader.read<uint8_t>();
    header.versionMinor = reader.read<uint8_t>();
    header.padding      = reader.read<uint16_t>();
    header.lodCount     = reader.read<uint32_t>();

    if (std::memcmp(header.signature, "MLOD", 4) != 0)
        throw std::runtime_error("Invalid MLOD signature: expected 'MLOD', got '" + std::string(header.signature, 4) + "'");
    if (header.versionMajor == 0 || header.versionMajor > 10)
        throw std::runtime_error("Invalid MLOD version: " + std::to_string(header.versionMajor) + "." + std::to_string(header.versionMinor));
    if (header.lodCount == 0 || header.lodCount > 100)
        throw std::runtime_error("Invalid LOD count: " + std::to_string(header.lodCount) + " (expected 1-100)");

    return header;
}

// SP3X section header (binary vertex/face data).
// headSize covers all header bytes; fields after the first 28 are skipped.
struct SP3XSection
{
    char    signature[4] = {0}; // "SP3X"
    int32_t headSize     = 0;
    int32_t version      = 0;
    int32_t nPos         = 0; // number of vertex positions
    int32_t nNorm        = 0; // number of normals
    int32_t nFace        = 0; // number of faces
    int32_t flags        = 0;

    bool isValid() const { return std::memcmp(signature, "SP3X", 4) == 0; }
};

inline SP3XSection readSP3XHeader(BinaryReader& reader)
{
    SP3XSection section;
    uint32_t    sig = reader.read<uint32_t>();
    std::memcpy(section.signature, &sig, 4);
    section.headSize = reader.read<int32_t>();
    section.version  = reader.read<int32_t>();
    section.nPos     = reader.read<int32_t>();
    section.nNorm    = reader.read<int32_t>();
    section.nFace    = reader.read<int32_t>();
    section.flags    = reader.read<int32_t>();

    // Skip remaining header bytes beyond the 28 we read
    constexpr int32_t headerStructSize = 28;
    if (section.headSize > headerStructSize)
        reader.seekRelative(section.headSize - headerStructSize);

    if (std::memcmp(section.signature, "SP3X", 4) != 0)
        throw std::runtime_error("Invalid SP3X signature: expected 'SP3X', got '" + std::string(section.signature, 4) + "'");

    return section;
}

struct PointEx
{
    Vector3 position;
    int32_t flags;
};

struct VertexTable
{
    std::vector<PointEx> points;
};

inline VertexTable readVertexTable(BinaryReader& reader, int32_t nPos)
{
    VertexTable vtable;
    validateCount(reader, nPos, "vertex");
    vtable.points.reserve(nPos);
    for (int32_t i = 0; i < nPos; ++i)
    {
        PointEx pt;
        pt.position.x = reader.read<float>();
        pt.position.y = reader.read<float>();
        pt.position.z = reader.read<float>();
        pt.flags      = reader.read<int32_t>();
        vtable.points.push_back(pt);
    }
    return vtable;
}

struct NormalTable
{
    std::vector<Vector3> normals;
};

inline NormalTable readNormalTable(BinaryReader& reader, int32_t nNorm)
{
    NormalTable ntable;
    validateCount(reader, nNorm, "normal");
    ntable.normals.reserve(nNorm);
    for (int32_t i = 0; i < nNorm; ++i)
    {
        Vector3 norm;
        norm.x = reader.read<float>();
        norm.y = reader.read<float>();
        norm.z = reader.read<float>();
        ntable.normals.push_back(norm);
    }
    return ntable;
}

struct DataVertex
{
    int32_t point;
    int32_t normal;
    float   mapU;
    float   mapV;
};

struct DataFaceEx
{
    char       texture[33] = {}; // 32 wire bytes + guaranteed NUL terminator
    int32_t    n;       // vertex count (3 or 4)
    DataVertex vs[4];   // up to 4 vertices (MAX_DATA_POLY)
    int32_t    flags;
};

struct FaceTable
{
    std::vector<DataFaceEx> faces;
};

inline FaceTable readFaceTable(BinaryReader& reader, int32_t nFace)
{
    FaceTable ftable;
    validateCount(reader, nFace, "face");
    ftable.faces.reserve(nFace);
    for (int32_t i = 0; i < nFace; ++i)
    {
        DataFaceEx face;
        for (int j = 0; j < 32; ++j)
            face.texture[j] = reader.read<char>();
        face.n = reader.read<int32_t>();
        for (int j = 0; j < 4; ++j)
        {
            face.vs[j].point  = reader.read<int32_t>();
            face.vs[j].normal = reader.read<int32_t>();
            face.vs[j].mapU   = reader.read<float>();
            face.vs[j].mapV   = reader.read<float>();
        }
        face.flags = reader.read<int32_t>();
        ftable.faces.push_back(face);
    }
    return ftable;
}

struct TAGGTag
{
    char    name[65] = {}; // 64 wire bytes + guaranteed NUL terminator
    int32_t size = 0;
};

inline TAGGTag readTAGGTag(BinaryReader& reader)
{
    TAGGTag tag;
    for (int i = 0; i < 64; ++i)
        tag.name[i] = reader.read<char>();
    tag.size = reader.read<int32_t>();
    return tag;
}

struct TAGGHeader
{
    char signature[4];
};

inline TAGGHeader readTAGGHeader(BinaryReader& reader)
{
    TAGGHeader header;
    uint32_t   sig = reader.read<uint32_t>();
    std::memcpy(header.signature, &sig, 4);
    if (std::memcmp(header.signature, "TAGG", 4) != 0)
        throw std::runtime_error("Invalid TAGG signature: expected 'TAGG', got '" + std::string(header.signature, 4) + "'");
    return header;
}

struct TAGGNamedSelection
{
    std::string           name;
    std::vector<uint8_t>  pointWeights; // 0=not selected, >0=selected with weight
    std::vector<bool>     faceFlags;
};

inline TAGGNamedSelection readTAGGNamedSelection(BinaryReader& reader, const TAGGTag& tag, int32_t nPoints, int32_t nFaces)
{
    TAGGNamedSelection sel;
    sel.name = tag.name;

    if (tag.size != nPoints + nFaces)
        throw std::runtime_error("Invalid named selection '" + std::string(tag.name) + "': expected " +
                                 std::to_string(nPoints + nFaces) + " bytes, got " + std::to_string(tag.size));

    validateCount(reader, nPoints, "selection point");
    sel.pointWeights.resize(nPoints);
    reader.readBytes(sel.pointWeights.data(), nPoints);

    validateCount(reader, nFaces, "selection face");
    std::vector<uint8_t> faceBytes(nFaces);
    reader.readBytes(faceBytes.data(), nFaces);
    sel.faceFlags.resize(nFaces);
    for (int32_t i = 0; i < nFaces; ++i)
        sel.faceFlags[i] = (faceBytes[i] != 0);

    return sel;
}

struct TAGGNamedProperty
{
    std::string property;
    std::string value;
};

inline TAGGNamedProperty readTAGGProperty(BinaryReader& reader)
{
    TAGGNamedProperty prop;
    // 64-byte NUL-terminated fields. The +1 byte (zero-initialized) guarantees a
    // terminator so the std::string ctor's strlen can't run off the stack when all
    // 64 wire bytes are non-NUL.
    char propName[65] = {};
    for (int i = 0; i < 64; ++i)
        propName[i] = reader.read<char>();
    prop.property = std::string(propName);

    char propValue[65] = {};
    for (int i = 0; i < 64; ++i)
        propValue[i] = reader.read<char>();
    prop.value = std::string(propValue);

    return prop;
}

struct TAGGMass
{
    std::vector<float> massPerPoint;
};

inline TAGGMass readTAGGMass(BinaryReader& reader, int32_t nPoints)
{
    TAGGMass mass;
    validateCount(reader, nPoints, "mass point");
    mass.massPerPoint.reserve(nPoints);
    for (int32_t i = 0; i < nPoints; ++i)
        mass.massPerPoint.push_back(reader.read<float>());
    return mass;
}

struct TAGGAnimationPhase
{
    float               time;
    std::vector<Vector3> positions; // one per point
};

inline TAGGAnimationPhase readTAGGAnimation(BinaryReader& reader, int32_t nPoints, int32_t tagSize)
{
    TAGGAnimationPhase anim;
    int32_t expectedSize = static_cast<int32_t>(sizeof(float) + sizeof(float) * 3 * nPoints);
    if (tagSize != expectedSize)
        throw std::runtime_error("Invalid #Animation# tag size");

    anim.time = reader.read<float>();
    validateCount(reader, nPoints, "animation point");
    anim.positions.reserve(nPoints);
    for (int32_t i = 0; i < nPoints; ++i)
    {
        Vector3 pos;
        pos.x = reader.read<float>();
        pos.y = reader.read<float>();
        pos.z = reader.read<float>();
        anim.positions.push_back(pos);
    }
    return anim;
}

struct TAGGMaterialIndex
{
    int32_t ambient;
    int32_t diffuse;
    int32_t specular;
    int32_t emissive;
};

inline TAGGMaterialIndex readTAGGMaterialIndex(BinaryReader& reader)
{
    TAGGMaterialIndex mat;
    mat.ambient  = reader.read<int32_t>();
    mat.diffuse  = reader.read<int32_t>();
    mat.specular = reader.read<int32_t>();
    mat.emissive = reader.read<int32_t>();
    return mat;
}

struct TAGGData
{
    std::vector<TAGGNamedSelection>  namedSelections;
    std::vector<TAGGNamedProperty>   namedProperties;
    std::vector<TAGGAnimationPhase>  animationPhases;
    TAGGMaterialIndex                materialIndex;
    TAGGMass                         mass;
    float                            resolution       = 0.0f;
    bool                             hasMass          = false;
    bool                             hasAnimations    = false;
    bool                             hasMaterialIndex = false;
};

inline TAGGData readTAGGSection(BinaryReader& reader, int32_t nPoints, int32_t nFaces)
{
    TAGGData data;
    readTAGGHeader(reader);

    for (;;)
    {
        TAGGTag tag = readTAGGTag(reader);

        if (std::strcmp(tag.name, "#EndOfFile#") == 0)
            break;
        else if (tag.name[0] != '#')
        {
            if (tag.name[0] != '-' && tag.name[0] != '.')
                data.namedSelections.push_back(readTAGGNamedSelection(reader, tag, nPoints, nFaces));
            else
                reader.seekRelative(tag.size);
        }
        else if (std::strcmp(tag.name, "#Property#") == 0)
            data.namedProperties.push_back(readTAGGProperty(reader));
        else if (std::strcmp(tag.name, "#Mass#") == 0)
        {
            data.mass    = readTAGGMass(reader, nPoints);
            data.hasMass = true;
        }
        else if (std::strcmp(tag.name, "#Animation#") == 0)
        {
            data.animationPhases.push_back(readTAGGAnimation(reader, nPoints, tag.size));
            data.hasAnimations = true;
        }
        else if (std::strcmp(tag.name, "#MaterialIndex#") == 0)
        {
            data.materialIndex    = readTAGGMaterialIndex(reader);
            data.hasMaterialIndex = true;
        }
        else
            reader.seekRelative(tag.size);
    }

    data.resolution = reader.read<float>();
    return data;
}

} // namespace MLOD
} // namespace Poseidon::Asset::Formats
