
#ifdef _KNI

#pragma optimize("t", on)

#include <Poseidon/Foundation/Math/Math3DK.hpp>

#include <Poseidon/Foundation/Framework/DebugLog.hpp>

namespace Poseidon::Foundation
{
const Vector3K VZeroK(0, 0, 0);
const Vector3K VAsideK(1, 0, 0);
const Vector3K VUpK(0, 1, 0);
const Vector3K VForwardK(0, 0, 1);

const Matrix3K M3ZeroK(0, 0, 0, 0, 0, 0, 0, 0, 0);
const Matrix3K M3IdentityK(1, 0, 0, 0, 1, 0, 0, 0, 1);

const Matrix4K M4ZeroK(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
const Matrix4K M4IdentityK(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0);

#if _MSC_VER
bool Matrix4K::IsFinite() const
{
    for (int i = 0; i < 3; i++)
    {
        if (!_finite(Get(i, 0)))
            return false;
        if (!_finite(Get(i, 1)))
            return false;
        if (!_finite(Get(i, 2)))
            return false;
        if (!_finite(GetPos(i)))
            return false;
    }
    return true;
}
bool Matrix3K::IsFinite() const
{
    for (int i = 0; i < 3; i++)
    {
        if (!_finite(Get(i, 0)))
            return false;
        if (!_finite(Get(i, 1)))
            return false;
        if (!_finite(Get(i, 2)))
            return false;
    }
    return true;
}
#endif

float Matrix4K::Characteristic() const
{
    // used in fast comparison
    float sum1 = 0; // lower dependency chains
    float sum2 = 0;
    for (int i = 0; i < 3; i++)
    {
        sum1 += Get(i, 0);
        sum2 += Get(i, 1);
        sum1 += Get(i, 2);
        sum2 += GetPos(i);
    }
    return sum1 + sum2;
}

void Matrix4K::SetTranslation(Vector3KPar offset)
{
    SetIdentity();
    SetPosition(offset);
}

void Matrix4K::SetRotationX(Coord angle)
{
    SetIdentity();
    Coord s = (Coord)sin(angle), c = (Coord)cos(angle);
    Set(1, 1) = +c, Set(1, 2) = -s;
    Set(2, 1) = +s, Set(2, 2) = +c;
}

void Matrix4K::SetRotationY(Coord angle)
{
    SetIdentity();
    Coord s = (Coord)sin(angle), c = (Coord)cos(angle);
    Set(0, 0) = +c, Set(0, 2) = -s;
    Set(2, 0) = +s, Set(2, 2) = +c;
}

void Matrix4K::SetRotationZ(Coord angle)
{
    SetIdentity();
    Coord s = (Coord)sin(angle), c = (Coord)cos(angle);
    Set(0, 0) = +c, Set(0, 1) = -s;
    Set(1, 0) = +s, Set(1, 1) = +c;
}

void Matrix4K::SetScale(Coord x, Coord y, Coord z)
{
    SetZero();
    Set(0, 0) = x;
    Set(1, 1) = y;
    Set(2, 2) = z;
}

void Matrix4K::SetPerspective(Coord cLeft, Coord cTop)
{
    SetZero();
    // xg=x*near/right :: <-1,+1>
    Set(0, 0) = 1.0f / cLeft;
    // yg=y*near/top :: <-1,+1>
    Set(1, 1) = 1.0f / cTop;

    // zg=-w*1
    SetPos(2) = -1;

    // wg=z
    // Set(3,2)=1;

    // this gives actual z result (suppose w=1) = zg/wg =
    // zRes(z)=-1/z;
}

void Matrix4K::Orthogonalize()
{
    Vector3KVal dir = Direction();
    Vector3KVal up = DirectionUp();
    SetDirectionAndUp(dir, up);
}
void Matrix3K::Orthogonalize()
{
    Vector3KVal dir = Direction();
    Vector3KVal up = DirectionUp();
    SetDirectionAndUp(dir, up);
}

void Matrix4K::SetOriented(Vector3KPar dir, Vector3KPar up)
{
    SetIdentity();
    SetDirectionAndUp(dir, up);
}

void Matrix4K::SetView(Vector3KPar point, Vector3KPar dir, Vector3KPar up)
{
    Matrix4K translate(MTranslation, -point);
    Matrix4K direction(MDirection, dir, up);
    SetMultiply(direction, translate);
}

void Matrix4K::SetMultiply(const Matrix4K& a, const Matrix4K& b)
{
#if 1
    // SSE matrix multiplication
    _orientation._aside =
        (a._orientation._aside * b._orientation._aside.X() + a._orientation._up * b._orientation._aside.Y() +
         a._orientation._dir * b._orientation._aside.Z());
    _orientation._up = (a._orientation._aside * b._orientation._up.X() + a._orientation._up * b._orientation._up.Y() +
                        a._orientation._dir * b._orientation._up.Z());
    _orientation._dir = (a._orientation._aside * b._orientation._dir.X() +
                         a._orientation._up * b._orientation._dir.Y() + a._orientation._dir * b._orientation._dir.Z());
    _position = (a._orientation._aside * b._position.X() + a._orientation._up * b._position.Y() +
                 a._orientation._dir * b._position.Z() + a._position);

#else
    // matrix multiplication
    int i, j;
    // b(3,0)=0, b(3,1)=0, b(3,2)=0, b(3,3)=1
    // a(3,0)=0, a(3,1)=0, a(3,2)=0, a(3,3)=1
    for (i = 0; i < 3; i++)
        for (j = 0; j < 3; j++)
        {
            Set(i, j) = (a.Get(i, 0) * b.Get(0, j) + a.Get(i, 1) * b.Get(1, j) + a.Get(i, 2) * b.Get(2, j));
        }
    for (i = 0; i < 3; i++)
    {
        SetPos(i) = (a.Get(i, 0) * b.GetPos(0) + a.Get(i, 1) * b.GetPos(1) + a.Get(i, 2) * b.GetPos(2) + a.GetPos(i));
    }
// for( j=0; j<3; j++ ) Set(3,j)=0;
// Set(3,3)=1;
#endif
}

void Matrix4K::SetMultiply(const Matrix4K& a, const FloatQuad& f)
{
    _orientation.SetMultiply(a._orientation, f);
    _position = a._position * f;
}

void Matrix4K::AddMultiply(const Matrix4K& a, float b)
{
    _orientation._aside += a._orientation._aside * b;
    _orientation._up += a._orientation._up * b;
    _orientation._dir += a._orientation._dir * b;
    _position += a._position * b;
}

void Matrix4K::SetMultiplyByPerspective(const Matrix4K& a, const Matrix4K& b)
{
    int i, j;
    // b is perspective projection
    // b(3,0)=0, b(3,1)=0, b(3,2)=1, b(3,3)=0
    // a(3,0)=0, a(3,1)=0, a(3,2)=0, a(3,3)=1
    for (i = 0; i < 3; i++)
        for (j = 0; j < 3; j++)
        {
            Set(i, j) = (a.Get(i, 0) * b.Get(0, j) + a.Get(i, 1) * b.Get(1, j) + a.Get(i, 2) * b.Get(2, j) +
                         // a.Get(i,3)*b.Get(3,j)
                         a.GetPos(i) * (j == 2));
        }
    for (i = 0; i < 3; i++)
    {
        SetPos(i) = (a.Get(i, 0) * b.GetPos(0) + a.Get(i, 1) * b.GetPos(1) + a.Get(i, 2) * b.GetPos(2));
    }
    // for( j=0; j<3; j++ ) Set(3,j)=b.Get(3,j);
    // Set(3,3)=0;
}

inline float InvSquareSize(float x, float y, float z)
{
    return Inv(x * x + y * y + z * z);
}

Vector3K Vector3K::Normalized() const
{
    Coord size2 = SquareSize();
    if (size2 == 0)
        return *this;
    Coord invSize = InvSqrt(size2);
    return (*this) * invSize;
}

void Vector3K::Normalize() // no return to avoid using instead of Normalized
{
    Coord size2 = SquareSize();
    if (size2 == 0)
        return;
    Coord invSize = InvSqrt(size2);
    (*this) *= invSize;
}

float Vector3K::CosAngle(Vector3KPar op) const
{
    return DotProduct(op) * InvSqrt(op.SquareSize() * SquareSize());
}

float Vector3K::Distance(Vector3KPar op) const
{
    return (*this - op).Size();
}

float Vector3K::Distance2(Vector3KPar op) const
{
    return (op - *this).SquareSize();
}

float Vector3K::DistanceXZ(Vector3KPar op) const
{
    return (op - *this).SizeXZ();
}

float Vector3K::DistanceXZ2(Vector3KPar op) const
{
    Vector3K temp = op - *this;
    return temp.SquareSizeXZ();
}

Vector3K Vector3K::Project(Vector3KPar op) const
{
    return op * DotProduct(op) * op.InvSquareSize();
}

Vector3K Vector3K::CrossProduct(Vector3KPar op) const
{
    float x = Y() * op.Z() - Z() * op.Y();
    float y = Z() * op.X() - X() * op.Z();
    float z = X() * op.Y() - Y() * op.X();
    return Vector3K(x, y, z);
}

#if _MSC_VER
bool Vector3K::IsFinite() const
{
    if (!_finite(Get(0)))
        return false;
    if (!_finite(Get(1)))
        return false;
    if (!_finite(Get(2)))
        return false;
    return true;
}
#endif

void Vector3K::SetFastTransform(const Matrix4K& a, Vector3KPar o)
{
    // matrix stored in major-column format
    __m128 t1, t2, t3;

    t1 = _mm_set_ps1(o.X());
    t2 = _mm_set_ps1(o.Y());

    t1 = _mm_mul_ps(t1, a._orientation._aside._k);
    t2 = _mm_mul_ps(t2, a._orientation._up._k);

    t3 = _mm_set_ps1(o.Z());

    t1 = _mm_add_ps(t1, t2);
    t3 = _mm_mul_ps(t3, a._orientation._dir._k);

    // sum a...
    t3 = _mm_add_ps(t3, a._position._k);
    _k = _mm_add_ps(t1, t3);
}

Vector3K Matrix4K::FastTransform(Vector3KPar o) const
{
    // matrix stored in major-column format
    __m128 t1, t2, t3;

    t1 = _mm_set_ps1(o.X());
    t2 = _mm_set_ps1(o.Y());

    t1 = _mm_mul_ps(t1, _orientation._aside._k);
    t2 = _mm_mul_ps(t2, _orientation._up._k);

    t3 = _mm_set_ps1(o.Z());

    t1 = _mm_add_ps(t1, t2);
    t3 = _mm_mul_ps(t3, _orientation._dir._k);

    // sum a...
    t3 = _mm_add_ps(t3, _position._k);
    return _mm_add_ps(t1, t3);
}

Vector3K Matrix4K::Rotate(Vector3KPar o) const
{
    // matrix stored in major-column format
    __m128 t1, t2, t3;

    t1 = _mm_set_ps1(o.X());
    t2 = _mm_set_ps1(o.Y());

    t1 = _mm_mul_ps(t1, _orientation._aside._k);
    t2 = _mm_mul_ps(t2, _orientation._up._k);

    t3 = _mm_set_ps1(o.Z());

    t1 = _mm_add_ps(t1, t2);
    t3 = _mm_mul_ps(t3, _orientation._dir._k);

    // sum a...
    return _mm_add_ps(t1, t3);
}

void Vector3K::SetMultiplyLeft(Vector3KPar o, const Matrix3K& a)
{ // vector rotation - only 3x3 matrix is used, translation is ignored
    // u=M*v
    Init();
    float o0 = o[0], o1 = o[1], o2 = o[2];
    for (int i = 0; i < 3; i++)
    {
        Set(i) = a.Get(0, i) * o0 + a.Get(1, i) * o1 + a.Get(2, i) * o2;
    }
}

void Matrix4K::SetInvertRotation(const Matrix4K& op)
{
    // matrix inversion is calculated based on these prepositions:
    // matrix is B*A, where A is orthogonal rotation, B is translation
    // inversion of such matrix is Inv(A)*Inv(B)
    // Inv(B) is Transpose(B), Inv(A) is -A
    // if row 3 of op must be (0,0,0,1),
    // size of row(i), i=0...2 must be 1.0
    SetIdentity();
    for (int i = 0; i < 3; i++)
    {
        Set(i, 0) = op.Get(0, i);
        Set(i, 1) = op.Get(1, i);
        Set(i, 2) = op.Get(2, i);
    }
    Vector3K invTranslate(VRotate, *this, -op.Position());
    SetPosition(invTranslate);
}

void Matrix4K::SetInvertScaled(const Matrix4K& op)
{
    // matrix inversion is calculated based on these prepositions:
    // matrix is S*B*A, where S is scale, A is rotation, B is translation
    // inversion of such matrix is Inv(S)*Inv(A)*Inv(B)
    // Inv(B) is Transpose(B), Inv(A) is -A, Inv(S) is C: C(i,i)=1/S(i,i)
    // row 3 of op must be (0,0,0,1),
    // sizes of row(i) are scale coeficients a,b,c
    SetIdentity();
    // calculate scale
    float invScale0 = InvSquareSize(op.Get(0, 0), op.Get(0, 1), op.Get(0, 2));
    float invScale1 = InvSquareSize(op.Get(1, 0), op.Get(1, 1), op.Get(1, 2));
    float invScale2 = InvSquareSize(op.Get(2, 0), op.Get(2, 1), op.Get(2, 2));
    // invert rotation and scale
    for (int i = 0; i < 3; i++)
    {
        Set(i, 0) = op.Get(0, i) * invScale0;
        Set(i, 1) = op.Get(1, i) * invScale1;
        Set(i, 2) = op.Get(2, i) * invScale2;
    }

    // invert translation
    Vector3K invTranslate(VRotate, *this, -op.Position());
    SetPosition(invTranslate);
}

void Matrix4K::SetInvertGeneral(const Matrix4K& op)
{
    // general matrix inverse - no preposition on matrix
    SetIdentity();
    // invert orientation
    SetOrientation(op.Orientation().InverseGeneral());
    // invert translation
    SetPosition(Rotate(-op.Position()));
}

void Matrix3K::SetNormalTransform(const Matrix3K& op)
{
    // normal transformation for scale matrix (a,b,c) is (1/a,1/b,1/c)
    // all matrices we use are rotation combined with scale
    SetIdentity();
    int j;
    float invRow0size2 = InvSquareSize(op.Get(0, 0), op.Get(0, 1), op.Get(0, 2));
    float invRow1size2 = InvSquareSize(op.Get(1, 0), op.Get(1, 1), op.Get(1, 2));
    float invRow2size2 = InvSquareSize(op.Get(2, 0), op.Get(2, 1), op.Get(2, 2));
    for (j = 0; j < 3; j++)
    {
        Set(0, j) = op.Get(0, j) * invRow0size2;
        Set(1, j) = op.Get(1, j) * invRow1size2;
        Set(2, j) = op.Get(2, j) * invRow2size2;
    }
}

void Matrix3K::SetIdentity()
{
    *this = M3IdentityK;
}

void Matrix3K::SetZero()
{
    *this = M3ZeroK;
}

void Matrix3K::SetRotationX(Coord angle)
{
    Coord s = sin(angle), c = cos(angle);
    SetIdentity();
    Set(1, 1) = +c, Set(1, 2) = -s;
    Set(2, 1) = +s, Set(2, 2) = +c;
}

void Matrix3K::SetRotationY(Coord angle)
{
    Coord s = sin(angle), c = cos(angle);
    SetIdentity();
    Set(0, 0) = +c, Set(0, 2) = -s;
    Set(2, 0) = +s, Set(2, 2) = +c;
}

void Matrix3K::SetRotationZ(Coord angle)
{
    Coord s = (Coord)sin(angle), c = (Coord)cos(angle);
    SetIdentity();
    Set(0, 0) = +c, Set(0, 1) = -s;
    Set(1, 0) = +s, Set(1, 1) = +c;
}

void Matrix3K::SetScale(Coord x, Coord y, Coord z)
{
    SetZero();
    Set(0, 0) = x;
    Set(1, 1) = y;
    Set(2, 2) = z;
}

void Matrix3K::SetScale(float scale)
{
    // note: old scale may be zero
    float invOldScale = InvScale();
    if (invOldScale > 0)
    {
        (*this) *= scale * invOldScale;
    }
    else
    {
        SetScale(scale, scale, scale);
    }
    // any SetDirection will remove scale
}

float Matrix3K::Scale() const
{
    // Frame transformation is composed from
    // translation*rotation*scale
    // current scale can be determined as
    // orient * transpose orient
    Matrix3K oTo = (*this) * InverseRotation();
    // note all but [i][i] should be zero
    float sx2 = oTo(0, 0);
    float sy2 = oTo(1, 1);
    float sz2 = oTo(2, 2);
    // calculate average scale
    float scale2 = (sx2 + sy2 + sz2) * (1.0 / 3);
    if (fabs(scale2 - 1) < 1e-3)
        return 1 + (scale2 - 1) * 0.5;
    return sqrt(scale2);
}

float Matrix3K::InvScale() const
{
    // Frame transformation is composed from
    // translation*rotation*scale
    // current scale can be determined as
    // orient * transpose orient
    Matrix3K oTo = (*this) * InverseRotation();
    // note all but [i][i] should be zero
    float sx2 = oTo(0, 0);
    float sy2 = oTo(1, 1);
    float sz2 = oTo(2, 2);
    // calculate average scale
    float avgS2 = (sx2 + sy2 + sz2) * (1.0 / 3);
    if (avgS2 <= 0)
        return 0; // singular matrix
    return InvSqrt(avgS2);
}

void Matrix3K::SetDirectionAndUp(Vector3KPar dir, Vector3KPar up)
{
    SetDirection(dir.Normalized());
    // Project into the plane
    Coord t = up * Direction();
    Vector3K temp = up - Direction() * t;
    SetDirectionUp(temp.Normalized());
    // Calculate the vector pointing along the x axis (i.e. aside)
    // sign chosen empirically
    // no need to normalize - cross product of two perpendicular unit vectors is unit vector
    SetDirectionAside(DirectionUp().CrossProduct(Direction()));
}

void Matrix3K::SetDirectionAndAside(Vector3KPar dir, Vector3KPar aside)
{
    SetDirection(dir.Normalized());
    // Project into the plane
    Coord t = aside * Direction();
    Vector3K temp = aside - Direction() * t;
    SetDirectionAside(temp.Normalized());
    // Calculate the vector pointing along the x axis (i.e. aside)
    // sign chosen empirically
    // no need to normalize - cross product of two perpendicular unit vectors is unit vector
    SetDirectionUp(Direction().CrossProduct(DirectionAside()));
}

void Matrix3K::SetUpAndAside(Vector3KPar up, Vector3KPar aside)
{
    SetDirectionUp(up.Normalized());
    // Project into the plane
    Coord t = DirectionUp() * aside;
    Vector3K temp = aside - DirectionUp() * t;
    SetDirectionAside(temp.Normalized());
    // Calculate the vector pointing along the x axis (i.e. aside)
    // sign chosen empirically
    // no need to normalize - cross product of two perpendicular unit vectors is unit vector
    SetDirection(DirectionAside().CrossProduct(DirectionUp()));
}

void Matrix3K::SetUpAndDirection(Vector3KPar up, Vector3KPar dir)
{
    SetDirectionUp(up.Normalized());
    // Project into the plane
    Coord t = DirectionUp() * dir;
    // Calculate the vector pointing along the x axis (i.e. aside)
    // sign chosen empirically
    // no need to normalize - cross product of two perpendicular unit vectors is unit vector
    Vector3K temp = dir - DirectionUp() * t;
    SetDirection(temp.Normalized());
    SetDirectionAside(DirectionUp().CrossProduct(Direction()));
}

void Matrix3K::SetTilda(Vector3KPar a)
{
    SetZero();
    Set(0, 1) = -a[2], Set(0, 2) = +a[1];
    Set(1, 0) = +a[2], Set(1, 2) = -a[0];
    Set(2, 0) = -a[1], Set(2, 1) = +a[0];
}

void Matrix3K::operator*=(float op)
{
    _aside *= op;
    _up *= op;
    _dir *= op;
}

void Matrix3K::SetMultiply(const Matrix3K& a, const Matrix3K& b)
{
    _aside = (a._aside * b._aside.X() + a._up * b._aside.Y() + a._dir * b._aside.Z());
    _up = (a._aside * b._up.X() + a._up * b._up.Y() + a._dir * b._up.Z());
    _dir = (a._aside * b._dir.X() + a._up * b._dir.Y() + a._dir * b._dir.Z());
}

void Matrix3K::SetMultiply(const Matrix3K& a, const FloatQuad& f)
{
    _aside = a._aside * f;
    _up = a._up * f;
    _dir = a._dir * f;
}

Matrix3K& Matrix3K::operator+=(const Matrix3K& a)
{
    _aside += a._aside;
    _up += a._up;
    _dir += a._dir;
    return *this;
}

void Matrix4K::operator+=(const Matrix4K& a)
{
    _orientation._aside += a._orientation._aside;
    _orientation._up += a._orientation._up;
    _orientation._dir += a._orientation._dir;
    _position += a._position;
}

void Matrix4K::operator-=(const Matrix4K& a)
{
    _orientation._aside -= a._orientation._aside;
    _orientation._up -= a._orientation._up;
    _orientation._dir -= a._orientation._dir;
    _position -= a._position;
}

Matrix3K Matrix3K::operator+(const Matrix3K& a) const
{
    Matrix3K res;
    res._aside = _aside + a._aside;
    res._up = _up + a._up;
    res._dir = _dir + a._dir;
    return res;
}

Matrix4K Matrix4K::operator+(const Matrix4K& a) const
{
    Matrix4K res;
    res._orientation._aside = _orientation._aside + a._orientation._aside;
    res._orientation._up = _orientation._up + a._orientation._up;
    res._orientation._dir = _orientation._dir + a._orientation._dir;
    res._position = _position + a._position;
    return res;
}

Matrix4K Matrix4K::operator-(const Matrix4K& a) const
{
    Matrix4K res;
    res._orientation._aside = _orientation._aside - a._orientation._aside;
    res._orientation._up = _orientation._up - a._orientation._up;
    res._orientation._dir = _orientation._dir - a._orientation._dir;
    res._position = _position - a._position;
    return res;
}

Matrix3K Matrix3K::operator-(const Matrix3K& a) const
{
    Matrix3K res;
    res._aside = _aside - a._aside;
    res._up = _up - a._up;
    res._dir = _dir - a._dir;
    return res;
}

Matrix3K& Matrix3K::operator-=(const Matrix3K& a)
{
    _aside -= a._aside;
    _up -= a._up;
    _dir -= a._dir;
    return *this;
}

void Matrix3K::SetInvertRotation(const Matrix3K& op)
{
    SetIdentity();
    for (int i = 0; i < 3; i++)
    {
        Set(i, 0) = op.Get(0, i);
        Set(i, 1) = op.Get(1, i);
        Set(i, 2) = op.Get(2, i);
    }
}

void Matrix3K::SetInvertScaled(const Matrix3K& op)
{
    // matrix inversion is calculated based on these prepositions:
    // matrix is S*A, where S is scale, A is rotation
    // inversion of such matrix is Inv(S)*Inv(A)
    // Inv(A) is Transpose(A), Inv(S) is C: C(i,i)=1/S(i,i)
    // sizes of row(i) are scale coeficients a,b,c
    SetIdentity();
    // calculate scale
    float invScale0 = InvSquareSize(op.Get(0, 0), op.Get(0, 1), op.Get(0, 2));
    float invScale1 = InvSquareSize(op.Get(1, 0), op.Get(1, 1), op.Get(1, 2));
    float invScale2 = InvSquareSize(op.Get(2, 0), op.Get(2, 1), op.Get(2, 2));
    // invert rotation and scale
    for (int i = 0; i < 3; i++)
    {
        Set(i, 0) = op.Get(0, i) * invScale0;
        Set(i, 1) = op.Get(1, i) * invScale1;
        Set(i, 2) = op.Get(2, i) * invScale2;
    }
}

#define swap(a, b) \
    {              \
        float p;   \
        p = a;     \
        a = b;     \
        b = p;     \
    }

void Matrix3K::SetInvertGeneral(const Matrix3K& op)
{
    // calculate inversion using Gauss-Jordan elimination
    Matrix3K a = op;
    // load result with identity
    SetIdentity();
    int row, col;
    // construct result by pivoting
    // pivot column
    for (col = 0; col < 3; col++)
    {
        // use maximal number as pivot
        float max = 0;
        int maxRow = col;
        for (row = col; row < 3; row++)
        {
            float mag = fabs(a.Get(row, col));
            if (max < mag)
                max = mag, maxRow = row;
        }
        if (max <= 0.0)
            continue; // no pivot exists
        // swap lines col and maxRow
        swap(a.Set(col, 0), a.Set(maxRow, 0));
        swap(a.Set(col, 1), a.Set(maxRow, 1));
        swap(a.Set(col, 2), a.Set(maxRow, 2));
        swap(Set(col, 0), Set(maxRow, 0));
        swap(Set(col, 1), Set(maxRow, 1));
        swap(Set(col, 2), Set(maxRow, 2));
        // use a(col,col) as pivot
        float quotient = 1 / a.Get(col, col);
        // make pivot 1
        a.Set(col, 0) *= quotient, a.Set(col, 1) *= quotient, a.Set(col, 2) *= quotient;
        Set(col, 0) *= quotient, Set(col, 1) *= quotient, Set(col, 2) *= quotient;
        // use pivot line to zero all other lines
        for (row = 0; row < 3; row++)
            if (row != col)
            {
                float factor = a.Get(row, col);
                a.Set(row, 0) -= a.Get(col, 0) * factor;
                a.Set(row, 1) -= a.Get(col, 1) * factor;
                a.Set(row, 2) -= a.Get(col, 2) * factor;
                Set(row, 0) -= Get(col, 0) * factor;
                Set(row, 1) -= Get(col, 1) * factor;
                Set(row, 2) -= Get(col, 2) * factor;
            }
    }
}

void SaturateMin(Vector3K& min, Vector3KPar val)
{
    min = _mm_min_ps(min.GetM128(), val.GetM128());
}
void SaturateMax(Vector3K& max, Vector3KPar val)
{
    max = _mm_max_ps(max.GetM128(), val.GetM128());
}

void CheckMinMax(Vector3K& min, Vector3K& max, Vector3KPar val)
{
    min = _mm_min_ps(min.GetM128(), val.GetM128());
    max = _mm_max_ps(max.GetM128(), val.GetM128());
}

Vector3K VectorMin(Vector3KPar a, Vector3KPar b)
{
    return _mm_min_ps(a.GetM128(), b.GetM128());
}

Vector3K VectorMax(Vector3KPar a, Vector3KPar b)
{
    return _mm_max_ps(a.GetM128(), b.GetM128());
}

Matrix4P ConvertKToP(const Matrix4K& m)
{
    Matrix4P ret;
    Vector3P pos(m.Position()[0], m.Position()[1], m.Position()[2]);
    Vector3P aside(m.DirectionAside()[0], m.DirectionAside()[1], m.DirectionAside()[2]);
    Vector3P up(m.DirectionUp()[0], m.DirectionUp()[1], m.DirectionUp()[2]);
    Vector3P dir(m.Direction()[0], m.Direction()[1], m.Direction()[2]);
    ret.SetPosition(pos);
    ret.SetDirectionAside(aside);
    ret.SetDirectionUp(up);
    ret.SetDirection(dir);
    return ret;
}
Matrix4K ConvertPToK(const Matrix4P& m)
{
    Matrix4K ret;
    Vector3K pos(m.Position()[0], m.Position()[1], m.Position()[2]);
    Vector3K aside(m.DirectionAside()[0], m.DirectionAside()[1], m.DirectionAside()[2]);
    Vector3K up(m.DirectionUp()[0], m.DirectionUp()[1], m.DirectionUp()[2]);
    Vector3K dir(m.Direction()[0], m.Direction()[1], m.Direction()[2]);
    ret.SetPosition(pos);
    ret.SetDirectionAside(aside);
    ret.SetDirectionUp(up);
    ret.SetDirection(dir);
    return ret;
}

Matrix3P ConvertKToP(const Matrix3K& m)
{
    Matrix3P ret;
    Vector3P aside(m.DirectionAside()[0], m.DirectionAside()[1], m.DirectionAside()[2]);
    Vector3P up(m.DirectionUp()[0], m.DirectionUp()[1], m.DirectionUp()[2]);
    Vector3P dir(m.Direction()[0], m.Direction()[1], m.Direction()[2]);
    ret.SetDirectionAside(aside);
    ret.SetDirectionUp(up);
    ret.SetDirection(dir);
    return ret;
}
Matrix3K ConvertPToK(const Matrix3P& m)
{
    Matrix3K ret;
    Vector3K aside(m.DirectionAside()[0], m.DirectionAside()[1], m.DirectionAside()[2]);
    Vector3K up(m.DirectionUp()[0], m.DirectionUp()[1], m.DirectionUp()[2]);
    Vector3K dir(m.Direction()[0], m.Direction()[1], m.Direction()[2]);
    ret.SetDirectionAside(aside);
    ret.SetDirectionUp(up);
    ret.SetDirection(dir);
    return ret;
}

} // namespace Poseidon::Foundation

#endif
