#ifndef WRP_READER_HPP
#define WRP_READER_HPP

// This header must be included after PoseidonPCH.hpp (or equivalent)
// which provides AutoArray, Vector3, Matrix4
#include <Poseidon/Foundation/Strings/RString.hpp>
#include <Poseidon/Foundation/Math/Math3D.hpp>
namespace Poseidon { class QIStream; }

namespace Poseidon
{

struct WrpObjectInfo
{
    int id;
    RStringB name;
    Vector3 position;
    float heading; // only meaningful for RVW v2
    Matrix4 transform;
    bool hasMatrix; // v3/v4/OPRW have full matrix; v2 has position+heading

};

class WrpReader
{
  public:
    enum Format
    {
        Unknown,
        RVW_V2,
        RVW_V3,
        RVW_V4,
        OPRW_V2,
        OPRW_V3
    };

    bool Load(const char* path);

    bool Load(QIStream& f);

    Format GetFormat() const { return _format; }
    const char* GetFormatName() const;

    int GetGridX() const { return _gridX; }
    int GetGridZ() const { return _gridZ; }
    int GetTerrainX() const { return _terrainX; }
    int GetTerrainZ() const { return _terrainZ; }

    // Heightmap access (OPRW stores raw float; RVW stores short scaled by 0.045)
    int GetHeightmapSize() const { return _heightmap.Size(); }
    const float* GetHeightmapData() const { return _heightmap.Data(); }
    float GetHeight(int x, int z) const { return _heightmap[z * _gridX + x]; }
    float GetMinHeight() const { return _minHeight; }
    float GetMaxHeight() const { return _maxHeight; }

    // Raw OPRW arrays (used by Landscape to populate its Array2D members)
    const AutoArray<uint8_t>& GetGeography() const { return _geography; }
    const AutoArray<uint8_t>& GetSoundMap() const { return _soundMap; }
    const AutoArray<Vector3>& GetMountains() const { return _mountains; }
    const AutoArray<uint8_t>& GetTexIndices() const { return _texIndices; }
    const AutoArray<uint8_t>& GetRandom() const { return _random; }

    // Texture table
    int GetTextureCount() const { return _textureNames.Size(); }
    const RStringB& GetTextureName(int i) const { return _textureNames[i]; }

    // Objects
    int GetObjectCount() const { return _objects.Size(); }
    const WrpObjectInfo& GetObject(int i) const { return _objects[i]; }

    // Object class names (OPRW only — indexed name table)
    int GetObjectNameCount() const { return _objectNames.Size(); }
    const RStringB& GetObjectName(int i) const { return _objectNames[i]; }

    const char* GetError() const { return _error; }

  private:
    bool LoadRVW(QIStream& f, int version);
    bool LoadOPRW(QIStream& f);

    Format _format = Unknown;
    int _gridX = 0, _gridZ = 0;
    int _terrainX = 0, _terrainZ = 0;
    AutoArray<float> _heightmap;
    float _minHeight = 0, _maxHeight = 0;
    AutoArray<uint8_t> _geography;  // OPRW: gridX*gridZ*4 bytes (GeographyInfo)
    AutoArray<uint8_t> _soundMap;   // OPRW: gridX*gridZ bytes
    AutoArray<Vector3> _mountains;  // OPRW: mountain peak positions
    AutoArray<uint8_t> _texIndices; // OPRW: gridX*gridZ*2 bytes (texture indices)
    AutoArray<uint8_t> _random;     // OPRW: gridX*gridZ*4 bytes (RandomInfo)
    AutoArray<RStringB> _textureNames;
    AutoArray<WrpObjectInfo> _objects;
    AutoArray<RStringB> _objectNames; // OPRW indexed name table
    const char* _error = nullptr;
};
}  // namespace Poseidon

#endif // WRP_READER_HPP
