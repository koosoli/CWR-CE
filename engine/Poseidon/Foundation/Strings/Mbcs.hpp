#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include <Poseidon/UI/Locale/Languages.hpp>
#include <Poseidon/Foundation/Strings/RString.hpp>


namespace Poseidon::Foundation
{
int FindLangID(const char *language);
int GetLangID();
void SetLangID(const char *language);

int NextTextElementPos(const char* text, int pos, int langID);
int PrevTextElementPos(const char* text, int pos, int langID);
int CountTextElements(const char* text, int langID);
int ByteOffsetForTextElements(const char* text, int count, int langID);
int CopyTextElement(const char* text, int pos, int langID, char* out, size_t outSize);
int Utf8CodepointBytes(const char* text);
int DecodeUtf8Codepoint(const char* text, uint32_t* outCodepoint);
int CountUtf8Codepoints(const char* text);
const char* Utf8AdvanceCodepoints(const char* text, int count);
int EncodeUtf8Codepoint(uint32_t codepoint, char* out, size_t outSize);

struct FontID
{
	RString name;
	int langID;

	FontID(RString n, int lID) {name = n; langID = lID;}
};

} // namespace Poseidon::Foundation
using ::Poseidon::Foundation::FontID;
using ::Poseidon::Foundation::DecodeUtf8Codepoint;
using ::Poseidon::Foundation::Utf8CodepointBytes;
using ::Poseidon::Foundation::CountUtf8Codepoints;
using ::Poseidon::Foundation::Utf8AdvanceCodepoints;
using ::Poseidon::Foundation::EncodeUtf8Codepoint;
using ::Poseidon::Foundation::CopyTextElement;
using ::Poseidon::Foundation::ByteOffsetForTextElements;
using ::Poseidon::Foundation::NextTextElementPos;
using ::Poseidon::Foundation::PrevTextElementPos;
using ::Poseidon::Foundation::CountTextElements;
