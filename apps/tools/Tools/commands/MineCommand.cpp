#include "MineCommand.hpp"

#include <Poseidon/IO/ParamFile/ParamFile.hpp>
#include <Poseidon/IO/Streams/QBStream.hpp>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <CLI/App.hpp>
#include <CLI/Option.hpp>
#include <CLI/Validators.hpp>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <format>
#include <functional>
#include <utility>
#include <Poseidon/Foundation/Strings/RString.hpp>
#include <Poseidon/Foundation/Types/Pointers.hpp>

namespace fs = std::filesystem;

namespace PoseidonTools
{

namespace
{

struct SearchItem
{
    std::string displayPath;
    std::string filePath;
    std::string pboPath;
    std::string entryName;
    std::string extension;
};

struct SearchContent
{
    std::string text;
    bool debinarized = false;
};

std::string ToLowerAscii(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

bool EndsWithInsensitive(const std::string& value, const char* suffix)
{
    const size_t suffixLength = std::strlen(suffix);
    if (value.size() < suffixLength)
        return false;
    return ToLowerAscii(value.substr(value.size() - suffixLength)) == suffix;
}

std::string StripPboExtension(const std::string& path)
{
    if (EndsWithInsensitive(path, ".pbo"))
        return path.substr(0, path.size() - 4);
    return path;
}

bool IsConfigLikeExtension(const std::string& ext)
{
    return ext == ".bin" || ext == ".cpp" || ext == ".ext" || ext == ".sqm" || ext == ".hpp" || ext == ".rvmat" ||
           ext == ".bisurf" || ext == ".bimpas" || ext == ".unit" || ext == ".cfg" || ext == ".desc";
}

bool IsRawTextExtension(const std::string& ext)
{
    return ext == ".txt" || ext == ".csv" || ext == ".html" || ext == ".htm" || ext == ".log" || ext == ".md" ||
           ext == ".json" || ext == ".yaml" || ext == ".yml" || ext == ".xml" || ext == ".ini" || ext == ".toml" ||
           ext == ".sqf" || ext == ".sqs" || ext == ".inc" || ext == ".hpp" || ext == ".h" || ext == ".hh" ||
           ext == ".c" || ext == ".cc" || ext == ".cpp" || ext == ".cxx" || ext == ".ext" || ext == ".sqm" ||
           ext == ".cfg" || ext == ".desc" || ext == ".rvmat";
}

bool IsSearchableExtension(const std::string& ext)
{
    return IsConfigLikeExtension(ext) || IsRawTextExtension(ext);
}

bool ContainsQuery(const std::string& haystack, const std::string& needle, bool ignoreCase)
{
    if (needle.empty())
        return true;
    if (!ignoreCase)
        return haystack.find(needle) != std::string::npos;

    auto pred = [](char a, char b)
    { return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b)); };
    return std::search(haystack.begin(), haystack.end(), needle.begin(), needle.end(), pred) != haystack.end();
}

bool ReadLooseFile(const std::string& path, std::string& data)
{
    std::ifstream in(path, std::ios::binary);
    if (!in)
        return false;

    in.seekg(0, std::ios::end);
    std::streamoff length = in.tellg();
    if (length < 0)
        return false;

    in.seekg(0, std::ios::beg);
    data.resize(static_cast<size_t>(length));
    if (!data.empty())
        in.read(data.data(), length);
    return in.good() || in.eof();
}

bool ReadPboEntry(const std::string& pboPath, const std::string& entryName, std::string& data)
{
    QFBank bank;
    if (!bank.open(RString(StripPboExtension(pboPath).c_str())))
        return false;

    bank.Lock();
    if (bank.error())
    {
        bank.Unlock();
        return false;
    }

    Ref<IFileBuffer> buffer = bank.Read(entryName.c_str());
    bank.Unlock();
    if (!buffer)
        return false;

    data.assign(buffer->GetData(), buffer->GetSize());
    return true;
}

bool DebinarizeLooseConfig(const std::string& path, std::string& text)
{
    ParamFile cfg;
    if (!cfg.ParseBin(path.c_str()))
        return false;

    QOStream out;
    cfg.Save(out, 0);
    text.assign(out.str(), out.tellp());
    return true;
}

bool DebinarizePboConfig(const std::string& pboPath, const std::string& entryName, std::string& text)
{
    QFBank bank;
    if (!bank.open(RString(StripPboExtension(pboPath).c_str())))
        return false;

    bank.Lock();
    if (bank.error())
    {
        bank.Unlock();
        return false;
    }

    ParamFile cfg;
    bool ok = cfg.ParseBin(bank, entryName.c_str());
    bank.Unlock();
    if (!ok)
        return false;

    QOStream out;
    cfg.Save(out, 0);
    text.assign(out.str(), out.tellp());
    return true;
}

bool LoadSearchContent(const SearchItem& item, SearchContent& content)
{
    if (IsConfigLikeExtension(item.extension))
    {
        bool debinarized = item.pboPath.empty() ? DebinarizeLooseConfig(item.filePath, content.text)
                                                : DebinarizePboConfig(item.pboPath, item.entryName, content.text);
        if (debinarized)
        {
            content.debinarized = true;
            return true;
        }
    }

    if (!IsRawTextExtension(item.extension) && !IsConfigLikeExtension(item.extension))
        return false;

    return item.pboPath.empty() ? ReadLooseFile(item.filePath, content.text)
                                : ReadPboEntry(item.pboPath, item.entryName, content.text);
}

void CollectPboItems(const std::string& pboPath, std::vector<SearchItem>& items)
{
    QFBank bank;
    if (!bank.open(RString(StripPboExtension(pboPath).c_str())))
        return;

    bank.Lock();
    if (bank.error())
    {
        bank.Unlock();
        return;
    }

    struct CollectorContext
    {
        const std::string* pboPath;
        std::vector<SearchItem>* items;
    } ctx{&pboPath, &items};

    bank.ForEach(
        [](const FileInfoO& fi, const FileBankType*, void* rawCtx)
        {
            auto& ctx = *static_cast<CollectorContext*>(rawCtx);
            std::string entryName = static_cast<const char*>(fi.name);
            auto dot = entryName.find_last_of('.');
            std::string ext = dot == std::string::npos ? std::string() : ToLowerAscii(entryName.substr(dot));
            if (!IsSearchableExtension(ext))
                return;

            SearchItem item;
            item.displayPath = *ctx.pboPath + "#" + entryName;
            item.filePath = *ctx.pboPath;
            item.pboPath = *ctx.pboPath;
            item.entryName = entryName;
            item.extension = ext;
            ctx.items->push_back(std::move(item));
        },
        &ctx);

    bank.Unlock();
}

std::vector<SearchItem> CollectItems(const std::string& directory)
{
    std::vector<SearchItem> items;

    for (auto& entry : fs::recursive_directory_iterator(directory, fs::directory_options::skip_permission_denied))
    {
        try
        {
            if (!entry.is_regular_file())
                continue;

            std::string path = entry.path().string();
            std::string ext = ToLowerAscii(entry.path().extension().string());

            if (ext == ".pbo")
            {
                CollectPboItems(path, items);
                continue;
            }

            if (!IsSearchableExtension(ext))
                continue;

            SearchItem item;
            item.displayPath = path;
            item.filePath = path;
            item.extension = ext;
            items.push_back(std::move(item));
        }
        catch (const std::exception&)
        {
        }
    }

    return items;
}

int RunMine(const std::string& directory, const std::string& query, bool ignoreCase)
{
    if (query.empty())
    {
        std::cerr << "Error: empty query" << std::endl;
        return 1;
    }

    const std::vector<SearchItem> items = CollectItems(directory);
    int matchCount = 0;

    for (const SearchItem& item : items)
    {
        SearchContent content;
        if (!LoadSearchContent(item, content))
            continue;

        if (!ContainsQuery(content.text, query, ignoreCase))
            continue;

        std::cout << item.displayPath;
        if (content.debinarized)
            std::cout << " [debinarized]";
        std::cout << std::endl;
        ++matchCount;
    }

    return matchCount > 0 ? 0 : 1;
}

} // namespace

void MineCommand::Setup(CLI::App& app)
{
    auto* cmd = app.add_subcommand(
        "mine", "Recursively search text files, PBO contents, and debinarized BIS config-like files");

    static std::string directory;
    static std::string query;
    static bool ignoreCase = false;

    cmd->add_option("directory", directory, "Directory to search")->required()->check(CLI::ExistingDirectory);
    cmd->add_option("query", query, "Substring to search for")->required();
    cmd->add_flag("-i,--ignore-case", ignoreCase, "Case-insensitive search");
    cmd->callback([&]() { std::exit(RunMine(directory, query, ignoreCase)); });
}

} // namespace PoseidonTools
