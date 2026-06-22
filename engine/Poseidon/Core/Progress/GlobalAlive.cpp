#include <Poseidon/Foundation/Framework/GlobalAlive.hpp>

namespace Poseidon::Foundation
{
static GlobalAliveInterface GGlobalAliveInterface;
static GlobalAliveInterface* GlobalAlivePtr = &GGlobalAliveInterface;

void GlobalAliveInterface::Set(GlobalAliveInterface* functor)
{
    GlobalAlivePtr = functor;
}

void GlobalAlive()
{
    GlobalAlivePtr->Alive();
}
} // namespace Poseidon::Foundation
