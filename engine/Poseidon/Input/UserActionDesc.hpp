#pragma once

#include <Poseidon/Foundation/Containers/Array.hpp>

namespace Poseidon
{

typedef AutoArray<int> KeyList;

struct UserActionDesc
{
    const char* name;
    int& desc;
    bool axis;
    // default values
    KeyList keys;

    UserActionDesc(const char* n, int& d, int first, ...);
    UserActionDesc(bool a, const char* n, int& d, int first, ...);
};
} // namespace Poseidon

