#pragma once

#include <Poseidon/Foundation/Containers/List.hpp>
#include <Poseidon/Foundation/platform.hpp>
#include <Poseidon/Foundation/Containers/Array.hpp>

// simple application of double-linked list
// when object of class derived from RemoveLinks is destructed,
// all links (pointers) to that object are set to nullptr

#define DO_LINK_DIAGS 1


namespace Poseidon::Foundation
{
class BaseLink;
class RemoveLinks;

class BaseLink : public SLink
{
  private:
    RemoveLinks* _ref;

  protected:
    void DoConstruct(RemoveLinks* ref);
    void DoDestruct();

  public:
    RemoveLinks* GetRef() const;

    BaseLink() { _ref = nullptr; }
    BaseLink(RemoveLinks* src) { DoConstruct(src); }
    BaseLink(const BaseLink& src) { DoConstruct(src.GetRef()); }
    BaseLink& operator=(const BaseLink& src)
    {
        // note: src may be *this
        RemoveLinks* ref = src.GetRef();
        DoDestruct();
        DoConstruct(ref);
        return *this;
    }

    __forceinline bool NotNull() const { return _ref != nullptr; }
    __forceinline bool IsNull() const { return _ref == nullptr; }

    __forceinline void Remove() { DoDestruct(); }
    ~BaseLink() { DoDestruct(); }
};

class RemoveLinks : public RefCount, public SList<BaseLink>
{
#if DO_LINK_DIAGS
    friend class BaseLink;
    int _nLinks;
#endif

  public:
    RemoveLinks()
    {
#if DO_LINK_DIAGS
        _nLinks = 0;
#endif
    }

    // on copy of linked object do not copy links
    RemoveLinks(const RemoveLinks& /*src*/)
    {
#if DO_LINK_DIAGS
        _nLinks = 0;
#endif
    }
    void operator=(const RemoveLinks& /*src*/)
    {
#if DO_LINK_DIAGS
        _nLinks = 0;
#endif
        SLink::Reset();
    }

    ~RemoveLinks() override;

#if DO_LINK_DIAGS
    bool VerifyStructure();
#endif
};

typedef RemoveLinks RefCountWithLinks;

inline RemoveLinks::~RemoveLinks()
{ // set all existing references to nullptr
#if DO_LINK_DIAGS
    PoseidonAssert(VerifyStructure());
#endif
    while (First())
    {
        First()->Remove();
    }
#if DO_LINK_DIAGS
    PoseidonAssert(_nLinks == 0);
#endif
}

#if DO_LINK_DIAGS
inline bool RemoveLinks::VerifyStructure()
{
    BaseLink* link = First();
    if (!link)
    {
        return (_nLinks == 0);
    }
    for (int i = 0; i < _nLinks; i++)
    {
        link = Next(link);
        if (!link)
        {
            return (_nLinks == i + 1);
        }
    }
    return false;
}
#endif

inline void BaseLink::DoConstruct(RemoveLinks* ref)
{
    _ref = ref;
    if (_ref)
    {
#if DO_LINK_DIAGS
        PoseidonAssert(_ref->VerifyStructure());
#endif
        _ref->RemoveLinks::Insert(this);
#if DO_LINK_DIAGS
        _ref->_nLinks++;
#endif
    }
}
inline void BaseLink::DoDestruct()
{
    if (_ref)
    {
#if DO_LINK_DIAGS
        PoseidonAssert(_ref->VerifyStructure());
#endif
        _ref->Delete(this);
#if DO_LINK_DIAGS
        _ref->_nLinks--;
#endif
    }
    _ref = nullptr;
}

__forceinline RemoveLinks* BaseLink::GetRef() const
{
    return _ref;
}

template <class Type>
class Link : public BaseLink
{
#if _DEBUG
    Type* _refT; // debug-only: lets the debugger inspect the target

  public:
    Link() : _refT(nullptr) {}
    Link(Type* src) : BaseLink(src), _refT(src) {}
    Link(const Link& src) : BaseLink(src), _refT(src._refT) {}

#else
  public:
    Link() {}
    Link(Type* src) : BaseLink(src) {}
    Link(const Link& src) : BaseLink(src) {}
#endif

    __forceinline Type* GetTypeRef() const { return static_cast<Type*>(GetRef()); }
    __forceinline operator Type*() const { return GetTypeRef(); }
    __forceinline Type& operator*() const { return *GetTypeRef(); }
    __forceinline Type* operator->() const { return GetTypeRef(); }

};

template <class Type>
class LinkArray : public FindArray<Link<Type>>
{
  public:
    int Count() const;
    void Compact();
};

template <class Type>
int LinkArray<Type>::Count() const
{
    if (this->Size() == 0)
    {
        return 0;
    }
    int i, n = 0;
    for (i = 0; i < this->Size(); i++)
    {
        if ((*this)[i])
        {
            n++;
        }
    }
    return n;
}

template <class Type>
void LinkArray<Type>::Compact()
{
    int d = 0;
    for (int s = 0; s < this->Size(); s++)
    {
        if (this->Get(s))
        {
            if (s != d)
            {
                this->Set(d) = this->Get(s);
            }
            d++;
        }
    }
    this->Resize(d);
}

} // namespace Poseidon::Foundation

using ::Poseidon::Foundation::RemoveLinks;
using ::Poseidon::Foundation::RefCountWithLinks;
using ::Poseidon::Foundation::Link;
using ::Poseidon::Foundation::BaseLink;
using ::Poseidon::Foundation::LinkArray;
