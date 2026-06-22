#include <Poseidon/Graphics/Rendering/Primitives/Poly.hpp>
#include <Poseidon/Graphics/Core/TLVertex.hpp>
#include <Poseidon/Graphics/Rendering/Shape/Shape.hpp>
#include <limits.h>
#include <stdio.h>
#include <cmath>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Poseidon/Foundation/Math/Math3D.hpp>
#include <Poseidon/Foundation/Strings/RString.hpp>

namespace Poseidon
{

#ifdef _DEBUG
// make sure no default data are left
// everything must be initialized explicitelly
PolyProperties::PolyProperties()
{
    _texture = (Texture*)-1;
}
#endif

void PolyProperties::Init()
{
    _texture = nullptr;
    _special = 0;
}

Vector3 PolyVertices::GetNormal(const VertexTable& mesh) const
{
    Vector3Val p0 = mesh.Pos(_vertex[0]);
    Vector3Val p1 = mesh.Pos(_vertex[1]);
    Vector3Val p2 = mesh.Pos(_vertex[2]);
    Vector3Val p20 = (p2 - p0);
    return p20.CrossProduct(p1 - p0);
}

int PolyVertices::GetMaterial(const VertexTable& mesh) const
{
    // check material of all vertices
    int min = INT_MAX;
    for (int vi = 0; vi < N(); vi++)
    {
        int v = GetVertex(vi);
        ClipFlags clip = mesh.Clip(v);
        int mat = (clip & ClipUserMask) / ClipUserStep;
        if (min > mat)
        {
            min = mat;
        }
    }
    return min;
}

Vector3 PolyVertices::GetViewNormal(const TLVertexTable& mesh) const
{
    // world space normal
    Vector3Val p0 = mesh.TransPosA(_vertex[0]);
    Vector3Val p1 = mesh.TransPosA(_vertex[1]);
    Vector3Val p2 = mesh.TransPosA(_vertex[2]);
    Vector3 p20 = (p2 - p0);
    return p20.CrossProduct(p1 - p0);
}

bool PolyVertices::BackfaceCull(const TLVertexTable& mesh) const
{
    if (_n < 3)
    {
        return false; // never discard lines
    }
    Vector3Val p0 = mesh.TransPosA(_vertex[0]);
    Vector3Val p1 = mesh.TransPosA(_vertex[1]);
    Vector3Val p2 = mesh.TransPosA(_vertex[2]);

    Vector3Val p20 = p2 - p0;
    Vector3Val p10 = p1 - p0;
    // manually inlined cross product
    float x = p20.Y() * p10.Z() - p20.Z() * p10.Y();
    float y = p20.Z() * p10.X() - p20.X() * p10.Z();
    float z = p20.X() * p10.Y() - p20.Y() * p10.X();

    float np = p0.X() * x + p0.Y() * y + p0.Z() * z;
    return np < 0; // discard this one
}

float Poly::CalculateArea(const VertexTable& mesh)
{
    // calculate texture area and area
    float area = 0;
    int ii = _vertex[0];
    Vector3Val ip = mesh.Pos(ii);
    for (int i = 2; i < _n; i++)
    {
        int ll = _vertex[i - 1];
        int rr = _vertex[i];
        Vector3Val lp = mesh.Pos(ll);
        Vector3Val rp = mesh.Pos(rr);
        Vector3 lip = lp - ip;
        area += lip.CrossProduct(rp - ip).Size();
    }
    return area;
}

float Poly::CalculateUVArea(const VertexTable& mesh)
{
    // calculate texture area and area
    float text = 0;
    int ii = _vertex[0];
    const Texture* texture = GetTexture();
    if (!texture)
    {
        return 0;
    }
    float iu = texture->UToLogical(mesh.U(ii));
    float iv = texture->VToLogical(mesh.V(ii));
    for (int i = 2; i < _n; i++)
    {
        int ll = _vertex[i - 1];
        int rr = _vertex[i];
        float lu = texture->UToLogical(mesh.U(ll));
        float lv = texture->VToLogical(mesh.V(ll));
        float ru = texture->UToLogical(mesh.U(rr));
        float rv = texture->VToLogical(mesh.V(rr));
        text += fabs((lu - iu) * (rv - iv) - (ru - iu) * (lv - iv));
    }
    return text;
}

void PolyVertices::CalculateNormal(Plane& dst, const VertexTable& mesh)
{
    Vector3 normal = GetNormal(mesh).Normalized();
    Vector3Val p0 = mesh.Pos(_vertex[0]);
    dst.SetNormal(normal, -normal * p0);
}

void PolyVertices::CalculateD(Plane& dst, const VertexTable& mesh)
{
    Vector3Val p0 = mesh.Pos(_vertex[0]);
    dst.SetD(-dst.Normal() * p0);
}

void PolyProperties::AnimateTexture(float phase)
{
    if (!_texture)
    {
        return;
    }
    int n = _texture->AnimationLength();
    if (n <= 1)
    {
        return; // no animation
    }
    int i = toIntFloor(phase * n);
    if (i < 0)
    {
        i += n;
    }
    else if (i >= n)
    {
        i -= n;
    }
    PoseidonAssert(i >= 0 && i < n); // avoid clamping animation too much
    SetTexture(_texture->GetAnimation(i));
}

RString PolyProperties::GetDebugText() const
{
    char buf[1024];
    if (_texture)
    {
        snprintf(buf, sizeof(buf), "%s", (const char*)_texture->Name());
    }
    else
    {
        snprintf(buf, sizeof(buf), "%s", (const char*)"<null>");
    }
    if (_special)
    {
        sprintf(buf + strlen(buf), " - %x", _special);
    }
    return buf;
}

float PolyVertices::GetArea(const VertexTable& mesh) const
{
    int i;
    float area = 0;
    for (i = 2; i < _n; i++)
    {
        int ii = _vertex[0];
        int ll = _vertex[i - 1];
        int rr = _vertex[i];
        Vector3Val ip = mesh.Pos(ii);
        Vector3Val lp = mesh.Pos(ll);
        Vector3Val rp = mesh.Pos(rr);
        Vector3Val lmi = (lp - ip);
        Vector3 normal = lmi.CrossProduct(rp - ip);
        area += normal.Size();
    }
    return area * 0.5;
}

float PolyVertices::GetAreaTop(const VertexTable& mesh) const
{
    int i;
    float area = 0;
    for (i = 2; i < _n; i++)
    {
        int ii = _vertex[0];
        int ll = _vertex[i - 1];
        int rr = _vertex[i];
        Vector3Val ip = mesh.Pos(ii);
        Vector3Val lp = mesh.Pos(ll);
        Vector3Val rp = mesh.Pos(rr);
        Vector3Val lmi = (lp - ip);
        Vector3 normal = lmi.CrossProduct(rp - ip);
        area += floatMax(normal.Y(), 0);
    }
    return area * 0.5;
}

inline float UVTest(float a, float b, float c, const UVPair& uv)
{
    return a * uv.u + b * uv.v + c;
}

inline Coord Intersect(const UVPair& in, const UVPair& out, float a, float b, float c)
{
    return (in.u * a + in.v * b + c) / ((in.u - out.u) * a + (in.v - out.v) * b);
}

int Interpolate(Shape& mesh, int in, int out, Coord t, bool selections = false)
{
    const V3& iPos = mesh.Pos(in);
    const V3& oPos = mesh.Pos(out);
    ClipFlags iClip = mesh.Clip(in);
    ClipFlags oClip = mesh.Clip(out);
    const V3& iNorm = mesh.Norm(in);
    const V3& oNorm = mesh.Norm(out);
    // interpolate vertex user data
    ClipFlags hints = iClip & oClip & ClipHints;
    float it = 1 - t;
    // interpolate vertex position
    Vector3 point(iPos * it + oPos * t);
    Vector3 norm(iNorm * it + oNorm * t);
    norm.Normalize();
    // interpolate vertex data
    float inU = mesh.U(in), outU = mesh.U(out);
    float inV = mesh.V(in), outV = mesh.V(out);
    // create new vertex
    float resU = inU * it + outU * t;
    float resV = inV * it + outV * t;
    int nTIndex = mesh.AddVertex(point, norm, hints | ClipAll, resU, resV);
    if (selections)
    {
        // note: it may be neccessary to interpolare weigth in all selections
        // this is very important when splitting u,v coordinates for men or some vehicles
        // scan all selections
        for (int i = 0; i < mesh.NNamedSel(); i++)
        {
            NamedSelection& sel = mesh.NamedSel(i);
            if (sel.Weight(in) <= 0 && sel.Weight(out) <= 0)
            {
                continue;
            }
            // if any of in,out is selected
            // interpolated result must be selected to
            int w = toInt(sel.Weight(in) * it + sel.Weight(out) * t);
            saturate(w, 0, 255);
            // add vertex with interpolated weight
            sel.Add(i, w);
        }
    }
    return nTIndex;
}

void PolyVertices::SplitUV(Shape& mesh, PolyVertices& clip, PolyVertices& rest, float a, float b, float c)
{
    // source level splitting is used to create splitted shapes
    if (_n < 3)
    {
        rest._n = clip._n = 0;
        return;
    }

    // search for first vertex inside clipping half-space
    int i;
    VertexIndex* pVertex; // previous vertex
    VertexIndex* aVertex; // actual vertex

    UVPair pPos, aPos;

    pVertex = _vertex + _n - 1;
    pPos = mesh.UV(*pVertex);

    bool pOut = UVTest(a, b, c, pPos) < 0;

    int nClipped = 0;
    int nRest = 0;

    for (i = 0; i < _n; i++)
    {
        aVertex = _vertex + i;
        aPos = mesh.UV(*aVertex);
        bool aOut = UVTest(a, b, c, aPos) < 0;
        // four possible situations
        if (aOut != pOut)
        {
            // edge going in or out
            Coord t = Intersect(aPos, pPos, a, b, c);
            rest._vertex[nRest++] = clip._vertex[nClipped++] = Interpolate(mesh, *aVertex, *pVertex, t, true);
        }
        if (!aOut)
        {
            clip._vertex[nClipped++] = *aVertex; // point in
        }
        else
        {
            rest._vertex[nRest++] = *aVertex; // point out (in rest)
        }
        pVertex = aVertex;
        pOut = aOut;
        pPos = aPos;
    }

    PoseidonAssert(nClipped <= MaxPoly);
    PoseidonAssert(nRest <= MaxPoly);

    if (nClipped < 3)
    {
        clip._n = 0; // polygon completely out
    }
    else
    {
        clip._n = nClipped;
    }
    if (nRest < 3)
    {
        rest._n = 0; // polygon completely out
    }
    else
    {
        rest._n = nRest;
    }
}
} // namespace Poseidon
