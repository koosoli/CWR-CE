#pragma once

#include <Poseidon/Asset/Addon/BankContext.hpp>
#include <Poseidon/Foundation/Strings/RString.hpp>

namespace Poseidon
{

class AddonContext : public BankContext
{
  public:
    void Reset();
    void AddAddon(RString addon);
};

} // namespace Poseidon
