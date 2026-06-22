#include <Poseidon/Foundation/Strings/Mbcs.hpp>
#include <Poseidon/Foundation/Common/Win.h>
#include <algorithm>
#include <cstring>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Poseidon/Foundation/platform.hpp>


namespace Poseidon::Foundation
{
static int LangID = English;

int FindLangID(const char* language)
{
    if (stricmp(language, "Chinese") == 0)
    {
        return Chinese;
    }
    if (stricmp(language, "Czech") == 0)
    {
        return Czech;
    }
    if (stricmp(language, "Danish") == 0)
    {
        return Danish;
    }
    if (stricmp(language, "Dutch") == 0)
    {
        return Dutch;
    }
    if (stricmp(language, "English") == 0)
    {
        return English;
    }
    if (stricmp(language, "Finnish") == 0)
    {
        return Finnish;
    }
    if (stricmp(language, "French") == 0)
    {
        return French;
    }
    if (stricmp(language, "German") == 0)
    {
        return German;
    }
    if (stricmp(language, "Greek") == 0)
    {
        return Greek;
    }
    if (stricmp(language, "Hungarian") == 0)
    {
        return Hungarian;
    }
    if (stricmp(language, "Icelandic") == 0)
    {
        return Icelandic;
    }
    if (stricmp(language, "Italian") == 0)
    {
        return Italian;
    }
    if (stricmp(language, "Japanese") == 0)
    {
        return Japanese;
    }
    if (stricmp(language, "Norwegian") == 0)
    {
        return Norwegian;
    }
    if (stricmp(language, "Polish") == 0)
    {
        return Polish;
    }
    if (stricmp(language, "Portuguese") == 0)
    {
        return Portuguese;
    }
    if (stricmp(language, "Russian") == 0)
    {
        return Russian;
    }
    if (stricmp(language, "SerboCroatian") == 0)
    {
        return SerboCroatian;
    }
    if (stricmp(language, "Slovak") == 0)
    {
        return Slovak;
    }
    if (stricmp(language, "Spanish") == 0)
    {
        return Spanish;
    }
    if (stricmp(language, "Swedish") == 0)
    {
        return Swedish;
    }
    if (stricmp(language, "Turkish") == 0)
    {
        return Turkish;
    }
    Fail("Unsupported language");
    return English;
}

int GetLangID()
{
    return LangID;
}

void SetLangID(const char* language)
{
    LangID = FindLangID(language);
}

namespace
{

bool IsUtf8ContinuationByte(unsigned char c)
{
    return (c & 0xC0) == 0x80;
}

int Utf8SequenceLength(unsigned char lead)
{
    if (lead < 0x80)
        return 1;
    if ((lead & 0xE0) == 0xC0)
        return 2;
    if ((lead & 0xF0) == 0xE0)
        return 3;
    if ((lead & 0xF8) == 0xF0)
        return 4;
    return 1;
}

} // namespace

int Utf8CodepointBytes(const char* text)
{
    if (!text || !*text)
        return 0;

    int step = Utf8SequenceLength(static_cast<unsigned char>(text[0]));
    for (int i = 1; i < step; ++i)
    {
        unsigned char c = static_cast<unsigned char>(text[i]);
        if (c == 0 || !IsUtf8ContinuationByte(c))
            return i;
    }
    return step;
}

int DecodeUtf8Codepoint(const char* text, uint32_t* outCodepoint)
{
    if (outCodepoint)
        *outCodepoint = 0xFFFD;
    if (!text || !*text)
        return 0;

    const unsigned char lead = static_cast<unsigned char>(text[0]);
    if (lead < 0x80)
    {
        if (outCodepoint)
            *outCodepoint = lead;
        return 1;
    }

    const int required = Utf8SequenceLength(lead);
    const int bytes = Utf8CodepointBytes(text);
    if (bytes <= 0)
        return 0;
    if (required == 1 || bytes < required)
        return bytes;

    switch (required)
    {
        case 2:
            if (outCodepoint)
                *outCodepoint = ((lead & 0x1F) << 6) | (static_cast<unsigned char>(text[1]) & 0x3F);
            return 2;
        case 3:
            if (outCodepoint)
            {
                *outCodepoint = ((lead & 0x0F) << 12) | ((static_cast<unsigned char>(text[1]) & 0x3F) << 6) |
                                (static_cast<unsigned char>(text[2]) & 0x3F);
            }
            return 3;
        case 4:
            if (outCodepoint)
            {
                *outCodepoint = ((lead & 0x07) << 18) | ((static_cast<unsigned char>(text[1]) & 0x3F) << 12) |
                                ((static_cast<unsigned char>(text[2]) & 0x3F) << 6) |
                                (static_cast<unsigned char>(text[3]) & 0x3F);
            }
            return 4;
        default:
            return 1;
    }
}

int CountUtf8Codepoints(const char* text)
{
    if (!text)
        return 0;

    int count = 0;
    for (const char* p = text; *p; p += Utf8CodepointBytes(p))
        ++count;
    return count;
}

const char* Utf8AdvanceCodepoints(const char* text, int count)
{
    if (!text)
        return nullptr;

    const char* p = text;
    while (count > 0 && *p)
    {
        int step = Utf8CodepointBytes(p);
        if (step <= 0)
            break;
        p += step;
        --count;
    }
    return p;
}

int NextTextElementPos(const char* text, int pos, int langID)
{
    if (!text)
        return pos;

    int len = static_cast<int>(strlen(text));
    if (pos < 0)
        pos = 0;
    if (pos >= len)
        return len;

    int step = Utf8CodepointBytes(text + pos);
    step = std::min(step, len - pos);
    return pos + step;
}

int PrevTextElementPos(const char* text, int pos, int langID)
{
    if (!text)
        return pos;
    if (pos <= 0)
        return 0;

    int i = pos - 1;
    while (i > 0 && IsUtf8ContinuationByte(static_cast<unsigned char>(text[i])))
        --i;
    return i;
}

int CountTextElements(const char* text, int langID)
{
    if (!text)
        return 0;

    int count = 0;
    for (int pos = 0, len = static_cast<int>(strlen(text)); pos < len; pos = NextTextElementPos(text, pos, langID))
        ++count;
    return count;
}

int ByteOffsetForTextElements(const char* text, int count, int langID)
{
    if (!text || count <= 0)
        return 0;

    int pos = 0;
    for (int i = 0, len = static_cast<int>(strlen(text)); i < count && pos < len; ++i)
        pos = NextTextElementPos(text, pos, langID);
    return pos;
}

int CopyTextElement(const char* text, int pos, int langID, char* out, size_t outSize)
{
    if (!out || outSize == 0)
        return pos;
    out[0] = 0;

    if (!text)
        return pos;

    int next = NextTextElementPos(text, pos, langID);
    int span = std::max(0, next - pos);
    size_t copyLen = std::min(static_cast<size_t>(span), outSize - 1);
    if (copyLen > 0)
        memcpy(out, text + pos, copyLen);
    out[copyLen] = 0;
    return next;
}

int EncodeUtf8Codepoint(uint32_t codepoint, char* out, size_t outSize)
{
    if (!out || outSize == 0)
        return 0;

    if (codepoint <= 0x7F)
    {
        if (outSize < 2)
            return 0;
        out[0] = static_cast<char>(codepoint);
        out[1] = 0;
        return 1;
    }
    if (codepoint <= 0x7FF)
    {
        if (outSize < 3)
            return 0;
        out[0] = static_cast<char>(0xC0 | (codepoint >> 6));
        out[1] = static_cast<char>(0x80 | (codepoint & 0x3F));
        out[2] = 0;
        return 2;
    }
    if (codepoint <= 0xFFFF)
    {
        if (outSize < 4)
            return 0;
        out[0] = static_cast<char>(0xE0 | (codepoint >> 12));
        out[1] = static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
        out[2] = static_cast<char>(0x80 | (codepoint & 0x3F));
        out[3] = 0;
        return 3;
    }
    if (codepoint <= 0x10FFFF)
    {
        if (outSize < 5)
            return 0;
        out[0] = static_cast<char>(0xF0 | (codepoint >> 18));
        out[1] = static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));
        out[2] = static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
        out[3] = static_cast<char>(0x80 | (codepoint & 0x3F));
        out[4] = 0;
        return 4;
    }

    return EncodeUtf8Codepoint(0xFFFD, out, outSize);
}

} // namespace Poseidon::Foundation
