#pragma once

#include <Poseidon/Foundation/Framework/DebugLog.hpp>

// Base class for CLList elements. The int template parameter yields distinct
// link types, so one object can inherit several independent lists.

namespace Poseidon::Foundation
{
template <int i = 0>
class CLTLink
{
  protected:
    void Reset() { prev = next = this; } // reset without any clean up

  public:
    CLTLink* prev;
    CLTLink* next;

    CLTLink() { prev = next = nullptr; }
    ~CLTLink()
    {
        if (prev || next)
        {
            PoseidonAssert(prev);
            PoseidonAssert(next);
            Delete();
        }
    }

    void Delete()
    {
        // check it is in list
        PoseidonAssert(prev);
        PoseidonAssert(next);
        // remove element from list

        next->prev = prev;
        prev->next = next;

        // not linked
        prev = next = nullptr;
    }
    bool IsInList() const { return prev != nullptr; }

  private: // disable copy
    CLTLink(const CLTLink& src) = delete;
    void operator=(const CLTLink& src) = delete;
};

typedef CLTLink<0> CLDLink;

// circular list - any item can be used as root

template <class Type, class Base = CLDLink>
class CLList : public Base
{
  protected:
    void Reset() { Base::Reset(); }

  public:
    CLList() { Reset(); }
    ~CLList()
    {
        Clear();
        // prepare for CLTLink destruction
        PoseidonAssert(Base::prev == this);
        PoseidonAssert(Base::next == this);
        Base::prev = Base::next = nullptr;
    }
    void Insert(Type* element)
    {
        // insert before first element
        Base::next->Base::prev = element;
        element->Base::next = Base::next;

        element->Base::prev = this;
        Base::next = element;
    }
    void Add(Type* element)
    {
        // insert after the last element
        Base::prev->Base::next = element;
        element->Base::prev = Base::prev;

        element->Base::next = this;
        Base::prev = element;
    }
    static void InsertBefore(Type* after, Type* element)
    {
        // insert before the element
        after->Base::prev->Base::next = element;
        element->Base::prev = after->Base::prev;

        element->Base::next = after;
        after->Base::prev = element;
    }
    static void InsertAfter(Type* before, Type* element)
    {
        // insert after the element
        before->Base::next->Base::prev = element;
        element->Base::next = before->Base::next;

        element->Base::prev = before;
        before->Base::next = element;
    }

    void Delete(Type* element) { element->Base::Delete(); }

    // use in for loops
    Type* Start() const { return static_cast<Type*>(Base::next); }
    static Type* Advance(Type* item) { return static_cast<Type*>(item->Base::next); }
    bool NotEnd(Type* item) const { return item != (Base*)this; }

    // safe access
    Type* First() const
    {
        if (Base::next == const_cast<CLList*>(this))
        {
            return nullptr;
        }
        PoseidonAssert(Base::next);
        return static_cast<Type*>(Base::next);
    }

    Type* Next(Type* item) const
    {
        if (item->Base::next == const_cast<CLList*>(this))
        {
            return nullptr;
        }
        PoseidonAssert(item->Base::next);
        return static_cast<Type*>(item->Base::next);
    }
    Type* Prev(Type* item) const
    {
        if (item->Base::prev == const_cast<CLList*>(this))
        {
            return nullptr;
        }
        PoseidonAssert(item->Base::prev);
        return static_cast<Type*>(item->Base::prev);
    }

    Type* Last() const
    {
        if (Base::prev == const_cast<CLList*>(this))
        {
            return nullptr;
        }
        PoseidonAssert(Base::prev);
        return static_cast<Type*>(Base::prev);
    }

    void Move(CLList& src);
    void Clear();

    int Size() const;

  private: // disable copy
    CLList(const CLList& src) = delete;
    void operator=(const CLList& src) = delete;
};

template <class Type, class Base>
void CLList<Type, Base>::Move(CLList& src)
{
    // take whole list src and move it into this
    // result: this contains merged list, src is empty
    // take first and last members of src
    Base* srcFirst = src.First();
    Base* srcLast = src.Last();
    if (!srcFirst)
    {
        return; // nothing to merge with
    }
    // srcLast is not nullptr
    // insert first..last before first member of this
    Base* first = Base::next; // first member of this - can be prev (if no members)
    // link last of src with first of this
    first->Base::prev = srcLast, srcLast->Base::next = first;
    // make first of src to be first of this
    Base::next = srcFirst, srcFirst->Base::prev = this;
    src.Base::prev = src.Base::next = &src;
}

template <class Type, class Base>
void CLList<Type, Base>::Clear()
{
    while (First())
    {
        Delete(First());
    }
}

template <class Type, class Base>
int CLList<Type, Base>::Size() const
{
    // note: size may be slow - use it carefully
    int count = 0;
    for (Type* obj = First(); obj; obj = Next(obj))
    {
        count++;
    }
    return count;
}

typedef CLTLink<1> CLRefLink;

template <class Type>
class CLRefList : public CLList<Type, CLRefLink>
{
    typedef CLList<Type, CLRefLink> base;

  public:
    void Clear();
    ~CLRefList() { Clear(); }

    void Insert(Type* element)
    {
        element->AddRef();
        base::Insert(element);
    }
    void Add(Type* element)
    {
        element->AddRef();
        base::Add(element);
    }

    void Delete(Type* element)
    {
        base::Delete(element);
        element->Release();
    }
};

template <class Type>
void CLRefList<Type>::Clear()
{
    while (this->First())
    {
        this->Delete(this->First());
    }
}

} // namespace Poseidon::Foundation

using ::Poseidon::Foundation::CLList;
using ::Poseidon::Foundation::CLRefList;
using ::Poseidon::Foundation::CLTLink;
using ::Poseidon::Foundation::CLDLink;
using ::Poseidon::Foundation::CLRefLink;
