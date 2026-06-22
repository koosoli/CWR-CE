#include <Poseidon/Asset/Addon/AddonContext.hpp>
#include <Poseidon/Asset/Addon/AddonSystem.hpp>

#include <Poseidon/IO/ParamFile/ParamFile.hpp>
#include <Poseidon/Foundation/Strings/RString.hpp>

extern ParamClass Pars;

namespace Poseidon
{

void AddonContext::Reset()
{
    Clear();

    const ParamEntry& def = Pars >> "CfgAddons" >> "PreloadBanks";
    for (int c = 0; c < def.GetEntryCount(); c++)
    {
        const ParamEntry& cc = def.GetEntry(c);
        if (!cc.IsClass())
            continue;
        if (!cc.FindEntry("list"))
            continue;
        const ParamEntry& cl = cc >> "list";
        for (int i = 0; i < cl.GetSize(); i++)
        {
            RString bank = cl[i];
            Add(bank);
        }
    }
}

void AddonContext::AddAddon(RString addon)
{
    const AddonInfo* info = AddonSystem::FindAddonInfo(addon);
    if (!info)
        return;
    Add(info->GetPrefix());
}

} // namespace Poseidon
