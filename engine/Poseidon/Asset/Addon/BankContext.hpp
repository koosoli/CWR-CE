#pragma once

#include <Poseidon/Foundation/Strings/RString.hpp>
#include <Poseidon/Foundation/Containers/Array.hpp>
#include <Poseidon/Foundation/Containers/HashMap.hpp>
#include <Poseidon/IO/Streams/QStream.hpp>
#include <Poseidon/IO/Streams/QBStream.hpp>

namespace Poseidon
{

class BankContext : public IQFBankContext
{
    MapStringToClass<RString, AutoArray<RString>> _banks;

  public:
    bool IsAccessible(QFBank* bank) const override;
    void Clear();
    void Add(RString addon);
    void Compact();
};

} // namespace Poseidon
