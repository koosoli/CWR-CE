#pragma once

#include <math.h>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>

// n-dimensional matrix algebra

namespace Poseidon::Foundation
{
typedef float Real;

template <int N, int M>
class Matrix
{
    Real _e[N][M];

  public:
    Real Get(int i, int j) const { return _e[i][j]; }
    Real& Set(int i, int j) { return _e[i][j]; }

  public:
    enum RC
    {
        Rows = N,
        Columns = M
    };

    Real operator()(int i, int j) const { return Get(i, j); }
    Real& operator()(int i, int j) { return Set(i, j); }

    Matrix<M, N> Transposed() const;
    // note: inverse valid only for NxN
    Matrix<M, N> Inversed(bool* regular = nullptr) const;

    void InitIdentity();
    void InitZero();
    bool InitInversed(const Matrix<M, N>& op);
};

template <int N>
class Vector : public Matrix<N, 1>
{
  public:
    Real operator[](int i) const { return Matrix<N, 1>::operator()(i, 0); }
    Real& operator[](int i) { return Matrix<N, 1>::operator()(i, 0); }
};

template <int N>
class RowVector : public Matrix<1, N>
{
    Real operator[](int i) const { return Matrix<1, N>::operator()(0, i); }
    Real& operator[](int i) { return Matrix<1, N>::operator()(0, i); }
};

template <class AC, class AB, class BC>
void Multiply(AC& ac, const AB& ab, const BC& bc)
{
    PoseidonAssert(static_cast<int>(AC::Rows) == static_cast<int>(AB::Rows));
    PoseidonAssert(static_cast<int>(AC::Columns) == static_cast<int>(BC::Columns));
    PoseidonAssert(static_cast<int>(BC::Rows) == static_cast<int>(AB::Columns));
    for (int a = 0; a < AC::Rows; a++)
    {
        for (int c = 0; c < AC::Columns; c++)
        {
            Real res = 0;
            for (int b = 0; b < AB::Columns; b++)
            {
                res += ab(a, b) * bc(b, c);
            }
            ac(a, c) = res;
        }
    }
}

template <int N, int M>
void Matrix<N, M>::InitIdentity()
{
    PoseidonAssert(N == M);
    int i, j;
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < M; j++)
        {
            Set(i, j) = 0;
        }
    }
    for (i = 0; i < N; i++)
    {
        Set(i, i) = 1;
    }
}
template <int N, int M>
void Matrix<N, M>::InitZero()
{
    int i, j;
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < M; j++)
        {
            Set(i, j) = 0;
        }
    }
}

#define swap(a, b) \
    {              \
        float p;   \
        p = a;     \
        a = b;     \
        b = p;     \
    }

template <int N, int M>
bool Matrix<N, M>::InitInversed(const Matrix<M, N>& op)
{
    // calculate inversion using Gauss-Jordan elimination
    Matrix<M, N> a = op;
    // load result with identity
    InitIdentity();
    int row, col;
    // construct result by pivoting
    // pivot column
    for (col = 0; col < N; col++)
    {
        // use maximal number as pivot
        float max = 0;
        int maxRow = col;
        for (row = col; row < M; row++)
        {
            float mag = fabs(a.Get(row, col));
            if (max < mag)
            {
                max = mag, maxRow = row;
            }
        }
        if (max <= 0.0)
        {
            return false; // no pivot exists
        }
        // swap lines col and maxRow
        int i;
        for (i = 0; i < N; i++)
        {
            swap(a.Set(col, i), a.Set(maxRow, i));
            swap(Set(col, i), Set(maxRow, i));
        }
        // use a(col,col) as pivot
        float quotient = 1 / a.Get(col, col);
        // make pivot 1
        for (i = 0; i < N; i++)
        {
            a.Set(col, i) *= quotient;
            Set(col, i) *= quotient;
        }
        // use pivot line to zero all other lines
        for (row = 0; row < M; row++)
        {
            if (row != col)
            {
                float factor = a.Get(row, col);
                for (i = 0; i < N; i++)
                {
                    a.Set(row, i) -= a.Get(col, i) * factor;
                    Set(row, i) -= Get(col, i) * factor;
                }
            }
        }
    }
    // result constructed
    return true;
}

#undef swap

template <int N, int M>
Matrix<M, N> Matrix<N, M>::Transposed() const
{
    Matrix<M, N> res;
    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < M; j++)
        {
            res(i, j) = Get(j, i);
        }
    }
    return res;
}

template <int N, int M>
Matrix<M, N> Matrix<N, M>::Inversed(bool* regular) const
{
    // note: inverse valid only for NxN
    Matrix<M, N> res;
    bool ret = res.InitInversed(*this);
    if (regular)
    {
        *regular = ret;
    }
    return res;
}

} // namespace Poseidon::Foundation
