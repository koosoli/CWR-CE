#pragma once
// Op-count instrumentation for container behaviour tests; see test_traits_*_ops.cpp consumers.

#include <type_traits>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Poseidon/Foundation/Containers/TypeOpts.hpp>

namespace Poseidon::Foundation::ContainerTests {

struct OpCounts
{
    int defaultCtors = 0;
    int copyCtors    = 0;
    int moveCtors    = 0;
    int dtors        = 0;
    int copyAssigns  = 0;
    int moveAssigns  = 0;
    void Reset() { *this = {}; }
};

template <class Tag>
struct Counted
{
    static OpCounts& Counts()
    {
        static OpCounts s;
        return s;
    }

    int payload = 0;

    Counted() noexcept                                      { ++Counts().defaultCtors; }
    explicit Counted(int p) noexcept : payload(p)           { ++Counts().defaultCtors; }
    Counted(const Counted& o) noexcept : payload(o.payload) { ++Counts().copyCtors; }
    Counted(Counted&& o) noexcept : payload(o.payload)      { ++Counts().moveCtors; }
    ~Counted() noexcept                                     { ++Counts().dtors; }
    Counted& operator=(const Counted& o) noexcept
    {
        payload = o.payload;
        ++Counts().copyAssigns;
        return *this;
    }
    Counted& operator=(Counted&& o) noexcept
    {
        payload = o.payload;
        ++Counts().moveAssigns;
        return *this;
    }
    bool operator==(const Counted& o) const noexcept { return payload == o.payload; }

};

struct TrivialPod
{
    int a;
    int b;
    bool operator==(const TrivialPod& o) const noexcept { return a == o.a && b == o.b; }
};
static_assert(std::is_trivially_copyable_v<TrivialPod>);

} // namespace Poseidon::Foundation::ContainerTests
