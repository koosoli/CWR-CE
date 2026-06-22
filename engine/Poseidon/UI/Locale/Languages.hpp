#pragma once

#include <Poseidon/UI/Locale/LanguageRegistry.hpp>

// Language table

#define Chinese				0x04
#define Czech					0x05
#define Danish				0x06
#define Dutch					0x13
#define English				0x09
#define Finnish				0x0b
#define French				0x0c
#define German				0x07
#define Greek					0x08
#define Hungarian			0x0e
#define Icelandic			0x0f
#define Italian				0x10
#define Japanese			0x11
#define Norwegian			0x14
#define Polish				0x15
#define Portuguese		0x16
#define Russian				0x19
#define SerboCroatian	0x1a
#define Slovak				0x1b
#define Spanish				0x0a
#define Swedish				0x1d
#define Turkish				0x1f

// Returns PBO suffix for language-specific voice overlays (e.g. "cz" for Czech), nullptr if none

namespace Poseidon
{
inline const char* GetLanguagePboSuffix(const char* language)
{
    const CfgLib::LanguageInfo* info = CfgLib::LanguageRegistry::Instance().Find(language ? language : "");
    if (info && !info->voiceSuffix.empty())
        return info->voiceSuffix.c_str();
    return nullptr;
}

#define SpChinese							Speech4
#define SpCzech								Speech5
#define SpDanish							Speech6
#define SpDutch								Speech19
#define SpEnglish							Speech9
#define SpFinnish							Speech11
#define SpFrench							Speech12
#define SpGerman							Speech7
#define SpGreek								Speech8
#define SpHungarian						Speech14
#define SpIcelandic						Speech15
#define SpItalian							Speech16
#define SpJapanese						Speech17
#define SpNorwegian						Speech20
#define SpPolish							Speech21
#define SpPortuguese					Speech22
#define SpRussian							Speech25
#define SpSerboCroatian				Speech26
#define SpSlovak							Speech27
#define SpSpanish							Speech10
#define SpSwedish							Speech29
#define SpTurkish							Speech31

} // namespace Poseidon
