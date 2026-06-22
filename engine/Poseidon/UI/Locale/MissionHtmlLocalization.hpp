#pragma once

#include <Poseidon/Foundation/Strings/RString.hpp>

class CHTMLContainer;

namespace Poseidon
{
RString GetMissionStringtableFileForHtml(RString htmlFilename);
RString FindLocalizedMissionHtmlFile(RString directory, RString stem);
RString ExpandMissionHtmlStringtableTokens(RString htmlUtf8, RString csvPath);
RString LoadLocalizedMissionHtmlUtf8(RString filename);
RString LoadLocalizedMissionBriefingName(RString missionDirectory, RString fallback = RString());
RString FormatLocalizedBriefingTitle(RString unitName, RString groupName, int unitId);
void LoadLocalizedMissionHtml(CHTMLContainer* html, RString filename);
RString GetCurrentHtmlSectionName(const CHTMLContainer* html);

} // namespace Poseidon
