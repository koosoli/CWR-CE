#pragma once


namespace Poseidon::Foundation
{
class GlobalAliveInterface
{
  public:
    virtual ~GlobalAliveInterface() = default;
    virtual void Alive() {}
    static void Set(GlobalAliveInterface* functor);
};

void GlobalAlive();

} // namespace Poseidon::Foundation

#define I_AM_ALIVE() ::Poseidon::Foundation::GlobalAlive()
