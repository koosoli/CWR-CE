#pragma once

// SoAoS: Structure of arrays of structures

#include <Poseidon/Foundation/Containers/StaticArray.hpp>
#include <Poseidon/Foundation/Math/MathDefs.hpp>
#include <Poseidon/Foundation/Math/Math3D.hpp>
namespace Poseidon { class TLVertex; }

namespace Poseidon::Foundation
{
class V3QElement
{ // helper for conversion to/from Quads
  public:
    Coord x;

  private:
    Coord rx1, rx2, rx3;

  public:
    Coord y;

  private:
    Coord ry1, ry2, ry3;

  public:
    Coord z; //,rz1,rz2,rz3;

  public:
    V3QElement() {}
    V3QElement(float xx, float yy, float zz) { x = xx, y = yy, z = zz; }

    __forceinline float X() const { return x; }
    __forceinline float Y() const { return y; }
    __forceinline float Z() const { return z; }
    __forceinline float operator[](int i) const { return (&x)[i * 4]; }
    __forceinline float& operator[](int i) { return (&x)[i * 4]; }

    operator Vector3() const { return Vector3(x, y, z); }
    __forceinline void operator=(const Vector3& val) { x = val.X(), y = val.Y(), z = val.Z(); }
    __forceinline void operator+=(const Vector3& val) { x += val.X(), y += val.Y(), z += val.Z(); }
    __forceinline void operator-=(const Vector3& val) { x -= val.X(), y -= val.Y(), z -= val.Z(); }

    __forceinline float SquareSize() const { return x * x + y * y + z * z; }

    __forceinline void operator=(const V3QElement& val) { x = val.x, y = val.y, z = val.z; }
    __forceinline void operator+=(const V3QElement& val) { x += val.x, y += val.y, z += val.z; }
    __forceinline void operator-=(const V3QElement& val) { x -= val.x, y -= val.y, z -= val.z; }
    __forceinline void SetFastTransform(const Matrix4& a, const Vector3& o)
    {
        (*this) = a * o;
    }
    __forceinline void SetMultiply(const Matrix3& a, const Vector3& o) { (*this) = a * o; }

    __forceinline void SetFastTransform(const Matrix4& a, const V3QElement& o)
    {
        float r0 = a(0, 0) * o[0] + a(0, 1) * o[1] + a(0, 2) * o[2] + a.Position()[0];
        float r1 = a(1, 0) * o[0] + a(1, 1) * o[1] + a(1, 2) * o[2] + a.Position()[1];
        float r2 = a(2, 0) * o[0] + a(2, 1) * o[1] + a(2, 2) * o[2] + a.Position()[2];
        x = r0;
        y = r1;
        z = r2;
    }
    __forceinline void SetMultiply(const Matrix3& a, const V3QElement& o)
    {
        float r0 = a(0, 0) * o[0] + a(0, 1) * o[1] + a(0, 2) * o[2];
        float r1 = a(1, 0) * o[0] + a(1, 1) * o[1] + a(1, 2) * o[2];
        float r2 = a(2, 0) * o[0] + a(2, 1) * o[1] + a(2, 2) * o[2];
        x = r0;
        y = r1;
        z = r2;
    }

    Vector3 operator*(float f) const { return Vector3(x * f, y * f, z * f); }
    Vector3 operator-(const Vector3& op) const { return Vector3(x - op.X(), y - op.Y(), z - op.Z()); }
    Vector3 operator+(const Vector3& op) const { return Vector3(x + op.X(), y + op.Y(), z + op.Z()); }
    Vector3 operator-(const V3QElement& op) const { return Vector3(x - op.x, y - op.y, z - op.z); }
    Vector3 operator+(const V3QElement& op) const { return Vector3(x + op.x, y + op.y, z + op.z); }
    float Distance2(const V3QElement& op) const { return x * op.x + y * op.y + z * op.z; }
    float Distance(const V3QElement& op) const
    {
        float dist2 = x * op.x + y * op.y + z * op.z;
        return dist2 * InvSqrt(dist2);
    }
    float Distance2(Vector3Par op) const { return x * op.X() + y * op.Y() + z * op.Z(); }
    float Distance(Vector3Par op) const
    {
        float dist2 = x * op.X() + y * op.Y() + z * op.Z();
        return dist2 * InvSqrt(dist2);
    }

  private:
    // no initialized construction possible
    V3QElement(const V3QElement& val);
};

extern const V3QElement V3QZero;
extern const V3QElement V3QAside;
extern const V3QElement V3QUp;
extern const V3QElement V3QForward;

class V3Quad
{ // basic element of large vector arrays
    friend class V3Array;

  public:
    // note: only three coordinates calculated and stored
    Coord x[4], y[4], z[4]; // each quad can be hold in single XMM register

    const V3QElement& Get(int i) const { return *reinterpret_cast<const V3QElement*>(&x[i]); }
    V3QElement& Set(int i) { return *reinterpret_cast<V3QElement*>(&x[i]); }
    const Coord* GetXQuad() const { return x; }
    const Coord* GetYQuad() const { return y; }
    const Coord* GetZQuad() const { return z; }
    Coord* SetXQuad() { return x; }
    Coord* SetYQuad() { return y; }
    Coord* SetZQuad() { return z; }

    void operator+=(const V3Quad& op);
    void operator-=(const V3Quad& op);
    void operator*=(float f);

    void Modulate(const V3Quad& op);
    void DotProduct(float res[4], const V3Quad& op);

    void Add(const V3Quad& op1, const V3Quad& op2);
    void Sub(const V3Quad& op1, const V3Quad& op2);
};

struct Quatrix4;
struct Quatrix3;

class V3Array
{ // large vector array data structure
    StaticArray<V3Quad> _data;
    int _size; // size in terms of Vector3

  public:
    V3Array();
    ~V3Array();

    const V3QElement& Get(int i) const { return _data[i >> 2].Get(i & 3); }
    V3QElement& Set(int i) { return _data[i >> 2].Set(i & 3); }

    void Reserve(int sizeNeed, int sizeWant)
    {
        if (_data.MaxSize() * 4 < sizeNeed)
            _data.Realloc((sizeWant + 3) >> 2);
    }
    void Init(int size)
    {
        if (_data.MaxSize() * 4 < size)
            _data.Realloc((size + 3) >> 2);
    }
    void Realloc(int size) { _data.Realloc((size + 3) >> 2); }
    void Resize(int size) { _data.Resize((size + 3) >> 2), _size = size; }
    void SetStorage(MemAllocS* storage);
    void Clear() { _data.Clear(), _size = 0; }
    int Add()
    {
        int index = _size;
        Resize(index + 1);
        return index;
    }
    int Add(const Vector3& val)
    {
        int index = Add();
        Set(index) = val;
        return index;
    }

    int QuadSize() const { return _data.Size(); }
    const V3Quad* QuadData() const { return _data.Data(); }
    V3Quad* QuadData() { return _data.Data(); }

    int Size() const { return _size; }
    void Compact() { _data.Compact(); }

    V3QElement& operator[](int i) { return Set(i); }
    const V3QElement& operator[](int i) const { return Get(i); }

    // no swizzle - both in SoS format
    void Transform(V3Quad* dst, const Matrix4& a, int beg, int end) const;
    void Rotate(V3Quad* dst, const Matrix3& a, int beg, int end) const;

    // swizzle - only source in SoS format
    void Perspective(TLVertex* dst, const Matrix4& a) const;
};

} // namespace Poseidon::Foundation

using ::Poseidon::Foundation::V3Array;
using ::Poseidon::Foundation::V3QAside;
using ::Poseidon::Foundation::V3QElement;
using ::Poseidon::Foundation::V3QForward;
using ::Poseidon::Foundation::V3Quad;
using ::Poseidon::Foundation::V3QUp;
using ::Poseidon::Foundation::V3QZero;
