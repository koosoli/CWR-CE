#include <Poseidon/World/Terrain/WrpReader.hpp>
#include <Poseidon/World/Terrain/LandFile.hpp>
#include <Poseidon/IO/Streams/QBStream.hpp>
#include <Poseidon/IO/Streams/SerializeBin.hpp>
#include <Poseidon/IO/Serialization/SerializeBinExt.hpp>
#include <climits>
#include <cfloat>
#include <Poseidon/Foundation/Strings/RString.hpp>
#include <Poseidon/Foundation/platform.hpp>

namespace Poseidon
{

#ifdef _MSC_VER
static const int OPRW_MAGIC = 'WRPO';
#else
static const int OPRW_MAGIC = StrToInt("OPRW");
#endif

const char* WrpReader::GetFormatName() const
{
    switch (_format)
    {
        case RVW_V2:
            return "RVW v2";
        case RVW_V3:
            return "RVW v3";
        case RVW_V4:
            return "RVW v4";
        case OPRW_V2:
            return "OPRW v2";
        case OPRW_V3:
            return "OPRW v3";
        default:
            return "Unknown";
    }
}

bool WrpReader::Load(const char* path)
{
    QIFStream f;
    f.open(path);
    if (f.fail())
    {
        _error = "Failed to open file";
        return false;
    }
    return Load(f);
}

bool WrpReader::Load(QIStream& f)
{
    // Read first 4 bytes to detect format
    int magic = 0;
    f.read(&magic, sizeof(magic));
    if (f.fail())
    {
        _error = "Failed to read header";
        return false;
    }

    if (magic == FILE_MAGIC)
        return LoadRVW(f, 2);
    else if (magic == FILE_MAGIC_3)
        return LoadRVW(f, 3);
    else if (magic == FILE_MAGIC_4)
        return LoadRVW(f, 4);
    else if (magic == OPRW_MAGIC)
        return LoadOPRW(f);

    _error = "Unknown file format";
    return false;
}

bool WrpReader::LoadRVW(QIStream& f, int version)
{
    // Header magic already read; read grid dimensions
    int xRange = 0, zRange = 0;
    f.read(&xRange, sizeof(xRange));
    f.read(&zRange, sizeof(zRange));
    // Cap the grid: xRange/zRange are attacker-controlled and drive xRange*zRange
    // heightmap allocations (the product overflows int into a huge/negative alloc).
    if (f.fail() || xRange <= 0 || zRange <= 0 || xRange > 2048 || zRange > 2048)
    {
        _error = "Bad grid dimensions";
        return false;
    }

    switch (version)
    {
        case 2:
            _format = RVW_V2;
            break;
        case 3:
            _format = RVW_V3;
            break;
        case 4:
            _format = RVW_V4;
            break;
    }
    _gridX = xRange;
    _gridZ = zRange;
    _terrainX = xRange;
    _terrainZ = zRange;

    // Read heightmap (short per cell, convert to float)
    int totalCells = xRange * zRange;
    AutoArray<short> rawHeightmap;
    rawHeightmap.Realloc(totalCells);
    rawHeightmap.Resize(totalCells);

    f.read(rawHeightmap.Data(), totalCells * sizeof(short));
    if (f.fail())
    {
        _error = "Failed to read heightmap";
        return false;
    }

    // Convert to float using LANDDATA_SCALE = 0.045
    static const float LANDDATA_SCALE = 0.045f;
    _heightmap.Realloc(totalCells);
    _heightmap.Resize(totalCells);
    _minHeight = FLT_MAX;
    _maxHeight = -FLT_MAX;
    for (int i = 0; i < totalCells; i++)
    {
        float h = rawHeightmap[i] * LANDDATA_SCALE;
        _heightmap[i] = h;
        if (h < _minHeight)
            _minHeight = h;
        if (h > _maxHeight)
            _maxHeight = h;
    }

    // Skip texture indices (short per cell)
    f.seekg(totalCells * sizeof(short), QIOS::cur);

    // Read texture name table (512 × 32 bytes)
    static const int LAND_TEXTURES_MAX = 512;
    for (int i = 0; i < LAND_TEXTURES_MAX; i++)
    {
        char texName[33] = {}; // 32 wire bytes + guaranteed NUL terminator
        f.read(texName, 32);
        if (*texName)
            _textureNames.Add(RStringB(texName));
    }

    // Read objects
    for (;;)
    {
        if (version < 3)
        {
            SingleObject so;
            f.read(&so, sizeof(so));
            so.name[sizeof(so.name) - 1] = 0; // the raw struct read does not guarantee a terminator
            if (f.fail() || f.eof() || !*so.name)
                break;

            WrpObjectInfo info;
            info.id = _objects.Size();
            info.name = so.name;
            info.position = Vector3(so.x, so.y, so.z);
            info.heading = so.heading;
            info.hasMatrix = false;
            _objects.Add(info);
        }
        else if (version == 3)
        {
            SingleObject3 so;
            f.read(&so, sizeof(so));
            so.name[sizeof(so.name) - 1] = 0; // the raw struct read does not guarantee a terminator
            if (f.fail() || f.eof() || !*so.name)
                break;

            WrpObjectInfo info;
            info.id = _objects.Size();
            info.name = so.name;
            info.transform = ConvertToM(so.matrix);
            info.position = Vector3(so.matrix.Position().X(), so.matrix.Position().Y(), so.matrix.Position().Z());
            info.heading = 0;
            info.hasMatrix = true;
            _objects.Add(info);
        }
        else
        {
            SingleObject4 so;
            f.read(&so, sizeof(so));
            so.name[sizeof(so.name) - 1] = 0; // the raw struct read does not guarantee a terminator
            if (f.fail() || f.eof() || !*so.name)
                break;

            WrpObjectInfo info;
            info.id = so.id;
            info.name = so.name;
            info.transform = ConvertToM(so.matrix);
            info.position = Vector3(so.matrix.Position().X(), so.matrix.Position().Y(), so.matrix.Position().Z());
            info.heading = 0;
            info.hasMatrix = true;
            _objects.Add(info);
        }
    }

    return true;
}

// OPRW format (serialized) — from Landscape::Serialize()

bool WrpReader::LoadOPRW(QIStream& f)
{
    // Magic already consumed by Load(). SerializeBinStream::Version() reads
    // the magic int, so we need to construct the stream from the beginning.
    // Re-wrap as SerializeBinStream, but magic is already read.

    // Read version
    int version = 0;
    f.read(&version, sizeof(version));
    if (f.fail())
    {
        _error = "Failed to read OPRW version";
        return false;
    }
    if (version < 2 || version > 3)
    {
        _error = "Unsupported OPRW version";
        return false;
    }

    if (version >= 3)
    {
        int lx = 0, ly = 0, rx = 0, ry = 0;
        f.read(&lx, sizeof(lx));
        f.read(&ly, sizeof(ly));
        f.read(&rx, sizeof(rx));
        f.read(&ry, sizeof(ry));
        _gridX = lx;
        _gridZ = ly;
        _terrainX = rx;
        _terrainZ = ry;
        // Grid dimensions are attacker-controlled; reject absurd values before they
        // drive the _gridX*_gridZ*4 array allocations below (the product overflows int
        // into a huge/negative allocation). Real terrains are far below this cap.
        if (_gridX < 1 || _gridZ < 1 || _gridX > 2048 || _gridZ > 2048)
        {
            _error = "OPRW grid dimensions out of range";
            return false;
        }
    }
    else
    {
        _gridX = _gridZ = 256;
        _terrainX = _terrainZ = 256;
    }

    _format = (version == 2) ? OPRW_V2 : OPRW_V3;

    // Use SerializeBinStream to handle compressed data
    SerializeBinStream sf(&f);

    // _geography: Array2D<GeographyInfo> (4 bytes per cell = DWORD)
    int geogSize = _gridX * _gridZ * 4;
    _geography.Realloc(geogSize);
    _geography.Resize(geogSize);
    sf.LoadCompressed(_geography.Data(), geogSize);
    if (sf.GetError() != SerializeBinStream::EOK)
    {
        _error = "Failed to decompress geography";
        return false;
    }

    // _soundMap: Array2D<byte>
    int soundSize = _gridX * _gridZ;
    _soundMap.Realloc(soundSize);
    _soundMap.Resize(soundSize);
    sf.LoadCompressed(_soundMap.Data(), soundSize);
    if (sf.GetError() != SerializeBinStream::EOK)
    {
        _error = "Failed to decompress sound map";
        return false;
    }

    // _mountains: AutoArray<Vector3> (transferred via TransferBasicArray)
    sf.TransferBasicArray(_mountains);
    if (sf.GetError() != SerializeBinStream::EOK)
    {
        _error = "Failed to read mountains";
        return false;
    }

    // _tex: Array2D<short>
    int texSize = _gridX * _gridZ * sizeof(short);
    _texIndices.Realloc(texSize);
    _texIndices.Resize(texSize);
    sf.LoadCompressed(_texIndices.Data(), texSize);
    if (sf.GetError() != SerializeBinStream::EOK)
    {
        _error = "Failed to decompress tex data";
        return false;
    }

    // _random: Array2D<RandomInfo> (4 bytes per cell — bitfield packed in unsigned int)
    int randomSize = _gridX * _gridZ * 4;
    _random.Realloc(randomSize);
    _random.Resize(randomSize);
    sf.LoadCompressed(_random.Data(), randomSize);
    if (sf.GetError() != SerializeBinStream::EOK)
    {
        _error = "Failed to decompress random data";
        return false;
    }

    // _data: Array2D<float> — this is the heightmap (RawType = float)
    int dataSize = _gridX * _gridZ * sizeof(float);
    _heightmap.Realloc(_gridX * _gridZ);
    _heightmap.Resize(_gridX * _gridZ);
    sf.LoadCompressed(_heightmap.Data(), dataSize);

    if (sf.GetError() != SerializeBinStream::EOK)
    {
        _error = "Failed to decompress heightmap data";
        return false;
    }

    // Compute min/max height
    _minHeight = FLT_MAX;
    _maxHeight = -FLT_MAX;
    for (int i = 0; i < _heightmap.Size(); i++)
    {
        if (_heightmap[i] < _minHeight)
            _minHeight = _heightmap[i];
        if (_heightmap[i] > _maxHeight)
            _maxHeight = _heightmap[i];
    }

    // Read texture names
    int textureCount = sf.LoadInt();
    // textureCount is attacker-controlled; cap it so a huge value can't spin the
    // string-read loop below (a real terrain has at most a few hundred textures).
    if (textureCount < 0 || textureCount > 65536)
    {
        _error = "OPRW texture count out of range";
        return false;
    }
    for (int i = 0; i < textureCount; i++)
    {
        RStringB name;
        sf.Transfer(name);
        bool enableUV = false;
        sf.Transfer(enableUV);
        if (sf.GetError() != SerializeBinStream::EOK)
        {
            _error = "Truncated texture list";
            return false;
        }
        _textureNames.Add(name);
    }

    // Read object name index (TransferBasicArray)
    sf.TransferBasicArray(_objectNames);

    // Read objects
    for (;;)
    {
        // The object list has no count and ends at id < 0. Also stop at end of input:
        // LoadInt at EOF returns 0 (>= 0) without advancing, so the loop would grow
        // _objects forever. Break once fewer than one int remains (rest sits at 1-3 at
        // EOF, so checking <= 0 is not enough).
        if (sf.GetRest() < static_cast<int>(sizeof(int)))
            break;
        int id = sf.LoadInt();
        if (id < 0)
            break;

        int nameIndex = sf.LoadInt();
        Matrix4 trans;
        sf.Transfer(trans);

        WrpObjectInfo info;
        info.id = id;
        if (nameIndex >= 0 && nameIndex < _objectNames.Size())
            info.name = _objectNames[nameIndex];
        info.transform = trans;
        info.position = Vector3(trans.Position().X(), trans.Position().Y(), trans.Position().Z());
        info.heading = 0;
        info.hasMatrix = true;
        _objects.Add(info);
    }

    return true;
}

} // namespace Poseidon
