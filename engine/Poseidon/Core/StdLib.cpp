#include <Poseidon/Foundation/Common/FltOpts.hpp>

#if defined _MSC_VER && !defined _DEBUG && !defined _DLL
// MS VC++ strcmpi are very bad on PPro/PII
// is causes many partial stalls and serializations

namespace Poseidon
{
char* CCALL strlwr(char* a)
{
    char* saveA = a;
    while (*a)
    {
        *a = myLower(*a);
        a++;
    }
    return saveA;
}

char* CCALL strupr(char* a)
{
    char* saveA = a;
    while (*a)
    {
        *a = myUpper(*a);
        a++;
    }
    return saveA;
}

int CCALL strcmpi(const char* a, const char* b)
{
    for (;;)
    {
        char la = myLower(*a++);
        char lb = myLower(*b++);
        if (la != lb)
            return la - lb;
        if (la == 0)
            return 0;
    }
    // if *a==0, *b!=0, a<b (a shorter)
    // if *a!=0, *b==0, a>b (a longer)
    /*NOTREACHED*/
}
int CCALL stricmp(const char* a, const char* b)
{
    for (;;)
    {
        char la = myLower(*a++);
        char lb = myLower(*b++);
        if (la != lb)
            return la - lb;
        if (la == 0)
            return 0;
    }
    /*NOTREACHED*/
}

int CCALL strnicmp(const char* a, const char* b, int n)
{
    for (;;)
    {
        if (--n < 0)
            return 0;
        char la = myLower(*a++);
        char lb = myLower(*b++);
        if (la != lb)
            return la - lb;
        if (la == 0)
            return 0;
    }
    /*NOTREACHED*/
}

} // namespace Poseidon

#endif
