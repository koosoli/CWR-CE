#include <Poseidon/UI/Locale/LanguageRegistry.hpp>
#include <Poseidon/IO/ParamFile/ParamFile.hpp>

#include <cstring>
#ifndef _WIN32
#include <strings.h>
#endif
#include <utility>
#include <Poseidon/Foundation/platform.hpp>

namespace CfgLib
{

// Built-in default language set. PRIMARYLANGID values match the Win32 LANG_* constants used by
// DetectSystemLanguage; localeAliases are POSIX $LANG prefixes; codepage is the legacy CSV encoding.
static std::vector<LanguageInfo> BuiltinDefaults()
{
    // autonym = the language's name in its own script/language (UTF-8), shown in the options pickers.
    // Last field is hasVoice: the Remaster ships dubbing for English (the base voice) and Czech only,
    // so the voiceover-language picker offers just those two. A per-game CfgLanguages can flip the flag
    // for a language whose voiceover it provides.
    return {
        {"English", "English", "EN", Poseidon::Codepage::CP1252, {"en"}, 0x09, "", true},
        {"French", "Français", "FR", Poseidon::Codepage::CP1252, {"fr"}, 0x0c, "", false},
        {"Italian", "Italiano", "IT", Poseidon::Codepage::CP1252, {"it"}, 0x10, "", false},
        {"Spanish", "Español", "ES", Poseidon::Codepage::CP1252, {"es"}, 0x0a, "", false},
        {"German", "Deutsch", "DE", Poseidon::Codepage::CP1252, {"de"}, 0x07, "", false},
        {"Czech", "Čeština", "CZ", Poseidon::Codepage::CP1250, {"cs"}, 0x05, "cz", true},
        {"Polish", "Polski", "PL", Poseidon::Codepage::CP1250, {"pl"}, 0x15, "", false},
        {"Russian", "Русский", "RU", Poseidon::Codepage::CP1251, {"ru"}, 0x19, "", false},
    };
}

LanguageRegistry& LanguageRegistry::Instance()
{
    static LanguageRegistry instance;
    return instance;
}

void LanguageRegistry::RebuildNamePtrs()
{
    _namePtrs.clear();
    _displayPtrs.clear();
    _voiceDisplayPtrs.clear();
    _voiceLangIdx.clear();
    _namePtrs.reserve(_langs.size());
    _displayPtrs.reserve(_langs.size());
    for (int i = 0; i < static_cast<int>(_langs.size()); ++i)
    {
        const LanguageInfo& l = _langs[i];
        const char* display = l.autonym.empty() ? l.name.c_str() : l.autonym.c_str();
        _namePtrs.push_back(l.name.c_str());
        _displayPtrs.push_back(display);
        if (l.hasVoice)
        {
            _voiceDisplayPtrs.push_back(display);
            _voiceLangIdx.push_back(i);
        }
    }
}

const char* LanguageRegistry::VoiceNameAt(int voiceIndex) const
{
    if (voiceIndex < 0 || voiceIndex >= static_cast<int>(_voiceLangIdx.size()))
        return "";
    return _langs[_voiceLangIdx[voiceIndex]].name.c_str();
}

int LanguageRegistry::VoiceIndexOf(const std::string& name) const
{
    for (int v = 0; v < static_cast<int>(_voiceLangIdx.size()); ++v)
    {
        const std::string& cand = _langs[_voiceLangIdx[v]].name;
#ifdef _WIN32
        if (_stricmp(name.c_str(), cand.c_str()) == 0)
#else
        if (strcasecmp(name.c_str(), cand.c_str()) == 0)
#endif
            return v;
    }
    return -1;
}

void LanguageRegistry::ResetToDefaults()
{
    _langs = BuiltinDefaults();
    RebuildNamePtrs();
}

const LanguageInfo* LanguageRegistry::Find(const std::string& name) const
{
    for (const auto& l : _langs)
    {
#ifdef _WIN32
        if (_stricmp(name.c_str(), l.name.c_str()) == 0)
#else
        if (strcasecmp(name.c_str(), l.name.c_str()) == 0)
#endif
            return &l;
    }
    return nullptr;
}

static Poseidon::Codepage ParseCodepage(const char* s, Poseidon::Codepage fallback)
{
    if (!s || !*s)
        return fallback;
    if (_stricmp(s, "CP1250") == 0)
        return Poseidon::Codepage::CP1250;
    if (_stricmp(s, "CP1251") == 0)
        return Poseidon::Codepage::CP1251;
    if (_stricmp(s, "CP1252") == 0)
        return Poseidon::Codepage::CP1252;
    if (_stricmp(s, "UTF8") == 0 || _stricmp(s, "UTF-8") == 0)
        return Poseidon::Codepage::Utf8;
    return fallback;
}

void LanguageRegistry::LoadFromConfig(const Poseidon::ParamEntry& cfgLanguages)
{
    const std::vector<LanguageInfo> defaults = BuiltinDefaults();
    auto findDefault = [&](const std::string& name) -> const LanguageInfo*
    {
        for (const auto& d : defaults)
            if (_stricmp(name.c_str(), d.name.c_str()) == 0)
                return &d;
        return nullptr;
    };

    // The ordered supported set comes from the "languages[]" array.
    const ParamEntry* list = cfgLanguages.FindEntry("languages");
    if (!list || !list->IsArray() || list->GetSize() == 0)
        return; // nothing usable — keep current set

    std::vector<LanguageInfo> result;
    for (int i = 0; i < list->GetSize(); i++)
    {
        const std::string name = (const char*)(*list)[i].GetValue();
        if (name.empty())
            continue;

        // Seed from the built-in default for this name (if any), then apply config overrides.
        LanguageInfo info;
        if (const LanguageInfo* def = findDefault(name))
            info = *def;
        else
            info.name = name;
        info.name = name;

        const ParamEntry* cls = cfgLanguages.FindEntry(name.c_str());
        if (cls && cls->IsClass())
        {
            if (const ParamEntry* e = cls->FindEntry("autonym"))
                info.autonym = (const char*)e->GetValue();
            if (const ParamEntry* e = cls->FindEntry("code"))
                info.code = (const char*)e->GetValue();
            if (const ParamEntry* e = cls->FindEntry("codepage"))
                info.codepage = ParseCodepage((const char*)e->GetValue(), info.codepage);
            if (const ParamEntry* e = cls->FindEntry("voiceSuffix"))
                info.voiceSuffix = (const char*)e->GetValue();
            if (const ParamEntry* e = cls->FindEntry("voice"))
                info.hasVoice = (int)*e != 0;
            if (const ParamEntry* e = cls->FindEntry("win32"))
                info.win32PrimaryLang = (int)*e;
            if (const ParamEntry* e = cls->FindEntry("localeAliases"); e && e->IsArray())
            {
                info.localeAliases.clear();
                for (int j = 0; j < e->GetSize(); j++)
                    info.localeAliases.emplace_back((const char*)(*e)[j].GetValue());
            }
        }
        result.push_back(std::move(info));
    }

    if (!result.empty())
    {
        _langs = std::move(result);
        RebuildNamePtrs();
    }
}

} // namespace CfgLib
