#pragma once

#include <string.h>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Poseidon/Foundation/Containers/Array.hpp>

// default traits (used for most types)

namespace Poseidon::Foundation
{
template <class Type>
struct BankTraits
{
    typedef const char* NameType;
    static int CompareNames(NameType n1, NameType n2) { return strcmpi(n1, n2); }
    typedef RefArray<Type> ContainerType;
};

template <class Type, class Traits = BankTraits<Type>>
class BankArray : public Traits::ContainerType
{
    typedef typename Traits::NameType TypeNameType;
    typedef typename Traits::ContainerType base;

  protected:
    int Find(TypeNameType name);
    int Load(TypeNameType name);

  public:
    Type* New(TypeNameType name);
};

template <class Type, class Traits>
int BankArray<Type, Traits>::Find(TypeNameType name)
{
    for (int i = 0; i < this->Size(); i++)
    {
        Type* ii = this->Get(i);
        if (ii && !Traits::CompareNames(ii->GetName(), name))
        {
            return i;
        }
    }
    return -1;
}

template <class Type, class Traits>
int BankArray<Type, Traits>::Load(TypeNameType name)
{
    int free = base::Find(nullptr);
    if (free >= 0)
    {
        base::Set(free) = new Type(name);
        return free;
    }
    return this->Add(new Type(name));
}

template <class Type, class Traits>
Type* BankArray<Type, Traits>::New(TypeNameType name)
{
    int index = Find(name);
    if (index < 0)
    {
        index = Load(name);
    }
    if (index < 0)
    {
        return nullptr;
    }
    return this->Get(index);
}

} // namespace Poseidon::Foundation
using ::Poseidon::Foundation::BankArray;
using ::Poseidon::Foundation::BankTraits;

