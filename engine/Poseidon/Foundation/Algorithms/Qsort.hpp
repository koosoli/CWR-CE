#pragma once

#ifndef POSEIDON_BASE_ALGORITHMS_QSORT_HPP
#define POSEIDON_BASE_ALGORITHMS_QSORT_HPP

#include <utility>

namespace Poseidon::Foundation
{
namespace PoseidonBase::Algorithms::Detail
{
// Swap two elements.
template <class Type>
inline void Swap(Type* p, Type* q)
{
    std::swap(*p, *q);
}

// Insertion sort for short sub-arrays; sorts [lo,hi] in place (assumes lo < hi).
// comp(a, b) returns neg if *a<*b, 0 if equal, pos if *a>*b.
template <class Type>
inline void ShortSort(Type* lo, Type* hi, int (*comp)(const Type*, const Type*))
{
    Type *p, *max;

    /* Note: in assertions below, i and j are alway inside original bound of
       array to sort. */

    while (hi > lo)
    {
        /* A[i] <= A[j] for i <= j, j > hi */
        max = lo;
        for (p = lo + 1; p <= hi; p += 1)
        {
            /* A[i] <= A[max] for lo <= i < p */
            if (comp(p, max) > 0)
            {
                max = p;
            }
            /* A[i] <= A[max] for lo <= i <= p */
        }

        /* A[i] <= A[max] for lo <= i <= hi */

        Swap(max, hi);

        /* A[i] <= A[hi] for i <= hi, so A[i] <= A[j] for i <= j, j >= hi */

        hi -= 1;

        /* A[i] <= A[j] for i <= j, j > hi, loop top condition established */
    }
    /* A[i] <= A[j] for i <= j, j > lo, which implies A[i] <= A[j] for i < j,
       so array is sorted */
}

// functor implementation
template <class Type, class ContextType, class Compare>
inline void ShortSort(Type* lo, Type* hi, ContextType context, Compare comp)
{
    Type *p, *max;

    /* Note: in assertions below, i and j are alway inside original bound of
       array to sort. */

    while (hi > lo)
    {
        /* A[i] <= A[j] for i <= j, j > hi */
        max = lo;
        for (p = lo + 1; p <= hi; p += 1)
        {
            /* A[i] <= A[max] for lo <= i < p */
            if (comp(p, max, context) > 0)
            {
                max = p;
            }
            /* A[i] <= A[max] for lo <= i <= p */
        }

        /* A[i] <= A[max] for lo <= i <= hi */

        Swap(max, hi);

        /* A[i] <= A[hi] for i <= hi, so A[i] <= A[j] for i <= j, j >= hi */

        hi -= 1;

        /* A[i] <= A[j] for i <= j, j > hi, loop top condition established */
    }
    /* A[i] <= A[j] for i <= j, j > lo, which implies A[i] <= A[j] for i < j,
       so array is sorted */
}

/* this parameter defines the cutoff between using quick sort and
   insertion sort for arrays; arrays with lengths shorter or equal to the
   below value use insertion sort */

inline constexpr unsigned int QSortCutoff = 8;
} // namespace PoseidonBase::Algorithms::Detail

// Quicksort an array in place via pseudo-recursion (explicit stack, no real recursion).
// comp(a, b) returns neg if *a<*b, 0 if equal, pos if *a>*b.
template <class Type>
void QSort(Type* base, int num, int (*comp)(const Type*, const Type*))
{
    Type *lo, *hi;       /* ends of sub-array currently sorting */
    Type* mid;           /* points to middle of subarray */
    Type *loguy, *higuy; /* traveling pointers for partition step */
    unsigned size;       /* size of the sub-array */
    Type *lostk[30], *histk[30];
    int stkptr; /* stack for saving sub-array to be processed */

    /* Note: the number of stack entries required is no more than
       1 + log2(size), so 30 is sufficient for any array */

    if (num < 2)
    {
        return; /* nothing to do */
    }

    stkptr = 0; /* initialize stack */

    lo = base;
    hi = base + (num - 1); /* initialize limits */

    /* this entry point is for pseudo-recursion calling: setting
       lo and hi and jumping to here is like recursion, but stkptr is
       prserved, locals aren't, so we preserve stuff on the stack */
recurse:

    size = (hi - lo) + 1; /* number of el's to sort */

    /* below a certain size, it is faster to use a O(n^2) sorting method */
    if (size <= PoseidonBase::Algorithms::Detail::QSortCutoff)
    {
        PoseidonBase::Algorithms::Detail::ShortSort(lo, hi, comp);
    }
    else
    {
        /* First we pick a partititioning element.  The efficiency of the
           algorithm demands that we find one that is approximately the
           median of the values, but also that we select one fast.  Using
           the first one produces bad performace if the array is already
           sorted, so we use the middle one, which would require a very
           wierdly arranged array for worst case performance.  Testing shows
           that a median-of-three algorithm does not, in general, increase
           performance. */

        mid = lo + (size / 2); /* find middle element */
        PoseidonBase::Algorithms::Detail::Swap(mid, lo); /* swap it to beginning of array */

        /* We now wish to partition the array into three pieces, one
           consisiting of elements <= partition element, one of elements
           equal to the parition element, and one of element >= to it.  This
           is done below; comments indicate conditions established at every
           step. */

        loguy = lo;
        higuy = hi + 1;

        /* Note that higuy decreases and loguy increases on every iteration,
           so loop must terminate. */
        for (;;)
        {
            /* lo <= loguy < hi, lo < higuy <= hi + 1,
               A[i] <= A[lo] for lo <= i <= loguy,
               A[i] >= A[lo] for higuy <= i <= hi */

            do
            {
                loguy++;
            } while (loguy <= hi && comp(loguy, lo) <= 0);

            /* lo < loguy <= hi+1, A[i] <= A[lo] for lo <= i < loguy,
               either loguy > hi or A[loguy] > A[lo] */

            do
            {
                higuy--;
            } while (higuy > lo && comp(higuy, lo) >= 0);

            /* lo-1 <= higuy <= hi, A[i] >= A[lo] for higuy < i <= hi,
               either higuy <= lo or A[higuy] < A[lo] */

            if (higuy < loguy)
            {
                break;
            }

            /* if loguy > hi or higuy <= lo, then we would have exited, so
               A[loguy] > A[lo], A[higuy] < A[lo],
               loguy < hi, highy > lo */

            PoseidonBase::Algorithms::Detail::Swap(loguy, higuy);

            /* A[loguy] < A[lo], A[higuy] > A[lo]; so condition at top
               of loop is re-established */
        }

        /*     A[i] >= A[lo] for higuy < i <= hi,
               A[i] <= A[lo] for lo <= i < loguy,
               higuy < loguy, lo <= higuy <= hi
           implying:
               A[i] >= A[lo] for loguy <= i <= hi,
               A[i] <= A[lo] for lo <= i <= higuy,
               A[i] = A[lo] for higuy < i < loguy */

        PoseidonBase::Algorithms::Detail::Swap(lo, higuy); /* put partition element in place */

        /* OK, now we have the following:
              A[i] >= A[higuy] for loguy <= i <= hi,
              A[i] <= A[higuy] for lo <= i < higuy
              A[i] = A[lo] for higuy <= i < loguy    */

        /* We've finished the partition, now we want to sort the subarrays
           [lo, higuy-1] and [loguy, hi].
           We do the smaller one first to minimize stack usage.
           We only sort arrays of length 2 or more.*/

        if (higuy - 1 - lo >= hi - loguy)
        {
            if (lo + 1 < higuy)
            {
                lostk[stkptr] = lo;
                histk[stkptr] = higuy - 1;
                ++stkptr;
            } /* save big recursion for later */

            if (loguy < hi)
            {
                lo = loguy;
                goto recurse; /* do small recursion */
            }
        }
        else
        {
            if (loguy < hi)
            {
                lostk[stkptr] = loguy;
                histk[stkptr] = hi;
                ++stkptr; /* save big recursion for later */
            }

            if (lo + 1 < higuy)
            {
                hi = higuy - 1;
                goto recurse; /* do small recursion */
            }
        }
    }

    /* We have sorted the array, except for any pending sorts on the stack.
       Check if there are any, and do them. */

    --stkptr;
    if (stkptr >= 0)
    {
        lo = lostk[stkptr];
        hi = histk[stkptr];
        goto recurse; /* pop subarray from stack */
    }
    else
    {
        return; /* all subarrays done */
    }
}

// context passing version
template <class Type, class ContextType>
void QSort(Type* base, int num, ContextType context, int (*comp)(const Type*, const Type*, ContextType))
{
    Type *lo, *hi;       /* ends of sub-array currently sorting */
    Type* mid;           /* points to middle of subarray */
    Type *loguy, *higuy; /* traveling pointers for partition step */
    unsigned size;       /* size of the sub-array */
    Type *lostk[30], *histk[30];
    int stkptr; /* stack for saving sub-array to be processed */

    if (num < 2)
    {
        return; /* nothing to do */
    }

    stkptr = 0; /* initialize stack */

    lo = base;
    hi = base + (num - 1); /* initialize limits */

recurse:

    size = (hi - lo) + 1; /* number of el's to sort */

    if (size <= PoseidonBase::Algorithms::Detail::QSortCutoff)
    {
        PoseidonBase::Algorithms::Detail::ShortSort(lo, hi, context, comp);
    }
    else
    {
        mid = lo + (size / 2); /* find middle element */
        PoseidonBase::Algorithms::Detail::Swap(mid, lo); /* swap it to beginning of array */

        loguy = lo;
        higuy = hi + 1;

        for (;;)
        {
            do
            {
                loguy++;
            } while (loguy <= hi && comp(loguy, lo, context) <= 0);

            do
            {
                higuy--;
            } while (higuy > lo && comp(higuy, lo, context) >= 0);

            if (higuy < loguy)
            {
                break;
            }

            PoseidonBase::Algorithms::Detail::Swap(loguy, higuy);
        }

        PoseidonBase::Algorithms::Detail::Swap(lo, higuy); /* put partition element in place */

        if (higuy - 1 - lo >= hi - loguy)
        {
            if (lo + 1 < higuy)
            {
                lostk[stkptr] = lo;
                histk[stkptr] = higuy - 1;
                ++stkptr;
            } /* save big recursion for later */

            if (loguy < hi)
            {
                lo = loguy;
                goto recurse; /* do small recursion */
            }
        }
        else
        {
            if (loguy < hi)
            {
                lostk[stkptr] = loguy;
                histk[stkptr] = hi;
                ++stkptr; /* save big recursion for later */
            }

            if (lo + 1 < higuy)
            {
                hi = higuy - 1;
                goto recurse; /* do small recursion */
            }
        }
    }

    --stkptr;
    if (stkptr >= 0)
    {
        lo = lostk[stkptr];
        hi = histk[stkptr];
        goto recurse; /* pop subarray from stack */
    }
    else
    {
        return; /* all subarrays done */
    }
}

// context passing version
template <class Type, class ContextType, class Compare>
void QSortWithContext(Type* base, int num, ContextType context, Compare comp)
{
    Type *lo, *hi;       /* ends of sub-array currently sorting */
    Type* mid;           /* points to middle of subarray */
    Type *loguy, *higuy; /* traveling pointers for partition step */
    unsigned size;       /* size of the sub-array */
    Type *lostk[30], *histk[30];
    int stkptr; /* stack for saving sub-array to be processed */

    if (num < 2)
    {
        return; /* nothing to do */
    }

    stkptr = 0; /* initialize stack */

    lo = base;
    hi = base + (num - 1); /* initialize limits */

recurse:

    size = (hi - lo) + 1; /* number of el's to sort */

    if (size <= PoseidonBase::Algorithms::Detail::QSortCutoff)
    {
        PoseidonBase::Algorithms::Detail::ShortSort(lo, hi, context, comp);
    }
    else
    {
        mid = lo + (size / 2); /* find middle element */
        PoseidonBase::Algorithms::Detail::Swap(mid, lo); /* swap it to beginning of array */

        loguy = lo;
        higuy = hi + 1;

        for (;;)
        {
            do
            {
                loguy++;
            } while (loguy <= hi && comp(loguy, lo, context) <= 0);

            do
            {
                higuy--;
            } while (higuy > lo && comp(higuy, lo, context) >= 0);

            if (higuy < loguy)
            {
                break;
            }

            PoseidonBase::Algorithms::Detail::Swap(loguy, higuy);
        }

        PoseidonBase::Algorithms::Detail::Swap(lo, higuy); /* put partition element in place */

        if (higuy - 1 - lo >= hi - loguy)
        {
            if (lo + 1 < higuy)
            {
                lostk[stkptr] = lo;
                histk[stkptr] = higuy - 1;
                ++stkptr;
            } /* save big recursion for later */

            if (loguy < hi)
            {
                lo = loguy;
                goto recurse; /* do small recursion */
            }
        }
        else
        {
            if (loguy < hi)
            {
                lostk[stkptr] = loguy;
                histk[stkptr] = hi;
                ++stkptr; /* save big recursion for later */
            }

            if (lo + 1 < higuy)
            {
                hi = higuy - 1;
                goto recurse; /* do small recursion */
            }
        }
    }

    --stkptr;
    if (stkptr >= 0)
    {
        lo = lostk[stkptr];
        hi = histk[stkptr];
        goto recurse; /* pop subarray from stack */
    }
    else
    {
        return; /* all subarrays done */
    }
}

} // namespace Poseidon::Foundation

#endif
