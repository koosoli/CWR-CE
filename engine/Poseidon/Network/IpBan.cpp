#include <Poseidon/Network/IpBan.hpp>
#include <Poseidon/IO/Streams/QStream.hpp>

#include <cctype>
#include <cstdio>
#include <cstring>
#include <string>

namespace Poseidon
{
bool ParseIPv4(const char* s, uint32_t& out)
{
    if (!s)
        return false;
    while (*s == ' ' || *s == '\t')
        ++s;

    uint32_t octets[4];
    for (int i = 0; i < 4; ++i)
    {
        if (!isdigit(static_cast<unsigned char>(*s)))
            return false;
        int value = 0;
        int digits = 0;
        while (isdigit(static_cast<unsigned char>(*s)))
        {
            value = value * 10 + (*s - '0');
            if (++digits > 3 || value > 255)
                return false;
            ++s;
        }
        octets[i] = static_cast<uint32_t>(value);
        if (i < 3)
        {
            if (*s != '.')
                return false;
            ++s;
        }
    }

    while (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n')
        ++s;
    if (*s != 0)
        return false;

    out = (octets[0] << 24) | (octets[1] << 16) | (octets[2] << 8) | octets[3];
    return true;
}

RString FormatIPv4(uint32_t ip)
{
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%u.%u.%u.%u", (ip >> 24) & 0xff, (ip >> 16) & 0xff, (ip >> 8) & 0xff, ip & 0xff);
    return RString(buffer);
}

void LoadIpBanList(RString path, FindArray<uint32_t>& out)
{
    out.Clear();
    QIFStream f;
    f.open(path);
    std::string line;
    auto flush = [&]()
    {
        if (!line.empty())
        {
            uint32_t ip;
            if (ParseIPv4(line.c_str(), ip))
                out.AddUnique(ip);
            line.clear();
        }
    };
    while (!f.eof() && !f.fail())
    {
        int c = f.get();
        if (f.eof() || f.fail())
            break;
        if (c == '\n' || c == '\r')
            flush();
        else
            line.push_back(static_cast<char>(c));
    }
    flush();
}

void SaveIpBanList(RString path, const FindArray<uint32_t>& list)
{
    QOFStream f(path);
    for (int i = 0; i < list.Size(); ++i)
    {
        const RString s = FormatIPv4(list[i]);
        f.write(static_cast<const char*>(s), s.GetLength());
        f.put('\r');
        f.put('\n');
    }
    f.close();
}

BanMode ParseBanMode(const char* s)
{
    if (s)
    {
        if (stricmp(s, "id") == 0)
            return BanMode::Id;
        if (stricmp(s, "ip") == 0)
            return BanMode::Ip;
    }
    return BanMode::Both;
}
} // namespace Poseidon
