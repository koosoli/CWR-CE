#include <Poseidon/Asset/Formats/Common/CsvReader.hpp>
#include <cctype>
#include <cstdio>
#include <utility>

namespace Poseidon::Asset::Formats
{

std::string CsvReadCell(QIStream& stream)
{
    // Skip leading spaces (but not CR/LF or comma)
    int c = stream.get();
    while (c != EOF && c != '\r' && c != '\n' && c != ',' && isspace(c))
        c = stream.get();

    // Empty cell or end of line — leave delimiter in stream
    if (c == EOF || c == '\r' || c == '\n' || c == ',')
    {
        if (c != EOF)
            stream.unget();
        return {};
    }

    std::string result;

    if (c == '"')
    {
        // Quoted field: "" inside quotes is an escaped quote
        while (true)
        {
            c = stream.get();
            if (c == EOF)
                break;
            if (c == '"')
            {
                int next = stream.get();
                if (next == '"')
                {
                    result += '"';
                    continue;
                }
                // End of quoted field — leave delimiter in stream
                if (next != EOF)
                    stream.unget();
                break;
            }
            result += static_cast<char>(c);
        }
    }
    else
    {
        // Unquoted field: read until comma or EOL, leave delimiter in stream
        while (c != EOF && c != ',' && c != '\r' && c != '\n')
        {
            result += static_cast<char>(c);
            c = stream.get();
        }
        if (c != EOF)
            stream.unget();
    }

    return result;
}

bool CsvReadRow(QIStream& stream, std::vector<std::string>& row)
{
    row.clear();

    if (stream.eof())
        return false;

    int c = stream.get();
    if (c == EOF)
        return false;
    stream.unget();

    row.push_back(CsvReadCell(stream));

    while (true)
    {
        c = stream.get();
        if (c == EOF || c == '\r' || c == '\n')
            break;
        if (c == ',')
            row.push_back(CsvReadCell(stream));
    }

    // Consume CRLF pair
    if (c == '\r')
    {
        int next = stream.get();
        if (next != '\n' && next != EOF)
            stream.unget();
    }

    return true;
}

std::vector<std::vector<std::string>> CsvReadAll(QIStream& stream)
{
    std::vector<std::vector<std::string>> result;
    std::vector<std::string> row;
    while (CsvReadRow(stream, row))
        result.push_back(std::move(row));
    return result;
}

} // namespace Poseidon::Asset::Formats
