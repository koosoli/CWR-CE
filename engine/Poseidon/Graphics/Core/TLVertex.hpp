#ifdef _MSC_VER
#pragma once
#endif

#ifndef _TLVERTEX_HPP
#define _TLVERTEX_HPP

#include <Poseidon/Graphics/Rendering/Primitives/Vertex.hpp>
#include <Poseidon/Graphics/Rendering/Colors.hpp>
#include <Poseidon/Foundation/Math/Math3DP.hpp>
#include <Poseidon/Foundation/Containers/StaticArray.hpp>

namespace Poseidon
{
typedef PackedColor TLColor;

class TLVertex
{
  public:
    Vector3P pos; // D3DFVF_XYZRHW
    float rhw;
    PackedColor color;    // D3DFVF_DIFFUSE
    PackedColor specular; // D3DFVF_SPECULAR
    UVPair t0;            // D3DFVF_TEXCOORDSIZE2(0)
    UVPair t1;            // D3DFVF_TEXCOORDSIZE2(1)
#if _PIII
    UVPair t2; // force alignment to 16B multiply
#endif
};

//! material for T&L module

struct TLMaterial
{
    Color emmisive;
    Color ambient;
    Color diffuse;
    Color specular; // note: specular not supported
    // used from special lighting - half lighted etc.
    Color forcedDiffuse; // difuse to ambient transfer
    int specFlags;       // fog mode and some more special flags
    int specularPower;

    TLMaterial();
    bool IsEqual(const TLMaterial& src) const;
    bool operator==(const TLMaterial& src) const { return IsEqual(src); }
    bool operator!=(const TLMaterial& src) const { return !IsEqual(src); }
};

void CreateMaterialNormal(TLMaterial& mat, ColorVal col);
void CreateMaterialShining(TLMaterial& mat, ColorVal col);
void CreateMaterialConstant(TLMaterial& mat, ColorVal col, float factor);

void CreateMaterial(TLMaterial& mat, ColorVal col, int type);

class TLVertexTable
{
    // this class stores transformed and lit vetices
    // is uses view cordinate system

  protected:
    ClipFlags _orHints, _andHints; // we can do some optimizations based on this

  private:
    enum
    {
        BufSize = 4096
    };

#if USE_QUADS
    V3Array _posTransQ;
    bool _useQuads;
#endif
    StaticArray<Vector3> _posTrans;
    StaticArray<TLVertex> _vert;
    StaticArray<ClipFlags> _clip;

  public:
    void ReleaseTables();

  private:
    // disable default copy
    TLVertexTable(const TLVertexTable& src);
    void operator=(const TLVertexTable& src);

  public:
    TLVertexTable();
    void DoConstruct();

    ~TLVertexTable() { ReleaseTables(); }

    int NVertex() const { return _vert.Size(); }

    int AddPos();

#if USE_QUADS
    const V3QElement& TransPosQ(int i) const { return _posTransQ[i]; }
    V3QElement& SetTransPosQ(int i) { return _posTransQ[i]; }

    Vector3 TransPosA(int i) const
    {
        if (_useQuads)
        {
            const V3QElement& elem = _posTransQ[i];
            return Vector3(elem.X(), elem.Y(), elem.Z());
        }
        else
            return _posTrans[i];
    }
    void SetTransPosA(int i, const Vector3& val)
    {
        if (_posTransQ.Size() > 0)
            _posTransQ[i] = val;
        else
            _posTrans[i] = val;
    }
#else
    Vector3Val TransPosA(int i) const { return _posTrans[i]; }
    void SetTransPosA(int i, const Vector3& val) { _posTrans[i] = val; }
#endif

    Vector3Val TransPos(int i) const { return _posTrans[i]; }
    Vector3& SetTransPos(int i) { return _posTrans[i]; }

    Vector3PVal ScreenPos(int i) const { return _vert[i].pos; }
    Vector3P& SetScreenPos(int i) { return _vert[i].pos; }

    ClipFlags Clip(int i) const { return _clip[i]; }
    void SetClip(int i, ClipFlags clipped) { _clip[i] = clipped; }

    const TLVertex& GetVertex(int i) const { return _vert[i]; }
    TLVertex& SetVertex(int i) { return _vert[i]; }

    const TLVertex* VertexData() const { return _vert.Data(); }
    TLVertex* VertexData() { return _vert.Data(); }

    float U(int i) const { return _vert[i].t0.u; }
    float V(int i) const { return _vert[i].t0.v; }
    void SetU(int i, float u) { _vert[i].t0.u = u; }
    void SetV(int i, float v) { _vert[i].t0.v = v; }

    const UVPair& UV(int i) const { return _vert[i].t0; }
    void SetUV(int i, float u, float v) { _vert[i].t0.u = u, _vert[i].t0.v = v; }

    const TLColor& GetColor(int i) const { return _vert[i].color; }
    TLColor& SetColor(int i) { return _vert[i].color; }

    const TLColor& GetSpecular(int i) const { return _vert[i].specular; }
    TLColor& SetSpecular(int i) { return _vert[i].specular; }

    void DoTransformPoints(const IAnimator* anim, const Shape& src, const Matrix4& posView);
    void DoTransformPoints(const VertexTable& src, const Matrix4& posView, int beg, int end);

    // transformation constructor
    TLVertexTable(const IAnimator* anim, const Shape& src, const Matrix4& toView);
    void Init(const IAnimator* anim, const Shape& src, const Matrix4& toView);
    ClipFlags CheckClipping(const Camera& camera, ClipFlags mask, ClipFlags& andClip);
    void DoPerspective(const Camera& camera, ClipFlags clip);

    ClipFlags GetOrHints() const { return _orHints; }
    ClipFlags GetAndHints() const { return _andHints; }

#if USE_QUADS
    bool UsingQuads() const { return _useQuads; }
#endif

    // custom lighting
    void DoLighting(const IAnimator* matSource, const Matrix4& worldToModel, const LightList& lights, const Shape& mesh,
                    int spec);
    void DoLightingColorized(const LightList& lights,
                             const PackedColor* colors, // color array
                             const VertexTable& mesh, int spec);

    void DoMaterialLightingP(const TLMaterial& mat, const Matrix4& worldToModel, const LightList& lights,
                             const VertexTable& mesh, int beg, int end);
    void DoMaterialLightingQ(const TLMaterial& mat, const Matrix4& worldToModel, const LightList& lights,
                             const VertexTable& mesh, int beg, int end);
    void DoShadowLighting(const VertexTable& mesh, int spec);
    void DoLightLighting(const VertexTable& mesh, int spec);
    void DoCloudLighting(const VertexTable& mesh, int spec);
    void DoStarLighting(const Matrix4& worldToModel, const VertexTable& mesh, int spec);
    void DoLineLighting(const VertexTable& mesh, int spec);
    void DoSkyLighting(const VertexTable& mesh, int spec);
};

} // namespace Poseidon
#include <Poseidon/Graphics/Rendering/Primitives/ClipVert.hpp>

#endif
