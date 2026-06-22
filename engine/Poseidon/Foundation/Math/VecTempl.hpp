#pragma once
#include <Poseidon/Foundation/Math/MathOpt.hpp>

// Expression templates for fixed-dimension vector math.

#include <Poseidon/Foundation/platform.hpp>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#pragma warning(disable : 4786)

namespace Poseidon::Foundation
{
namespace vt
{ // using namespace: usefull to hide operator + from conflicting

// short name is used to avoid warning about long names as much as possible

// templates together with namespaces lead to very long symbols

template <class ta_a>
class Arg
{
    const ta_a& Argv;

  public:
    enum
    {
        Dimension = ta_a::Dimension
    };
    __forceinline Arg(const ta_a& A) : Argv(A) {}
    __forceinline const float Evaluate(const int i) const { return Argv.Evaluate(i); }
};

// handling of general member function on any expression supporting "Evaluate"

template <int ta_dimension>
struct Func
{
    template <class ta, class tb>
    struct dot_prod
    {
        template <int I, class R>
        struct recurse
        {
            enum
            {
                COUNTER = I + 1
            };

            static __forceinline float Eval(const ta& A, const tb& B)
            {
                return A.Evaluate(I) * B.Evaluate(I) + recurse<COUNTER, int>::Eval(A, B);
            }
        };

        template <>
        struct recurse<ta_dimension, int>
        {
            static __forceinline float Eval(const ta& A, const tb& B) { return 0; }
        };

        static __forceinline float Eval(const ta& A, const tb& B) { return recurse<0, int>::Eval(A, B); }
    };

    template <class ta, class tb>
    struct equal
    {
        template <int I, class R>
        struct recurse
        {
            enum
            {
                COUNTER = I + 1
            };

            static __forceinline bool Eval(const ta& A, const tb& B)
            {
                return A.Evaluate(I) == B.Evaluate(I) && recurse<COUNTER, int>::Eval(A, B);
            }
        };

        template <>
        struct recurse<ta_dimension, int>
        {
            static __forceinline bool Eval(const ta& A, const tb& B) { return true; }
        };

        static __forceinline bool Eval(const ta& A, const tb& B) { return recurse<0, int>::Eval(A, B); }
    };

    // modulus = square size (sum of squares)
    template <class ta>
    struct modulus
    {
        template <int I, class R>
        struct recurse
        {
            enum
            {
                COUNTER = I + 1
            };

            static __forceinline float Eval(const ta& A)
            {
                float t = A.Evaluate(I);
                return t * t + recurse<COUNTER, int>::Eval(A);
            }
        };

        template <>
        struct recurse<ta_dimension, int>
        {
            static __forceinline float Eval(const ta& A) { return 0; }
        };

        static __forceinline float Eval(const ta& A) { return recurse<0, int>::Eval(A); }
    };

    template <class ta>
    struct xzmodulus
    {
        // modulus of members 0,2,4,...
        template <int I, class R>
        struct recurse
        {
            enum
            {
                COUNTER = I + 2
            };

            static __forceinline float Eval(const ta& A)
            {
                float t = A.Evaluate(I);
                return t * t + recurse<COUNTER, int>::Eval(A);
            }
        };

        template <>
        struct recurse<ta_dimension, int>
        {
            static __forceinline float Eval(const ta& A) { return 0; }
        };
        enum
        {
            ta_dimension1 = ta_dimension + 1
        };
        template <>
        struct recurse<ta_dimension1, int>
        {
            static __forceinline float Eval(const ta& A) { return 0; }
        };

        static __forceinline float Eval(const ta& A) { return recurse<0, int>::Eval(A); }
    };
};

// partial specialization: some functions supported only for 3D vectors

template <class ta_a, class ta_b, class ta_eval>
class Exp2 : Func<ta_a::Dimension>
{
    const Arg<ta_a> Arg1;
    const Arg<ta_b> Arg2;

  public:
    // note: we assume ta_a is same dimension as ta_b dimension
    enum
    {
        Dimension = ta_a::Dimension
    };
    __forceinline Exp2(const ta_a& A1, const ta_b& A2) : Arg1(A1), Arg2(A2) {}
    __forceinline const float Evaluate(const int i) const { return ta_eval::Evaluate(i, Arg1, Arg2); }

    float SquareSize() const { return Func<ta_a::Dimension>::template modulus<Exp2>::Eval(*this); }
    __forceinline float SquareSizeInline() const { return Func<ta_a::Dimension>::template modulus<Exp2>::Eval(*this); }

    float SquareSizeXZ() const { return Func<ta_a::Dimension>::template xzmodulus<Exp2>::Eval(*this); }

    float Size() const
    {
        float size2 = SquareSizeXZ();
        if (size2 == 0)
            return 0;
        float invSize = InvSqrt(size2);
        return size2 * invSize;
    }
    float SizeXZ() const
    {
        float size2 = SquareSizeInline();
        if (size2 == 0)
            return 0;
        float invSize = InvSqrt(size2);
        return size2 * invSize;
    }
    float InvSize() const
    {
        float size2 = SquareSizeXZ();
        if (size2 == 0)
            return 0;
        return InvSqrt(size2);
    }

    // normalization
    // note: some functions need to be implemented without recursion
    // it might introduce some problems

    template <class ta_type>
    __forceinline float DotProduct(const ta_type& A) const
    {
        return Func<ta_a::Dimension>::template dot_prod<Exp2, ta_type>::Eval(*this, A);
    }

    template <class ta_type>
    float CosAngle(const ta_type& op) const
    {
        PoseidonAssert(ta_a::Dimension == 3);
        PoseidonAssert(ta_type::Dimension == 3);
        return DotProduct(op) * InvSqrt(op.SquareSizeInline() * SquareSizeInline());
    }
};

// vector by float
template <class ta_a, class ta_eval>
class Exp2vf
{
    const Arg<ta_a> Arg1;
    const float Arg2;

  public:
    enum
    {
        Dimension = ta_a::Dimension
    };
    __forceinline Exp2vf(const ta_a& A1, const float& A2) : Arg1(A1), Arg2(A2) {}
    __forceinline const float Evaluate(const int i) const { return ta_eval::Evaluate(i, Arg1, Arg2); }
};

// float by vector
template <class ta_b, class ta_eval>
class Exp2fv
{
    const float Arg1;
    const Arg<ta_b> Arg2;

  public:
    enum
    {
        Dimension = ta_b::Dimension
    };
    __forceinline Exp2fv(const float& A1, const ta_b& A2) : Arg1(A1), Arg2(A2) {}
    __forceinline const float Evaluate(const int i) const { return ta_eval::Evaluate(i, Arg1, Arg2); }
};

template <class ta_a, class ta_eval>
class vecexp_1
{
    const Arg<ta_a> Arg1;

  public:
    enum
    {
        Dimension = ta_a::Dimension
    };
    __forceinline vecexp_1(const ta_a& A1) : Arg1(A1) {}

    __forceinline const float Evaluate(const int i) const { return ta_eval::Evaluate(i, Arg1.Evaluate(i)); }
};

///////////////////////////////////////////////////////////////////////////
// BASE CLASS
///////////////////////////////////////////////////////////////////////////

// note: it should be possible to pass scalar type as template argument
template <int ta_dimension>
class EBase : public Func<ta_dimension>
{
  protected:
    float _e[ta_dimension];

  public:
    enum
    {
        Dimension = ta_dimension
    };

    __forceinline const float& Get(const int i) const { return _e[i]; }
    __forceinline float& Set(const int i) { return _e[i]; }
    __forceinline float& operator[](const int i) { return _e[i]; }
    __forceinline const float& operator[](const int i) const { return _e[i]; }
    __forceinline const float Evaluate(const int i) const { return _e[i]; }

    //////////////////////////////////////////////////////////////////
    // ASSIGMENT
    //////////////////////////////////////////////////////////////////
    template <class ta>
    struct Assignment
    {
        // template< int I, class R>
        template <int I, class R>
        struct recurse
        {
            enum
            {
                COUNTER = I + 1
            };

            static __forceinline void Assign(EBase<ta_dimension>& V, const ta& A,
                                             void Operator(float& a, const float& b))
            {
                Operator(V[I], A.Evaluate(I));
                recurse<COUNTER, int>::Assign(V, A, Operator);
            }
        };

        template <>
        struct recurse<ta_dimension, int>
        {
            static __forceinline void Assign(EBase<ta_dimension>& V, const ta& A,
                                             void Operator(float& a, const float& b))
            {
            }
        };

        static __forceinline void Assign(EBase<ta_dimension>& V, const ta& A, void Operator(float& a, const float& b))
        {
            recurse<0, int>::Assign(V, A, Operator);
        }
    };

    __forceinline static void AssignFloat(float& a, const float& b) { a = b; }

    template <class ta_type>
    __forceinline const EBase<ta_dimension>& operator=(const ta_type& A)
    {
        Assignment<ta_type>::Assign(*this, A, AssignFloat);
        return *this;
    }

    __forceinline static void AssignAddFloat(float& a, const float& b) { a += b; }

    template <class ta_type>
    __forceinline const EBase<ta_dimension>& operator+=(const ta_type& A)
    {
        Assignment<ta_type>::Assign(*this, A, AssignAddFloat);
        return *this;
    }

    __forceinline static void AssignSubFloat(float& a, const float& b) { a -= b; }

    template <class ta_type>
    __forceinline const EBase<ta_dimension>& operator-=(const ta_type& A)
    {
        Assignment<ta_type>::Assign(*this, A, AssignSubFloat);
        return *this;
    }

    __forceinline float SquareSize() const { return Func<ta_dimension>::template modulus<EBase>::Eval(*this); }
    __forceinline float SquareSizeInline() const { return Func<ta_dimension>::template modulus<EBase>::Eval(*this); }

    __forceinline float SquareSizeXZ() const { return Func<ta_dimension>::template xzmodulus<EBase>::Eval(*this); }

    template <class ta_type>
    __forceinline bool operator!=(const ta_type& A) const
    {
        return !Func<ta_dimension>::template equal<EBase, ta_type>::Eval(*this, A);
    }

    template <class ta_type>
    __forceinline bool operator==(const ta_type& A) const
    {
        return Func<ta_dimension>::template equal<EBase, ta_type>::Eval(*this, A);
    }

    float Size() const
    {
        float size2 = SquareSizeInline();
        if (size2 == 0)
            return 0;
        float invSize = InvSqrt(size2);
        return size2 * invSize;
    }
    float SizeXZ() const
    {
        float size2 = SquareSizeXZ();
        if (size2 == 0)
            return 0;
        float invSize = InvSqrt(size2);
        return size2 * invSize;
    }
    float InvSize() const
    {
        float size2 = SquareSizeInline();
        if (size2 == 0)
            return 0;
        return InvSqrt(size2);
    }

    // normalization
    // note: some functions need to be implemented without recursion
    // it might introduce some problems

    template <class ta_type>
    __forceinline float DotProduct(const ta_type& A) const
    {
        return Func<ta_dimension>::template dot_prod<EBase, ta_type>::Eval(*this, A);
    }

    template <class ta_type>
    float CosAngle(const ta_type& op) const
    {
        PoseidonAssert(ta_dimension == 3);
        return DotProduct(op) * InvSqrt(op.SquareSizeInline() * SquareSizeInline());
    }
};

struct Add
{
    template <class ta_a, class ta_b>
    __forceinline static const float Evaluate(const int i, const ta_a& A, const ta_b& B)
    {
        return A.Evaluate(i) + B.Evaluate(i);
    }
};

template <class ta_c1, class ta_c2>
__forceinline const Exp2<const ta_c1, const ta_c2, Add> operator+(const ta_c1& Pa, const ta_c2& Pb)
{
    return Exp2<const ta_c1, const ta_c2, Add>(Pa, Pb);
}

struct Sub
{
    template <class ta_a, class ta_b>
    __forceinline static const float Evaluate(const int i, const ta_a& A, const ta_b& B)
    {
        return A.Evaluate(i) - B.Evaluate(i);
    }
};

template <class ta_c1, class ta_c2>
__forceinline const Exp2<const ta_c1, const ta_c2, Sub> operator-(const ta_c1& Pa, const ta_c2& Pb)
{
    return Exp2<const ta_c1, const ta_c2, Sub>(Pa, Pb);
}

struct MulFV
{
    template <class ta_a>
    __forceinline static const float Evaluate(const int i, const ta_a& A, const float& B)
    {
        return A.Evaluate(i) * B;
    }
};

template <class ta_c1>
__forceinline const Exp2vf<const ta_c1, MulFV> operator*(const ta_c1& Pa, const float& Pb)
{
    return Exp2vf<const ta_c1, MulFV>(Pa, Pb);
}

struct MulVF
{
    template <class ta_b>
    __forceinline static const float Evaluate(const int i, const float& A, const ta_b& B)
    {
        return A * B.Evaluate(i);
    }
};

template <class ta_c1>
__forceinline const Exp2fv<const ta_c1, MulVF> operator*(const float& Pa, const ta_c1& Pb)
{
    return Exp2fv<const ta_c1, MulVF>(Pa, Pb);
}

}; // namespace vt

} // namespace Poseidon::Foundation
