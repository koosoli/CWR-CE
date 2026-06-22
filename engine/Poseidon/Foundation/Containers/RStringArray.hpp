#pragma once

#include <Poseidon/Foundation/Containers/Array.hpp>
#include <Poseidon/Foundation/Strings/RString.hpp>

namespace Poseidon::Foundation
{
// Case-insensitive key traits for RString / RStringB arrays.
struct FindArrayTraitsCI
{
    typedef const char* KeyType;
    static bool IsEqual(const char* a, const char* b) { return strcmpi(a, b) == 0; }
    static const char* GetKey(const RString& a) { return a; }
    static const char* GetKey(const RStringB& a) { return a; }
};

typedef FindArrayKey<RString, FindArrayTraitsCI> FindArrayRStringCI;
typedef FindArrayKey<RStringB, FindArrayTraitsCI> FindArrayRStringBCI;

} // namespace Poseidon::Foundation


using ::Poseidon::Foundation::FindArrayRStringCI;
using ::Poseidon::Foundation::FindArrayRStringBCI;
using ::Poseidon::Foundation::FindArrayTraitsCI;
