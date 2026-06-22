
#ifdef _T_MATH

#pragma optimize("t", on)

#include <Poseidon/Foundation/Math/Math3DT.hpp>

#include <Poseidon/Foundation/Framework/DebugLog.hpp>

namespace Poseidon::Foundation
{
const Matrix3T M3ZeroT(0, 0, 0, 0, 0, 0, 0, 0, 0);
const Matrix3T M3IdentityT(1, 0, 0, 0, 1, 0, 0, 0, 1);

const Matrix4T M4ZeroT(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
const Matrix4T M4IdentityT(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0);

const Vector3T VZeroT(0, 0, 0);
const Vector3T VUpT(0, 1, 0);
const Vector3T VForwardT(0, 0, 1);
const Vector3T VAsideT(1, 0, 0);

#if _MSC_VER
bool Matrix4T::IsFinite() const
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
bool Matrix3T::IsFinite() const
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

float Matrix4T::Characteristic() const
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

void Matrix4T::SetIdentity()
{
    *this = M4IdentityT;
}

void Matrix4T::SetZero()
{
    *this = M4ZeroT;
}

void Matrix4T::SetTranslation(Vector3TPar offset)
{
    SetIdentity();
    SetPosition(offset);
}

void Matrix4T::SetRotationX(Coord angle)
{
    SetIdentity();
    Coord s = (Coord)sin(angle), c = (Coord)cos(angle);
    Set(1, 1) = +c, Set(1, 2) = -s;
    Set(2, 1) = +s, Set(2, 2) = +c;
}

void Matrix4T::SetRotationY(Coord angle)
{
    SetIdentity();
    Coord s = (Coord)sin(angle), c = (Coord)cos(angle);
    Set(0, 0) = +c, Set(0, 2) = -s;
    Set(2, 0) = +s, Set(2, 2) = +c;
}

void Matrix4T::SetRotationZ(Coord angle)
{
    SetIdentity();
    Coord s = (Coord)sin(angle), c = (Coord)cos(angle);
    Set(0, 0) = +c, Set(0, 1) = -s;
    Set(1, 0) = +s, Set(1, 1) = +c;
}

void Matrix4T::SetScale(Coord x, Coord y, Coord z)
{
    SetZero();
    Set(0, 0) = x;
    Set(1, 1) = y;
    Set(2, 2) = z;
}

void Matrix4T::SetPerspective(Coord cLeft, Coord cTop)
{
    SetZero();
    // xg=x*near/right :: <-1,+1>
    Set(0, 0) = 1.0f / cLeft;
    // yg=y*near/top :: <-1,+1>
    Set(1, 1) = 1.0f / cTop;

    Set(2, 2) = 1.0; // DirectX has Q here, Q = zFar/(zFar-zNear)

    // zg=-w*1
    SetPos(2) = -1; // DirectX has -Q/zNear here

    // wg=z
    // Set(3,2)=1;

    // this gives actual z result (suppose w=1) = zg/wg =
    // zRes(z)=-1/z;
}

void Matrix4T::Orthogonalize()
{
    Vector3TVal dir = Direction();
    Vector3TVal up = DirectionUp();
    SetDirectionAndUp(dir, up);
}
void Matrix3T::Orthogonalize()
{
    Vector3TVal dir = Direction();
    Vector3TVal up = DirectionUp();
    SetDirectionAndUp(dir, up);
}

void Matrix4T::SetOriented(Vector3TPar dir, Vector3TPar up)
{
    SetIdentity();
    SetDirectionAndUp(dir, up);
}

void Matrix4T::SetView(Vector3TPar point, Vector3TPar dir, Vector3TPar up)
{
    Matrix4T translate(MTranslation, -point);
    Matrix4T direction(MDirection, dir, up);
    SetMultiply(direction, translate);
}

void Matrix4T::SetAdd(const Matrix4T& a, const Matrix4T& b)
{
    _orientation._aside = a._orientation._aside + b._orientation._aside;
    _orientation._up = a._orientation._up + b._orientation._up;
    _orientation._dir = a._orientation._dir + b._orientation._dir;
    _position = a._position + b._position;
}

void Matrix4T::SetMultiply(const Matrix4T& a, const Matrix4T& b)
{
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
}

void Matrix4T::SetMultiply(const Matrix4T& a, float b)
{
    _orientation._aside = a._orientation._aside * b;
    _orientation._up = a._orientation._up * b;
    _orientation._dir = a._orientation._dir * b;
    _position = a._position * b;
}

void Matrix4T::AddMultiply(const Matrix4T& a, float b)
{
    _orientation._aside += a._orientation._aside * b;
    _orientation._up += a._orientation._up * b;
    _orientation._dir += a._orientation._dir * b;
    _position += a._position * b;
}

void Matrix4T::SetMultiplyByPerspective(const Matrix4T& a, const Matrix4T& b)
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
}

inline float InvSquareSize(float x, float y, float z)
{
    return Inv(x * x + y * y + z * z);
}

Vector3T Vector3T::Normalized() const
{
    Coord size2 = SquareSizeInline();
    if (size2 == 0)
        return *this;
    Coord invSize = InvSqrt(size2);
    return Vector3T(X() * invSize, Y() * invSize, Z() * invSize);
}

void Vector3T::Normalize() // no return to avoid using instead of Normalized
{
    Coord size2 = SquareSizeInline();
    if (size2 == 0)
        return;
    Coord invSize = InvSqrt(size2);
    Set(0) *= invSize, Set(1) *= invSize, Set(2) *= invSize;
}

Vector3T Vector3T::CrossProduct(Vector3TPar op) const
{
    float x = Y() * op.Z() - Z() * op.Y();
    float y = Z() * op.X() - X() * op.Z();
    float z = X() * op.Y() - Y() * op.X();
    return Vector3T(x, y, z);
}

float Vector3T::CosAngle(Vector3TPar op) const
{
    return DotProduct(op) * InvSqrt(op.SquareSizeInline() * SquareSizeInline());
}

float Vector3T::Distance(Vector3TPar op) const
{
    return (*this - op).Size();
}

float Vector3T::DistanceXZ(Vector3TPar op) const
{
    return (*this - op).SizeXZ();
}
float Vector3T::DistanceXZ2(Vector3TPar op) const
{
    return (*this - op).SquareSizeXZ();
}

Vector3T Vector3T::Project(Vector3TPar op) const
{
    return op * DotProduct(op) * op.InvSquareSize();
}

bool VerifyFloat(float x);

#if _MSC_VER
bool Vector3T::IsFinite() const
{
    if (!VerifyFloat(Get(0)))
        return false;
    if (!VerifyFloat(Get(1)))
        return false;
    if (!VerifyFloat(Get(2)))
        return false;
    return true;
}
#endif

#if !_PIII // special optimization for PIII

void Vector3T::SetFastTransform(const Matrix4T& a, Vector3TPar o)
{
    float r0 = a.Get(0, 0) * o[0] + a.Get(0, 1) * o[1] + a.Get(0, 2) * o[2] + a.GetPos(0);
    float r1 = a.Get(1, 0) * o[0] + a.Get(1, 1) * o[1] + a.Get(1, 2) * o[2] + a.GetPos(1);
    float r2 = a.Get(2, 0) * o[0] + a.Get(2, 1) * o[1] + a.Get(2, 2) * o[2] + a.GetPos(2);
    Set(0) = r0;
    Set(1) = r1;
    Set(2) = r2;
}

#endif

void Vector3T::SetMultiplyLeft(Vector3TPar o, const Matrix3T& a)
{ // vector rotation - only 3x3 matrix is used, translation is ignored
    // u=M*v
    float o0 = o[0], o1 = o[1], o2 = o[2];
    for (int i = 0; i < 3; i++)
    {
        Set(i) = a.Get(0, i) * o0 + a.Get(1, i) * o1 + a.Get(2, i) * o2;
    }
}

void Matrix4T::SetInvertRotation(const Matrix4T& op)
{
    // invert orientation
    _orientation.SetInvertRotation(op.Orientation());
    // invert translation
    SetPosition(Rotate(-op.Position()));
}

void Matrix4T::SetInvertScaled(const Matrix4T& op)
{
    // invert orientation
    _orientation.SetInvertScaled(op.Orientation());
    // invert translation
    Vector3T pos;
    pos.SetMultiply(Orientation(), -op.Position());
    SetPosition(pos);
}

void Matrix4T::SetInvertGeneral(const Matrix4T& op)
{
    // invert orientation
    _orientation.SetInvertGeneral(op.Orientation());
    // invert translation
    SetPosition(Rotate(-op.Position()));
}

void Matrix3T::SetNormalTransform(const Matrix3T& op)
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

void Matrix3T::SetIdentity()
{
    *this = M3IdentityT;
}

void Matrix3T::SetZero()
{
    *this = M3ZeroT;
}

void Matrix3T::SetRotationX(Coord angle)
{
    Coord s = sin(angle), c = cos(angle);
    SetIdentity();
    Set(1, 1) = +c, Set(1, 2) = -s;
    Set(2, 1) = +s, Set(2, 2) = +c;
}

void Matrix3T::SetRotationY(Coord angle)
{
    Coord s = sin(angle), c = cos(angle);
    SetIdentity();
    Set(0, 0) = +c, Set(0, 2) = -s;
    Set(2, 0) = +s, Set(2, 2) = +c;
}

void Matrix3T::SetRotationZ(Coord angle)
{
    Coord s = (Coord)sin(angle), c = (Coord)cos(angle);
    SetIdentity();
    Set(0, 0) = +c, Set(0, 1) = -s;
    Set(1, 0) = +s, Set(1, 1) = +c;
}

void Matrix3T::SetScale(Coord x, Coord y, Coord z)
{
    SetZero();
    Set(0, 0) = x;
    Set(1, 1) = y;
    Set(2, 2) = z;
}

void Matrix3T::SetScale(float scale)
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

float Matrix3T::Scale() const
{
    // scale = average of the row magnitudes of orient*transpose(orient), whose
    // off-diagonal terms are ~0 for a rotation*scale matrix
    // optimized matrix transposition + multiplication
    Vector3T sv;
    for (int i = 0; i < 3; i++)
    {
        sv[i] = Square(Get(i, 0)) + Square(Get(i, 1)) + Square(Get(i, 2));
    }

    // calculate average scale
    float scale2 = (sv[0] + sv[1] + sv[2]) * (1.0 / 3);
    return scale2 * InvSqrt(scale2);
}

float Matrix3T::InvScale() const
{
    // scale = average of the row magnitudes of orient*transpose(orient)
    Vector3T sv;
    for (int i = 0; i < 3; i++)
    {
        sv[i] = Square(Get(i, 0)) + Square(Get(i, 1)) + Square(Get(i, 2));
    }

    // calculate average scale
    float scale2 = (sv[0] + sv[1] + sv[2]) * (1.0 / 3);

    if (scale2 <= 0)
        return 0; // singular matrix
    return InvSqrt(scale2);
}

void Matrix3T::SetDirectionAndUp(Vector3TPar dir, Vector3TPar up)
{
    SetDirection(dir.Normalized());
    // Project into the plane
    Coord t = up * Direction();
    Vector3TVal norm = (up - Direction() * t);
    SetDirectionUp(norm.Normalized());
    // Calculate the vector pointing along the x axis (i.e. aside)
    // sign chosen empirically
    // no need to normalize - cross product of two perpendicular unit vectors is unit vector
    SetDirectionAside(DirectionUp().CrossProduct(Direction()));
}

void Matrix3T::SetDirectionAndAside(Vector3TPar dir, Vector3TPar aside)
{
    SetDirection(dir.Normalized());
    // Project into the plane
    Coord t = aside * Direction();
    Vector3TVal norm = (aside - Direction() * t);
    SetDirectionAside(norm.Normalized());
    // Calculate the vector pointing along the x axis (i.e. aside)
    // sign chosen empirically
    // no need to normalize - cross product of two perpendicular unit vectors is unit vector
    SetDirectionUp(Direction().CrossProduct(DirectionAside()));
}

void Matrix3T::SetUpAndAside(Vector3TPar up, Vector3TPar aside)
{
    SetDirectionUp(up.Normalized());
    // Project into the plane
    Coord t = DirectionUp() * aside;
    Vector3TVal norm = (aside - DirectionUp() * t);
    SetDirectionAside(norm.Normalized());
    // Calculate the vector pointing along the x axis (i.e. aside)
    // sign chosen empirically
    // no need to normalize - cross product of two perpendicular unit vectors is unit vector
    SetDirection(DirectionAside().CrossProduct(DirectionUp()));
}

void Matrix3T::SetUpAndDirection(Vector3TPar up, Vector3TPar dir)
{
    SetDirectionUp(up.Normalized());
    // Project into the plane
    Coord t = DirectionUp() * dir;
    // Calculate the vector pointing along the x axis (i.e. aside)
    // sign chosen empirically
    // no need to normalize - cross product of two perpendicular unit vectors is unit vector
    Vector3TVal norm = (dir - DirectionUp() * t);
    SetDirection(norm.Normalized());
    SetDirectionAside(DirectionUp().CrossProduct(Direction()));
}

void Matrix3T::SetTilda(Vector3TPar a)
{
    SetZero();
    Set(0, 1) = -a[2], Set(0, 2) = +a[1];
    Set(1, 0) = +a[2], Set(1, 2) = -a[0];
    Set(2, 0) = -a[1], Set(2, 1) = +a[0];
}

void Matrix3T::operator*=(float op)
{
    _aside *= op;
    _up *= op;
    _dir *= op;
}

void Matrix3T::SetAdd(const Matrix3T& a, const Matrix3T& b)
{
    _aside = a._aside + b._aside;
    _up = a._up + b._up;
    _dir = a._dir + b._dir;
}

void Matrix3T::SetMultiply(const Matrix3T& a, const Matrix3T& b)
{
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
        {
            Set(i, j) = (a.Get(i, 0) * b.Get(0, j) + a.Get(i, 1) * b.Get(1, j) + a.Get(i, 2) * b.Get(2, j));
        }
}

void Matrix3T::SetMultiply(const Matrix3T& a, float op)
{
    _aside = a._aside * op;
    _up = a._up * op;
    _dir = a._dir * op;
}

void Matrix3T::AddMultiply(const Matrix3T& a, float op)
{
    _aside += a._aside * op;
    _up += a._up * op;
    _dir += a._dir * op;
}

Matrix3T Matrix3T::operator+(const Matrix3T& a) const
{
    Matrix3T res;
    res._aside = _aside + a._aside;
    res._up = _up + a._up;
    res._dir = _dir + a._dir;
    return res;
}

Matrix3T Matrix3T::operator-(const Matrix3T& a) const
{
    Matrix3T res;
    res._aside = _aside - a._aside;
    res._up = _up - a._up;
    res._dir = _dir - a._dir;
    return res;
}

Matrix3T& Matrix3T::operator-=(const Matrix3T& a)
{
    _aside -= a._aside;
    _up -= a._up;
    _dir -= a._dir;
    return *this;
}

Matrix3T& Matrix3T::operator+=(const Matrix3T& a)
{
    _aside += a._aside;
    _up += a._up;
    _dir += a._dir;
    return *this;
}

Matrix4T Matrix4T::operator+(const Matrix4T& a) const
{
    Matrix4T res;
    res._orientation._aside = _orientation._aside + a._orientation._aside;
    res._orientation._up = _orientation._up + a._orientation._up;
    res._orientation._dir = _orientation._dir + a._orientation._dir;
    res._position = _position + a._position;
    return res;
}

Matrix4T Matrix4T::operator-(const Matrix4T& a) const
{
    Matrix4T res;
    res._orientation._aside = _orientation._aside - a._orientation._aside;
    res._orientation._up = _orientation._up - a._orientation._up;
    res._orientation._dir = _orientation._dir - a._orientation._dir;
    res._position = _position - a._position;
    return res;
}

void Matrix4T::operator+=(const Matrix4T& op)
{
    _orientation._aside += op._orientation._aside;
    _orientation._up += op._orientation._up;
    _orientation._dir += op._orientation._dir;
    _position += op._position;
}
void Matrix4T::operator-=(const Matrix4T& op)
{
    _orientation._aside -= op._orientation._aside;
    _orientation._up -= op._orientation._up;
    _orientation._dir -= op._orientation._dir;
    _position -= op._position;
}

void Matrix3T::SetInvertRotation(const Matrix3T& op)
{
    // matrix inversion is calculated based on these prepositions:
    SetIdentity();
    for (int i = 0; i < 3; i++)
    {
        Set(i, 0) = op.Get(0, i);
        Set(i, 1) = op.Get(1, i);
        Set(i, 2) = op.Get(2, i);
    }
}

void Matrix3T::SetInvertScaled(const Matrix3T& op)
{
    // matrix inversion is calculated based on these prepositions:
    // matrix is S*R, where S is scale, R is rotation
    // inversion of such matrix is Inv(S)*Inv(R)
    // Inv(R) is Transpose(R), Inv(S) is C: C(i,i)=1/S(i,i)
    // sizes of row(i) are scale coeficients a,b,c
    // all members are set below, so SetIdentity() is unnecessary
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

void Matrix3T::SetInvertGeneral(const Matrix3T& op)
{
#if 1
    // check if they are really necessary
    // check if matrix is really general
    // if not, we can use scaled version
    // scaled matrix has form S*R, where S is scale matrix and R is rotation
    // (S*R)*(RT*ST)=S*ST=S*S
    //
    Matrix3T t;
    for (int i = 0; i < 3; i++)
    {
        t(0, i) = op(i, 0), t(1, i) = op(i, 1), t(2, i) = op(i, 2);
    }
    Matrix3TVal se = op * t;
    // check if se is diagonal
    const float max = 1e-6;
    if (fabs(se(0, 1)) < max && fabs(se(0, 2)) < max && fabs(se(1, 0)) < max && fabs(se(1, 2)) < max &&
        fabs(se(2, 0)) < max && fabs(se(2, 1)) < max)
    {
        // matrix is diagonal - use special case inversion
        SetInvertScaled(op);
        return;
    }
#endif

    // calculate inversion using Gauss-Jordan elimination
    Matrix3T a = op;
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

void SaturateMin(Vector3T& min, Vector3TPar val)
{
    saturateMin(min[0], val[0]);
    saturateMin(min[1], val[1]);
    saturateMin(min[2], val[2]);
}
void SaturateMax(Vector3T& max, Vector3TPar val)
{
    saturateMax(max[0], val[0]);
    saturateMax(max[1], val[1]);
    saturateMax(max[2], val[2]);
}

void CheckMinMax(Vector3T& min, Vector3T& max, Vector3TPar val)
{
    saturateMin(min[0], val[0]), saturateMax(max[0], val[0]);
    saturateMin(min[1], val[1]), saturateMax(max[1], val[1]);
    saturateMin(min[2], val[2]), saturateMax(max[2], val[2]);
}

Vector3T VectorMin(Vector3TPar a, Vector3TPar b)
{
    return Vector3T(floatMin(a[0], b[0]), floatMin(a[1], b[1]), floatMin(a[2], b[2]));
}

Vector3T VectorMax(Vector3TPar a, Vector3TPar b)
{
    return Vector3T(floatMax(a[0], b[0]), floatMax(a[1], b[1]), floatMax(a[2], b[2]));
}

} // namespace Poseidon::Foundation

#endif
