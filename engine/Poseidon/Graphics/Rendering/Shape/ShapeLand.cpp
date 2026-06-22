
#include <Poseidon/Core/Application.hpp>
#include <Poseidon/Graphics/Rendering/Shape/Shape.hpp>
// #include <Poseidon/Graphics/Core/Engine.hpp>
#include <Poseidon/World/Terrain/Landscape.hpp>
#include <Poseidon/World/Entities/Vehicles/Plane.hpp>
#include <Poseidon/Foundation/Math/PolyClip.hpp>
#include <utility>
#include <Poseidon/Foundation/Common/FltOpts.hpp>
#include <Poseidon/Foundation/Containers/Array.hpp>
#include <Poseidon/Foundation/Containers/StaticArray.hpp>
#include <Poseidon/Foundation/Containers/StreamArray.hpp>
#include <Poseidon/Foundation/Framework/Log.hpp>
#include <Poseidon/Foundation/Math/Math3D.hpp>
#include <Poseidon/Foundation/Math/Math3DP.hpp>
#include <Poseidon/Foundation/Math/MathDefs.hpp>

namespace Poseidon
{

/*
\patch_internal 1.17 Date 08/14/2001 by Ondra
- Fixed: selection were kept for shadows and roadsplits,
but they got corrupted. This caused Assert in debug build,
othewise it should be harmless.
*/

void Shape::SurfaceSplit(const Landscape* land, const Matrix4& toWorld, float y, float useOrigY)
{
    // selection must be ingored: otherwise they would be invalidated
    _sel.Clear();
    // start: fit all vertices to landscape
    Matrix4 fromWorld(MInverseGeneral, toWorld);

    FaceArray splitFaces(_face.Size() * 3 + 16, false);

    // determine range of landscape squares polygon is in
    Vector3 sNo(1, 0, 0);
    Vector3 uNo(0, 0, 1);
    Vector3 cNo(+H_SQRT2 / 2, 0, +H_SQRT2 / 2);

    Poly zTemp1, zTemp2;
    Poly xTemp1, xTemp2;
    Poly splitA, splitB;
    for (Offset i = BeginFaces(), e = EndFaces(); i < e; NextFace(i))
    {
        Poly source = Face(i);

        if (source.N() <= 0)
        {
            LOG_DEBUG(Graphics, "Invalid source?");
            continue;
        }

        Vector3 pos(VFastTransform, toWorld, Pos(source.GetVertex(0)));
        float xFMin = pos.X(), xFMax = xFMin;
        float zFMin = pos.Z(), zFMax = zFMin;
        for (int vi = 1; vi < source.N(); vi++)
        {
            Vector3 pos(VFastTransform, toWorld, Pos(source.GetVertex(vi)));
            xFMin = floatMin(xFMin, pos.X());
            xFMax = floatMax(xFMax, pos.X());
            zFMin = floatMin(zFMin, pos.Z());
            zFMax = floatMax(zFMax, pos.Z());
        }

        int xMin = toIntFloor(xFMin * InvTerrainGrid);
        int zMin = toIntFloor(zFMin * InvTerrainGrid);
        int xMax = toIntFloor(xFMax * InvTerrainGrid);
        int zMax = toIntFloor(zFMax * InvTerrainGrid);

        Poly* zRest = &source;
        Poly* zSplit = &zTemp1;
        Poly* zFree = &zTemp2;
        for (int z = zMin; z <= zMax; z++)
        {
            float zT = z * TerrainGrid;
            if (z < zMax)
            {
                float zB = (z + 1) * TerrainGrid;
                Vector3 ptB = Vector3(0, 0, zB);
                Plane bottom(-uNo, ptB);
                // cut a part from zRest and use it for x-splitting
                zRest->Split(toWorld, *this, *zSplit, *zFree, bottom.Normal(), bottom.D());
                zSplit->CopyProperties(*zRest);
                zFree->CopyProperties(*zRest);
                swap(zFree, zRest);
            }
            else
            {
                swap(zRest, zSplit); // use whole zRest
            }
            Poly* xRest = zSplit; // zSplit will be destroyed during x splitting
            Poly* xSplit = &xTemp1;
            Poly* xFree = &xTemp2;
            for (int x = xMin; x <= xMax; x++)
            {
                // four clipping planes
                float xR = (x + 1) * TerrainGrid;
                Vector3 ptRT = Vector3(xR, 0, zT);
                if (x < xMax)
                {
                    // left and right side clipping
                    Plane right(-sNo, ptRT);
                    xRest->Split(toWorld, *this, *xSplit, *xFree, right.Normal(), right.D());
                    xSplit->CopyProperties(*xRest);
                    xFree->CopyProperties(*xRest);
                    swap(xFree, xRest);
                }
                else
                {
                    swap(xSplit, xRest);
                }

                // split 'xSplit' to two part (by A/B triangles)
                Plane cutA(-cNo, ptRT);
                xSplit->Split(toWorld, *this, splitA, splitB, cutA.Normal(), cutA.D());

                if (splitA.N() >= 3)
                {
                    splitA.CopyProperties(*xSplit);
                    splitFaces.Add(splitA);
                }

                if (splitB.N() >= 3)
                {
                    splitB.CopyProperties(*xSplit);
                    splitFaces.Add(splitB);
                }
            }
        }
    }

    for (int i = 0; i < NPos(); i++)
    {
        Vector3& pos = SetPos(i);
        Vector3 wPos(VFastTransform, toWorld, pos);
        float oy = y + pos.Y() * useOrigY;
        wPos[1] = land->SurfaceY(wPos[0], wPos[2]) + oy;
        pos.SetFastTransform(fromWorld, wPos);
    }

    _face = splitFaces;              // copy to compact array
    _face.GetData().UnlinkStorage(); // unlink from static storage
    // unmark all splitting hints
    for (int ii = 0; ii < NVertex(); ii++)
    {
        SetClip(ii, Clip(ii) & ~ClipLandMask);
    }
}
} // namespace Poseidon
