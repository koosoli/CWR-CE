#pragma once

#include <Poseidon/Foundation/Framework/DebugLog.hpp>
namespace Poseidon::Foundation
{
// One object can inherit several TLink<N> bases for independent list memberships.
template <int i = 0>
class TLink
{
  protected:
    void Reset() { next = nullptr; } // reset without any clean up

  public:
    TLink* next;

    TLink() { next = nullptr; }
    ~TLink() = default;

  private: // disable copy
    TLink(const TLink& src) = delete;
    void operator=(const TLink& src) = delete;
};

#define SLink TLink<0>

// circular list - any item can be used as root

template <class Type, class Base = SLink>
class SList : public Base
{
  protected:
    void Reset() { Base::Reset(); }

  public:
    SList() = default;
    void Insert(Type* element)
    {
        // insert before first element
        element->Base::next = Base::next;
        Base::next = element;
    }
    void DeleteNext(Base* cur, Type* element)
    {
        PoseidonAssert(cur->next == element);
        cur->next = element->Base::next;
        element->Base::next = nullptr;
    }
    void Delete(Type* element)
    {
        Base* cur = this;
        while (cur && cur->next != element)
        {
            cur = cur->next;
            if (!cur)
            {
                Fail("Item not in list");
                return;
            }
        }
        DeleteNext(cur, element);
    }
    void InsertAfter(Type* cur, Type* element)
    {
        element->Base::next = cur->Base::next;
        cur->Base::next = element;
    }
    void Clear()
    {
        while (Start())
        {
            DeleteNext(this, Start());
        }
    }
    Type* Start() const { return static_cast<Type*>(Base::next); }
    bool NotEnd(Type* item) const { return item != nullptr; }
    Type* Advance(Type* item) const { return static_cast<Type*>(item->Base::next); }

    Type* First() const { return static_cast<Type*>(Base::next); }
    Type* Next(Type* cur) const { return static_cast<Type*>(cur->Base::next); }

    void Move(SList& src);

  private: // disable copy
    SList(const SList& src) = delete;
    void operator=(const SList& src) = delete;
};

template <class Type, class Base>
void SList<Type, Base>::Move(SList& src)
{
    // Splice all of src in front of this; src ends up empty.
    Base* srcFirst = src.First();
    if (!srcFirst)
        return;
    Base* first = Base::next;
    Base* last = srcFirst;
    while (last->next)
        last = last->next;
    last->next = first;
    src.Base::next = nullptr;
}

} // namespace Poseidon::Foundation


using ::Poseidon::Foundation::TLink;
using ::Poseidon::Foundation::SList;
