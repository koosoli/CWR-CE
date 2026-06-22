#pragma once

#include <new> // provides placement operator new/delete

namespace Poseidon::Foundation
{
template <class Type>
void ConstructAt(Type& dst)
{
    new (&dst) Type;
}

template <class Type>
void ConstructAt(Type& dst, const Type& src)
{
    new (&dst) Type(src);
}

} // namespace Poseidon::Foundation

using ::Poseidon::Foundation::ConstructAt;
