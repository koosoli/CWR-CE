#pragma once

#include <string.h>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Poseidon/Foundation/Containers/Array.hpp>
#include <Poseidon/Foundation/Containers/BankArray.hpp>


namespace Poseidon::Foundation
{
template <class Type, class Traits = BankTraits<Type>>
class BankInitArray : public Traits::ContainerType
{
    typedef typename Traits::NameType TypeNameType;

  protected:
    int Find(TypeNameType name);
    int Load(TypeNameType name);

  public:
    Type* New(TypeNameType name);
};

template <class Type, class Traits>
int BankInitArray<Type, Traits>::Find(TypeNameType name)
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
int BankInitArray<Type, Traits>::Load(TypeNameType name)
{
    Type* item = new Type();
    item->Init(name);
    return this->Add(item);
}

template <class Type, class Traits>
Type* BankInitArray<Type, Traits>::New(TypeNameType name)
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
using ::Poseidon::Foundation::BankInitArray;


