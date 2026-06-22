#include <Poseidon/Input/UserActionDesc.hpp>

#include <cstdarg>

namespace Poseidon
{

UserActionDesc::UserActionDesc(const char* n, int& d, int first, ...) : desc(d)
{
    name = n;
    axis = false;

    keys.Resize(0);
    va_list arglist;
    va_start(arglist, first);
    int key = first;
    while (key != -1)
    {
        keys.Add(key);
        key = va_arg(arglist, int);
    }
    va_end(arglist);
    keys.Compact();
}

UserActionDesc::UserActionDesc(bool a, const char* n, int& d, int first, ...) : desc(d)
{
    name = n;
    axis = a;

    keys.Resize(0);
    va_list arglist;
    va_start(arglist, first);
    int key = first;
    while (key != -1)
    {
        keys.Add(key);
        key = va_arg(arglist, int);
    }
    va_end(arglist);
    keys.Compact();
}
} // namespace Poseidon
