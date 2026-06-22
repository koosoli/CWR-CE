#pragma once

#include <string>

namespace CfgLib
{

// The supported-language set is owned by LanguageRegistry (config-driven per game). This is only an
// upper bound for fixed-size buffers; the live count is LanguageRegistry::Instance().Count().
inline constexpr int kMaxSupportedLanguages = 16;

int FindSupportedLanguageIndex(const std::string& language);
bool IsSupportedLanguage(const std::string& language);
std::string NormalizeSupportedLanguage(const std::string& language, const std::string& fallback = "English");
std::string DetectSystemLanguage();

} // namespace CfgLib

namespace Poseidon
{
}