#pragma once

#include <Poseidon/Foundation/Types/Pointers.hpp>
#include <Poseidon/Foundation/Memory/FastAlloc.hpp>
#include <Poseidon/Foundation/Containers/Array.hpp>


namespace Poseidon::Foundation
{
class TrackLLinks;
class RemoveLLinks;

class RemoveLLinks : public RefCount
{
    friend class TrackLLinks;
    TrackLLinks* _track;

  public:
    RemoveLLinks() { _track = nullptr; }
    RemoveLLinks(const RemoveLLinks& src)
    {
        (void)src;
        _track = nullptr;
    }
    void operator=(const RemoveLLinks& src)
    {
        if (this == &src)
        {
            return;
        }
        (void)src;
        _track = nullptr;
    }

    ~RemoveLLinks() override;

    __forceinline TrackLLinks* Track() const { return _track; }
};

class TrackLLinks : public RefCount
{
    friend class RemoveLLinks;

    RemoveLLinks* _remove;

  public:
    TrackLLinks() = default;
    explicit TrackLLinks(RemoveLLinks* obj)
    {
        _remove = obj;
        if (obj)
        {
            PoseidonAssert(!obj->_track);
            obj->_track = this;
        }
    }
    ~TrackLLinks() override
    {
        if (_remove)
        {
            _remove->_track = nullptr;
        }
        _remove = nullptr;
    }

    __forceinline RemoveLLinks* GetObject() const { return _remove; }

    USE_FAST_ALLOCATOR_ID(TrackLLinks)
};

inline RemoveLLinks::~RemoveLLinks()
{
    if (_track)
    {
        _track->_remove = nullptr;
    }
    _track = nullptr;
}

// Nil reference via Meyers Singleton to avoid SIOF.
inline Ref<TrackLLinks>& GetLLinkNil()
{
    [[clang::no_destroy]] static Ref<TrackLLinks> instance = new TrackLLinks(nullptr);
    return instance;
}
// LLinkNil spells the accessor as a value.
#define LLinkNil GetLLinkNil()

template <class Type>
class LLink
{
    mutable Ref<TrackLLinks> _tracker;

  public:
    LLink() : _tracker(LLinkNil) {}
    LLink(Type* ref) : _tracker(ref ? (ref->Track() ? ref->Track() : new TrackLLinks(ref)) : (TrackLLinks*)LLinkNil) {}

    // An empty link's _tracker is the LLinkNil sentinel (non-null, GetObject()==null).
    // But a slot left zeroed by container growth/relocation has _tracker==null, and
    // dereferencing it reads [null+offset] — the AIUnit::CheckGetOut crash in
    // 33Escape (a null OLink survives WhoIsGettingOut().Compact()).  Treat a null
    // tracker as an empty link (no object); Compact() then drops it.
    __forceinline Type* GetLink() const
    {
        return _tracker.NotNull() ? static_cast<Type*>(_tracker->GetObject()) : nullptr;
    }
    __forceinline operator Type*() const { return GetLink(); }
    __forceinline Type* operator->() const { return GetLink(); }

};

template <class Type>
class LLinkArray : public FindArray<LLink<Type>>
{
  public:
    int Count() const;
    void Compact();
};

template <class Type>
int LLinkArray<Type>::Count() const
{
    int i, n = 0;
    for (i = 0; i < this->Size(); i++)
    {
        if (this->Get(i) != nullptr)
        {
            n++;
        }
    }
    return n;
}

template <class Type>
void LLinkArray<Type>::Compact()
{
    int d = 0;
    for (int s = 0; s < this->Size(); s++)
    {
        if (this->Get(s) != nullptr)
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


using ::Poseidon::Foundation::LLink;
using ::Poseidon::Foundation::RemoveLLinks;
using ::Poseidon::Foundation::LLinkArray;
