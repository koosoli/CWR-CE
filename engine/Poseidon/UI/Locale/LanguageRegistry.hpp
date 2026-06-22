#pragma once

#include <Poseidon/UI/Locale/Stringtable/CodepageTranscode.hpp>
#include <string>
#include <vector>

namespace Poseidon
{
class ParamEntry;
}

namespace CfgLib
{

// Everything the engine needs to know about one game language. Seeded with built-in defaults and
// optionally overridden per-game by a CfgLanguages config class (see LanguageRegistry::LoadFromConfig).
struct LanguageInfo
{
    std::string name;                                         // canonical identity name (English), e.g. "Czech"
    std::string autonym;                                      // native display name in its own script, e.g. "Čeština"
    std::string code;                                         // short code used in file suffixes, e.g. "CZ"
    Poseidon::Codepage codepage = Poseidon::Codepage::CP1252; // legacy CSV encoding for this language
    std::vector<std::string> localeAliases;                   // POSIX locale prefixes, e.g. {"cs"}
    int win32PrimaryLang = 0;                                 // Win32 PRIMARYLANGID value, 0 = none
    std::string voiceSuffix;                                  // PBO suffix for voice overlays, e.g. "cz"; empty = none
    bool hasVoice = true;                                     // whether voiceover is provided for this language
};

// Single source of truth for the supported-language set and its per-language metadata. A process-wide
// singleton seeded with the engine defaults; a game's CfgLanguages can replace/extend the set.
class LanguageRegistry
{
  public:
    static LanguageRegistry& Instance();

    const std::vector<LanguageInfo>& Languages() const { return _langs; }
    int Count() const { return static_cast<int>(_langs.size()); }

    // Canonical names as a contiguous C-string array (stable until the set is reloaded). For
    // call sites that take a `const char* const*` + count (e.g. the options language list).
    const char* const* NamePtrs() const { return _namePtrs.data(); }

    // Native display names (autonyms) as a contiguous C-string array, same order as Languages().
    // What the options language pickers show — each language reads in its own script regardless of
    // the current UI language (English, Français, …, Русский). Falls back to the canonical name.
    const char* const* DisplayNamePtrs() const { return _displayPtrs.data(); }

    // Voice-capable subset (LanguageInfo::hasVoice) — the voiceover-language picker should only offer
    // languages the game actually provides dubbing for, not the full text-language set. Indices here
    // are into THIS subset, not Languages(). Display names follow the same autonym semantics.
    const char* const* VoiceDisplayNamePtrs() const { return _voiceDisplayPtrs.data(); }
    int VoiceCount() const { return static_cast<int>(_voiceDisplayPtrs.size()); }
    // Canonical name of the voice-subset entry at `voiceIndex`, or "" if out of range.
    const char* VoiceNameAt(int voiceIndex) const;
    // Voice-subset index of a language by canonical name (case-insensitive); -1 if not voice-capable.
    int VoiceIndexOf(const std::string& name) const;

    // Case-insensitive lookup by canonical name; nullptr if unknown.
    const LanguageInfo* Find(const std::string& name) const;

    // Replace the registry from a CfgLanguages config entry. Missing per-language fields fall back to the
    // matching built-in default (so a config may list only names, or override a single field).
    void LoadFromConfig(const Poseidon::ParamEntry& cfgLanguages);

    // Restore the built-in default set.
    void ResetToDefaults();

  private:
    LanguageRegistry() { ResetToDefaults(); }
    void RebuildNamePtrs();
    std::vector<LanguageInfo> _langs;
    std::vector<const char*> _namePtrs;         // points into _langs[i].name; rebuilt on every set change
    std::vector<const char*> _displayPtrs;      // points into _langs[i].autonym (or .name); rebuilt with _namePtrs
    std::vector<const char*> _voiceDisplayPtrs; // display names of the hasVoice subset, rebuilt with _namePtrs
    std::vector<int> _voiceLangIdx;             // index into _langs for each _voiceDisplayPtrs entry
};

} // namespace CfgLib
