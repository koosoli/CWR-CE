#include "ModId.hpp"

#include <cctype>

namespace Poseidon
{

ModId::ModId(const std::string& token)
{
    // Trim surrounding whitespace.
    std::size_t b = 0;
    std::size_t e = token.size();
    while (b < e && std::isspace(static_cast<unsigned char>(token[b])))
    {
        ++b;
    }
    while (e > b && std::isspace(static_cast<unsigned char>(token[e - 1])))
    {
        --e;
    }

    // Basename: drop everything up to the last path separator ('/' or '\').
    std::size_t base = b;
    for (std::size_t i = b; i < e; ++i)
    {
        if (token[i] == '/' || token[i] == '\\')
        {
            base = i + 1;
        }
    }

    // Strip a single leading '@'.
    if (base < e && token[base] == '@')
    {
        ++base;
    }

    _id.reserve(e - base);
    for (std::size_t i = base; i < e; ++i)
    {
        _id.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(token[i]))));
    }
}

} // namespace Poseidon
