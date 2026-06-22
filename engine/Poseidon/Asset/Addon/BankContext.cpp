#include <Poseidon/Asset/Addon/BankContext.hpp>
#include <Poseidon/Foundation/Containers/Array.hpp>
#include <Poseidon/Foundation/Containers/HashMap.hpp>
#include <Poseidon/Foundation/Strings/RString.hpp>

namespace Poseidon
{

bool BankContext::IsAccessible(QFBank* bank) const
{
    return !_banks.IsNull(_banks[bank->GetPrefix()]);
}

void BankContext::Clear()
{
    _banks.Clear();
}

void BankContext::Add(RString addon)
{
    _banks.Add(addon);
}

void BankContext::Compact() {}

} // namespace Poseidon
