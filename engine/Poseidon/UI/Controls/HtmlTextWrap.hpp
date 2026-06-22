#pragma once

#include <algorithm>
#include <Poseidon/Foundation/Strings/RString.hpp>
#include <Poseidon/Foundation/Strings/Mbcs.hpp>

#include <vector>

#define HTML_WRAP_ISSPACE(c) ((c) >= 0 && (c) <= 32)


namespace Poseidon
{
namespace HtmlTextWrap
{

inline int Utf8CharBytes(const char* text, int remaining)
{
    if (!text || remaining <= 0)
        return 0;
    return std::min(Poseidon::Foundation::Utf8CodepointBytes(text), remaining);
}

inline bool IsWrapWhitespace(const char* text, int byteCount)
{
    return byteCount == 1 && HTML_WRAP_ISSPACE(static_cast<unsigned char>(text[0]));
}

template <class MeasureFn>
inline std::vector<int> ComputeWrapBreaks(RString text, float lineWidth, MeasureFn&& measure)
{
    std::vector<int> breaks;
    float curW = 0;
    float wordW = 0;
    int wordI = -1;
    const int n = text.GetLength();

    for (int i = 0; i < n;)
    {
        const int j = i;
        const int charBytes = Utf8CharBytes(text + i, n - i);
        const RString ch = text.Substring(i, i + charBytes);
        if (IsWrapWhitespace(text + i, charBytes))
        {
            wordI = i + charBytes;
            wordW = curW;
        }

        const float cW = measure(ch);
        if (curW + cW > lineWidth)
        {
            if (wordW > 0)
            {
                breaks.push_back(wordI);
                i = wordI;
            }
            else
            {
                if (curW <= 0)
                {
                    curW += cW;
                    i += charBytes;
                    continue;
                }
                breaks.push_back(j);
                i = j;
            }
            curW = 0;
            wordW = 0;
            wordI = i;
        }
        else
        {
            curW += cW;
            i += charBytes;
        }
    }

    return breaks;
}

} // namespace HtmlTextWrap

}  // namespace Poseidon
