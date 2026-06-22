#include <Poseidon/Foundation/PoseidonPCH.hpp>
#include <Poseidon/IO/ParamFile/LocalizedString.hpp>
#include <Poseidon/IO/ParamFile/ParamFile.hpp>
#include <Poseidon/UI/Locale/Stringtable/CodepageTranscode.hpp>
#include <Poseidon/UI/Locale/Stringtable/Stringtable.hpp>

namespace Poseidon
{

const RStringB& LocalizedString::Get() const
{
    if (_value)
    {
        const uint32_t gen = GetLanguageGeneration();
        if (gen != _gen)
        {
            // ParamEntry::operator RStringB() expands `$STR_…` against the current
            // language, so re-reading the bound node yields fresh text after a switch.
            const RStringB raw = *_value;
            _cache = RStringB(DecodeLegacyTextToRString(raw.Data(), GLanguage));
            _gen = gen;
        }
    }
    return _cache;
}

} // namespace Poseidon
