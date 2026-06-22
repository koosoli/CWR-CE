#include "ServerModResolve.hpp"

namespace Poseidon
{

void ModCatalog::Add(ModCatalogEntry entry)
{
    const ModId id = entry.Id();
    if (id.Empty() || _index.count(id) != 0)
    {
        return; // skip blank or duplicate ids (first wins)
    }
    _index.emplace(id, _entries.size());
    _entries.push_back(std::move(entry));
}

const ModCatalogEntry* ModCatalog::Find(const ModId& id) const
{
    auto it = _index.find(id);
    return it != _index.end() ? &_entries[it->second] : nullptr;
}

ServerModList::ServerModList(const std::string& modString, bool equalModRequired) : _exact(equalModRequired)
{
    std::size_t start = 0;
    while (start <= modString.size())
    {
        std::size_t sep = modString.find(';', start);
        if (sep == std::string::npos)
        {
            sep = modString.size();
        }
        ModId id(modString.substr(start, sep - start));
        if (!id.Empty() && _set.insert(id).second)
        {
            _required.push_back(std::move(id));
        }
        start = sep + 1;
    }
}

ServerModResolver::ServerModResolver(const std::vector<ModId>& installed, const std::vector<ModId>& active)
    : _active(active)
{
    for (const ModId& id : installed)
    {
        if (!id.Empty())
        {
            _installed.insert(id);
        }
    }
}

ServerModResolution ServerModResolver::Resolve(const ServerModList& server, const ModCatalog& catalog) const
{
    ServerModResolution out;

    for (const ModId& id : server.Required())
    {
        if (_installed.count(id) != 0)
        {
            out._satisfied.push_back(id);
            continue;
        }
        if (const ModCatalogEntry* e = catalog.Find(id))
        {
            out._toDownload.push_back(
                {id, e->Name().empty() ? id.Value() : e->Name(), e->DownloadUrl(), e->FolderName(), e->SizeBytes()});
        }
        else
        {
            out._blocked.push_back(id);
        }
    }

    // Only an exact-match server forces extras off; otherwise the player keeps them.
    if (server.RequiresExactMatch())
    {
        std::unordered_set<ModId, ModId::Hash> seen;
        for (const ModId& id : _active)
        {
            if (id.Empty() || server.Requires(id))
            {
                continue;
            }
            if (seen.insert(id).second)
            {
                out._toDisable.push_back(id);
            }
        }
    }

    return out;
}

} // namespace Poseidon
