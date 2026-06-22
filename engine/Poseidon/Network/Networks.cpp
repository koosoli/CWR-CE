#include <Poseidon/World/Terrain/Landscape.hpp>
#include <Poseidon/Foundation/Common/FltOpts.hpp>
#include <Poseidon/Foundation/Containers/Array.hpp>
#include <Poseidon/Foundation/Containers/Array2D.hpp>
#include <Poseidon/Foundation/Math/Math3D.hpp>
#include <Poseidon/Foundation/Math/Math3DP.hpp>

void Landscape::TerrainChanged(float x, float z, float limit)
{
    FlushCache();
    int xMin, xMax, zMin, zMax;
    xMin = toIntFloor((x - limit) * _invLandGrid);
    xMax = toIntCeil((x + limit) * _invLandGrid);
    zMin = toIntFloor((z - limit) * _invLandGrid);
    zMax = toIntCeil((z + limit) * _invLandGrid);
    if (xMin < 0)
    {
        xMin = 0;
    }
    if (xMax > _landRangeMask)
    {
        xMax = _landRangeMask;
    }
    if (zMin < 0)
    {
        zMin = 0;
    }
    if (zMax > _landRangeMask)
    {
        zMax = _landRangeMask;
    }
    for (int zz = zMin; zz <= zMax; zz++)
    {
        for (int xx = xMin; xx <= xMax; xx++)
        {
            ObjectList& oList = _objects(xx, zz);
            int n = oList.Size();
            for (int i = 0; i < n; i++)
            {
                Object* obj = oList[i];
                if (!obj)
                {
                    continue;
                }
                if (obj->GetType() == Primary || obj->GetType() == Network)
                {
                    Vector3 offCenter = obj->DirectionModelToWorld(obj->GetShape()->BoundingCenter());
                    Point3 pos = obj->Position() - offCenter;
                    pos[1] = SurfaceY(pos[0], pos[2]);
                    obj->SetPosition(pos + offCenter);
                }
            }
        }
    }
}
