#include <Poseidon/Core/Application.hpp>

#include <Poseidon/Graphics/Rendering/Primitives/Edges.hpp>
#include <Poseidon/Graphics/Rendering/Shape/Shape.hpp>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Poseidon/Foundation/Framework/Log.hpp>
#include <Poseidon/Foundation/Math/Math3D.hpp>
#include <Poseidon/Foundation/Memory/FastAlloc.hpp>
#include <Poseidon/Foundation/Types/Pointers.hpp>

namespace Poseidon::Foundation
{
template class Ref<ConvexComponent>;
} // namespace Poseidon::Foundation
namespace Poseidon
{

// assume max. 8 faces per single vertex in geometry
enum
{
    MaxFacesPerVertex = 8
};

} // namespace Poseidon
#include <Poseidon/Foundation/Containers/SmallArray.hpp>
namespace Poseidon
{

class FaceList : public VerySmallArray<VertexIndex, sizeof(MaxFacesPerVertex) * MaxFacesPerVertex + sizeof(int)>
{
};

class FVConnections
{
    AutoArray<FaceList> _point;
    bool _error;

  public:
    void AddFace(Shape* shape, int faceI); // add single face
    void Build(Shape* shape);              // build entire
    // find neighbourg to o over edge a..b
    int FindNeighbourgh(Shape* shape, int faceI, int a, int b) const;
    bool GetError() const { return _error; }
};

void FVConnections::AddFace(Shape* shape, int faceI)
{
    // find vertex
    Offset o = shape->FaceIndexToOffset(faceI);
    const Poly& face = shape->Face(o);
    for (int i = 0; i < face.N(); i++)
    {
        int point = shape->VertexToPoint(face.GetVertex(i));
        if (_point[point].Add(faceI) < 0)
        {
            LOG_DEBUG(Graphics, "Too complex geometry");
            _error = true;
        }
    }
}

int FVConnections::FindNeighbourgh(Shape* shape, int faceI, int a, int b) const
{
    // scan face lists of a and b
    // o should be in both
    // some other face might be in both - this is neighbourgh
    const FaceList& aFaces = _point[a];
    const FaceList& bFaces = _point[b];
    for (int i = 0; i < aFaces.Size(); i++)
    {
        for (int j = 0; j < bFaces.Size(); j++)
        {
            if (aFaces[i] != bFaces[j])
            {
                continue;
            }
            // face aFaces[i] (equal to bFaces[j]) contains both a and b
            if (aFaces[i] == faceI)
            {
                continue;
            }
            return aFaces[i];
        }
    }
    // return invalid offset
    return -1;
}

void FVConnections::Build(Shape* shape)
{
    _error = false;
    // note: conectivity with Objektiv points
    shape->BuildFaceIndexToOffset();

    _point.Realloc(shape->NPoints());
    _point.Resize(shape->NPoints());
    for (int i = 0; i < shape->NFaces(); i++)
    {
        AddFace(shape, i);
    }
}

FaceEdges::FaceEdges()
{
    _error = false;
}

void FaceEdges::Build(Shape* shape, const FVConnections& con)
{
    // scan all faces and set corresponding neighbourghs
    shape->BuildFaceIndexToOffset();
    _data.Reserve(shape->NFaces());
    _offsets.Realloc(shape->NFaces());
    _offsets.Resize(shape->NFaces());
    for (int i = 0; i < shape->NFaces(); i++)
    {
        Offset o = shape->FaceIndexToOffset(i);
        const Poly& face = shape->Face(o);
        int n = face.N();
        int p = n - 1;
        Poly dst;
        for (int v = 0; v < n; v++)
        {
            // check edge p..v
            int vp = face.GetVertex(p); // vertex indices
            int vv = face.GetVertex(v);
            int pp = shape->VertexToPoint(vp); // point indices
            int pv = shape->VertexToPoint(vv);
            // check faces connected with this edge
            int overEdge = con.FindNeighbourgh(shape, i, pp, pv);
            if (overEdge < 0)
            {
                if (!_error)
                {
                    RptF("No neighbourgh");
                }
                _error = true;
            }
            dst.Set(v, overEdge);
            p = v;
        }
        dst.SetN(n);
        Offset dOffset = _data.Add(dst);
        _offsets[i] = dOffset;
    }
}

void FaceEdges::Build(Shape* shape)
{
    FVConnections con;
    con.Build(shape);
    Build(shape, con);
    if (con.GetError())
    {
        _error = true;
    }
}

void ComponentEdges::Build(const FaceEdges& edges, const ConvexComponent& component)
{
    // only edges from component should be converted
    const Selection& faces = component.Faces();
    _offsets.Realloc(faces.Size());
    _offsets.Resize(faces.Size());
    _data.Reserve(faces.Size());
    for (int i = 0; i < faces.Size(); i++)
    {
        // shape face index
        int shapeFI = faces[i];
        const Poly& poly = edges.GetEdges(shapeFI);
        Poly res;
        for (int v = 0; v < poly.N(); v++)
        {
            // shape face index
            int overEdge = poly.GetVertex(v);
            // convert to selection index
            int index = faces.Find(overEdge);
            if (index < 0)
            {
                if (!_error)
                {
                    RptF("Neighbourgh in other component");
                }
                _error = true;
            }
            res.Set(v, index);
        }
        // if face is in component, is should have all neighbourghs there

        res.SetN(poly.N());

        Offset dOffset = _data.Add(res);
        _offsets[i] = dOffset;
    }
    _data.Compact();
}

bool ConvexComponents::RecalculateEdges(Shape* shape)
{
    FaceEdges edges;
    edges.Build(shape);
    bool error = edges.GetError();
    for (int i = 0; i < Size(); i++)
    {
        ComponentEdges* cEdges = new ComponentEdges;
        ConvexComponent& cc = *Set(i);
        cEdges->Build(edges, cc);
        cc.SetEdges(cEdges);
        if (cEdges->GetError())
        {
            RptF("  Component%02d", i + 1);
            error = true;
        }
    }
    return error;
}

DEFINE_FAST_ALLOCATOR(ConvexComponent)

ConvexComponent::ConvexComponent() : _center(VZero), _radius(0) {}

ConvexComponent::~ConvexComponent() = default;

DEFINE_FAST_ALLOCATOR(ConvexComponents)

ConvexComponents::ConvexComponents()
{
    _valid = false;
}

ConvexComponents::~ConvexComponents() = default;

void ConvexComponents::Recalculate(Shape* shape) const
{
    for (int i = 0; i < Size(); i++)
    {
        Get(i)->Recalculate();
    }
    _valid = true;
}

void ConvexComponents::RecalculateAsNeeded(Shape* shape) const
{
    if (!_valid)
    {
        Recalculate(shape);
    }
}
} // namespace Poseidon
