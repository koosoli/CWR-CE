#pragma once

#include <Poseidon/UI/Locale/SupportedLanguages.hpp>
#include <Poseidon/Foundation/Strings/RString.hpp>

#include <optional>
#include <Poseidon/IO/Streams/QBStream.hpp>
namespace Poseidon
{
class QFBank;

namespace MissionLanguageDetector
{

struct MissionLanguageFlags
{
    bool stringtable = false;
    bool briefing = false;
    bool overview = false;
    bool voice = false;

    bool Any() const { return stringtable || briefing || overview || voice; }
};

struct MissionLanguageInfo
{
    // Indexed by LanguageRegistry order; only the first LanguageRegistry::Instance().Count() entries are used.
    MissionLanguageFlags flags[CfgLib::kMaxSupportedLanguages];
};

struct MissionPreviewInfo
{
    MissionLanguageInfo languages;
    std::optional<float> missionViewDistance;
};

MissionLanguageInfo Detect(RString root);
MissionLanguageInfo Detect(const QFBank& bank);
MissionPreviewInfo DetectPreview(RString root);
MissionPreviewInfo DetectPreview(const QFBank& bank);
RString FormatTextLanguages(const MissionLanguageInfo& info);
RString FormatVoiceLanguages(const MissionLanguageInfo& info);
RString FormatViewDistance(const MissionPreviewInfo& info);
RString BuildPreviewHtml(const MissionPreviewInfo& info, RString overviewHtmlUtf8 = RString());

// The TXT / VO / VD localization-status block is a diagnostic overlay, hidden
// from players by default.  Toggle it from the dev panel (Ctrl+` -> Game tab).
bool ShowLocalizationDebugInfo();
void SetShowLocalizationDebugInfo(bool show);

} // namespace MissionLanguageDetector

}  // namespace Poseidon
