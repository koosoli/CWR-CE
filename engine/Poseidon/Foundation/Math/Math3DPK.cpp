
#if defined __ICL && defined _PIII // special optimization for PIII

#include <Poseidon/Foundation/Math/Math3DP.hpp>
#include <xmmintrin.h>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>

// KNI optimizations of some generic routines

#if 1

namespace Poseidon::Foundation
{
void Vector3P::SetFastTransform(const Matrix4P& a, Vector3PPar o)
{
    // u = M*v, assuming the matrix has last row (0,0,0,1); o may alias *this
    float r0 = a.Get(0, 0) * o[0] + a.Get(0, 1) * o[1] + a.Get(0, 2) * o[2] + a.GetPos(0);
    float r1 = a.Get(1, 0) * o[0] + a.Get(1, 1) * o[1] + a.Get(1, 2) * o[2] + a.GetPos(1);
    float r2 = a.Get(2, 0) * o[0] + a.Get(2, 1) * o[1] + a.Get(2, 2) * o[2] + a.GetPos(2);
    Set(0) = r0;
    Set(1) = r1;
    Set(2) = r2;
}

#else

void Vector3P::SetFastTransform(const Matrix4P& a, Vector3PPar o)
{
    // note: o and a must be aligned
    // matrix stored in major-column format
    __m128 t1, t2, t3, t4;

    t1 = _mm_set_ps1(o.X());
    t2 = _mm_set_ps1(o.Y());

    // note: unaligned source data
    t3 = _mm_load_ps((const float*)&a._orientation._aside);
    t4 = _mm_load_ps((const float*)&a._orientation._up);

    t1 = _mm_mul_ps(t1, t3);
    t2 = _mm_mul_ps(t2, t4);

    t3 = _mm_set_ps1(o.Z());

    t4 = _mm_load_ps((const float*)&a._orientation._dir);

    t1 = _mm_add_ps(t1, t2);
    t3 = _mm_mul_ps(t3, t4);

    t4 = _mm_load_ps((const float*)&a._position);

    t3 = _mm_add_ps(t3, t4);
    t2 = _mm_add_ps(t1, t3);
    float temp[4];
    _mm_storeu_ps(temp, t2);
    Set(0) = temp[0];
    Set(1) = temp[1];
    Set(2) = temp[2];
}

} // namespace Poseidon::Foundation

#endif

#endif
