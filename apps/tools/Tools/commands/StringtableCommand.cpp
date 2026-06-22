#include "StringtableCommand.hpp"
#include "ConfigCommand.hpp"
#include <Poseidon/UI/Locale/Stringtable/CodepageTranscode.hpp>
#include <Poseidon/IO/Streams/QBStream.hpp>
#include <Poseidon/Asset/Probes/AssetInfo.hpp>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cstring>
#include <cctype>
#include <filesystem>
#include <set>
#include <unordered_map>
#include <CLI/App.hpp>
#include <CLI/Option.hpp>
#include <CLI/TypeTools.hpp>
#include <cstdlib>
#include <functional>
#include <initializer_list>
#include <system_error>
#include <utility>
#include <Poseidon/Foundation/Strings/RString.hpp>
#include <Poseidon/Foundation/Types/Pointers.hpp>

namespace PoseidonTools
{

void StringtableCommand::Setup(CLI::App& app)
{
    auto* st = app.add_subcommand("stringtable", "Stringtable CSV operations");
    st->require_subcommand(1);
    {
        auto* sub = st->add_subcommand("validate", "Check that a stringtable CSV is parseable");
        static std::string inputPath;
        sub->add_option("input", inputPath, "Input stringtable CSV file")->required();
        sub->callback([&]() { std::exit(Validate(inputPath)); });
    }
    {
        auto* sub = st->add_subcommand("inspect", "Show key/language statistics");
        static std::string inputPath;
        sub->add_option("input", inputPath, "Input stringtable CSV file")->required();
        sub->callback([&]() { std::exit(Inspect(inputPath)); });
    }
    {
        auto* sub = st->add_subcommand("lookup", "Look up a key value for a given language");
        static std::string inputPath, language, key;
        sub->add_option("input", inputPath, "Input stringtable CSV file")->required();
        sub->add_option("language", language, "Language column name")->required();
        sub->add_option("key", key, "String key to look up")->required();
        sub->callback([&]() { std::exit(Lookup(inputPath, language, key)); });
    }
    {
        auto* sub =
            st->add_subcommand("check-config", "Report config $STR references that have no matching stringtable key");
        static std::string configPath;
        static std::vector<std::string> stringtablePaths;
        static std::string prefix;
        static bool reportUnused = false;
        static bool checkLangs = false;
        sub->add_option("config", configPath,
                        "Config (.bin/.cpp), a dir containing one, an addon .pbo, or a dir of addon .pbo files")
            ->required();
        sub->add_option("stringtable", stringtablePaths,
                        "Extra stringtable CSV file(s)/dirs (addon .pbo brings its own stringtable automatically)");
        sub->add_option("-p,--prefix", prefix, "Token prefix after '$' (default: STR, matches $STR/$STRM/...)")
            ->default_val("STR");
        sub->add_flag("--unused", reportUnused,
                      "Also list stringtable keys not referenced by this config (noisy across a full table)");
        sub->add_flag("--langs", checkLangs,
                      "Also flag referenced keys that lack a value in some declared language (INCOMPLETE)");
        sub->callback([&]()
                      { std::exit(CheckConfig(configPath, stringtablePaths, prefix, reportUnused, checkLangs)); });
    }
}
std::string StringtableCommand::ReadField(const char*& p, const char* end)
{
    std::string result;
    if (p >= end)
        return result;

    if (*p == '"')
    {
        p++;
        while (p < end)
        {
            if (*p == '"')
            {
                p++;
                if (p < end && *p == '"')
                {
                    result += '"';
                    p++;
                }
                else
                {
                    break;
                }
            }
            else
            {
                result += *p++;
            }
        }
        while (p < end && *p != ',' && *p != '\r' && *p != '\n')
            p++;
    }
    else
    {
        while (p < end && *p != ',' && *p != '\r' && *p != '\n')
        {
            result += *p++;
        }
    }
    if (p < end && *p == ',')
        p++;

    return result;
}

bool StringtableCommand::ParseCSV(const std::string& path, Table& table)
{
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "Error: cannot open file: " << path << std::endl;
        return false;
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    if (content.empty())
    {
        std::cerr << "Error: empty file: " << path << std::endl;
        return false;
    }

    return ParseCSVContent(content, table);
}

bool StringtableCommand::ParseCSVContent(const std::string& content, Table& table)
{
    const char* p = content.c_str();
    const char* end = p + content.size();
    while (p < end)
    {
        const char* lineStart = p;
        std::string firstField = ReadField(p, end);
        while (!firstField.empty() && isspace((unsigned char)firstField.front()))
            firstField.erase(firstField.begin());
        while (!firstField.empty() && isspace((unsigned char)firstField.back()))
            firstField.pop_back();
        std::string upper = firstField;
        for (auto& c : upper)
            c = (char)toupper((unsigned char)c);

        if (upper == "LANGUAGE")
        {
            while (p < end && *p != '\r' && *p != '\n')
            {
                std::string lang = ReadField(p, end);
                while (!lang.empty() && isspace((unsigned char)lang.front()))
                    lang.erase(lang.begin());
                while (!lang.empty() && isspace((unsigned char)lang.back()))
                    lang.pop_back();
                if (!lang.empty())
                    table.languages.push_back(lang);
            }
            while (p < end && (*p == '\r' || *p == '\n'))
                p++;
            break;
        }
        while (p < end && *p != '\r' && *p != '\n')
        {
            ReadField(p, end);
        }
        while (p < end && (*p == '\r' || *p == '\n'))
            p++;
    }

    if (table.languages.empty())
    {
        std::cerr << "Error: no LANGUAGE header row found" << std::endl;
        return false;
    }
    while (p < end)
    {
        if (*p == '\r' || *p == '\n')
        {
            p++;
            continue;
        }

        std::string key = ReadField(p, end);
        while (!key.empty() && isspace((unsigned char)key.front()))
            key.erase(key.begin());
        while (!key.empty() && isspace((unsigned char)key.back()))
            key.pop_back();

        if (key.empty())
        {
            while (p < end && *p != '\r' && *p != '\n')
                ReadField(p, end);
            while (p < end && (*p == '\r' || *p == '\n'))
                p++;
            continue;
        }
        std::string keyUpper = key;
        for (auto& c : keyUpper)
            c = (char)toupper((unsigned char)c);
        if (keyUpper == "COMMENT" || keyUpper == "LANGUAGE")
        {
            while (p < end && *p != '\r' && *p != '\n')
                ReadField(p, end);
            while (p < end && (*p == '\r' || *p == '\n'))
                p++;
            continue;
        }

        Row row;
        row.key = key;
        for (size_t i = 0; i < table.languages.size() && p < end && *p != '\r' && *p != '\n'; i++)
        {
            row.values.push_back(ReadField(p, end));
        }
        while (row.values.size() < table.languages.size())
        {
            row.values.push_back("");
        }

        table.rows.push_back(std::move(row));
        while (p < end && *p != '\r' && *p != '\n')
            ReadField(p, end);
        while (p < end && (*p == '\r' || *p == '\n'))
            p++;
    }

    return true;
}

// Detect whether a file's raw bytes are valid UTF-8 (and contain non-ASCII).
// Used for informational reporting only — the loader itself decides encoding via filename.
static bool IsLikelyUtf8(const std::string& path)
{
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open())
    {
        return false;
    }
    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    const unsigned char* p = reinterpret_cast<const unsigned char*>(content.data());
    const unsigned char* end = p + content.size();
    bool sawNonAscii = false;
    while (p < end)
    {
        if (*p < 0x80)
        {
            p++;
        }
        else if ((*p & 0xE0) == 0xC0)
        {
            if (p + 1 >= end || (p[1] & 0xC0) != 0x80)
            {
                return false;
            }
            sawNonAscii = true;
            p += 2;
        }
        else if ((*p & 0xF0) == 0xE0)
        {
            if (p + 2 >= end || (p[1] & 0xC0) != 0x80 || (p[2] & 0xC0) != 0x80)
            {
                return false;
            }
            sawNonAscii = true;
            p += 3;
        }
        else if ((*p & 0xF8) == 0xF0)
        {
            if (p + 3 >= end || (p[1] & 0xC0) != 0x80 || (p[2] & 0xC0) != 0x80 || (p[3] & 0xC0) != 0x80)
            {
                return false;
            }
            sawNonAscii = true;
            p += 4;
        }
        else
        {
            return false;
        }
    }
    return sawNonAscii;
}

static const char* CodepageName(Poseidon::Codepage cp)
{
    switch (cp)
    {
        case Poseidon::Codepage::Utf8:
            return "UTF-8";
        case Poseidon::Codepage::CP1252:
            return "CP1252 (Western European)";
        case Poseidon::Codepage::CP1250:
            return "CP1250 (Central European)";
        case Poseidon::Codepage::CP1251:
            return "CP1251 (Cyrillic)";
    }
    return "?";
}

static bool HasUtf8CsvSuffix(const std::string& path)
{
    const char* suffix = ".utf8.csv";
    size_t slen = std::strlen(suffix);
    if (path.size() < slen)
    {
        return false;
    }
    return std::equal(
        suffix, suffix + slen, path.end() - static_cast<std::string::difference_type>(slen), [](char a, char b)
        { return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b)); });
}

int StringtableCommand::Validate(const std::string& inputPath)
{
    Table table;
    if (!ParseCSV(inputPath, table))
    {
        return 1;
    }
    std::cout << "OK: " << table.languages.size() << " languages, " << table.rows.size() << " keys" << std::endl;
    return 0;
}

int StringtableCommand::Inspect(const std::string& inputPath)
{
    Table table;
    if (!ParseCSV(inputPath, table))
    {
        return 1;
    }
    bool byNameUtf8 = HasUtf8CsvSuffix(inputPath);
    bool byContentUtf8 = IsLikelyUtf8(inputPath);
    std::cout << "File: " << inputPath << std::endl;
    std::cout << "Encoding mode: " << (byNameUtf8 ? "UTF-8 (.utf8.csv suffix)" : "legacy (per-column codepage)")
              << std::endl;
    std::cout << "Content UTF-8 valid: " << (byContentUtf8 ? "yes" : "no") << std::endl;
    if (byNameUtf8 && !byContentUtf8)
    {
        std::cout << "  WARNING: filename suggests UTF-8 but content is not valid UTF-8" << std::endl;
    }
    if (!byNameUtf8 && byContentUtf8)
    {
        std::cout << "  NOTE: content looks like UTF-8; consider renaming to .utf8.csv" << std::endl;
    }
    std::cout << std::endl;

    std::cout << "Languages: " << table.languages.size() << std::endl;
    std::cout << "Keys: " << table.rows.size() << std::endl;
    std::cout << std::endl;

    for (size_t i = 0; i < table.languages.size(); i++)
    {
        int nonEmpty = 0;
        for (const auto& row : table.rows)
        {
            if (i < row.values.size())
            {
                std::string val = row.values[i];
                while (!val.empty() && isspace((unsigned char)val.front()))
                {
                    val.erase(val.begin());
                }
                while (!val.empty() && isspace((unsigned char)val.back()))
                {
                    val.pop_back();
                }
                if (!val.empty())
                {
                    nonEmpty++;
                }
            }
        }
        Poseidon::Codepage cp =
            byNameUtf8 ? Poseidon::Codepage::Utf8 : Poseidon::CodepageForLanguage(table.languages[i].c_str());
        std::cout << "  " << table.languages[i] << ": " << nonEmpty << "/" << table.rows.size() << "  ["
                  << CodepageName(cp) << "]" << std::endl;
    }

    return 0;
}

int StringtableCommand::Lookup(const std::string& inputPath, const std::string& language, const std::string& key)
{
    Table table;
    if (!ParseCSV(inputPath, table))
        return 1;
    int langIdx = -1;
    std::string langUpper = language;
    for (auto& c : langUpper)
        c = (char)toupper((unsigned char)c);

    for (size_t i = 0; i < table.languages.size(); i++)
    {
        std::string colUpper = table.languages[i];
        for (auto& c : colUpper)
            c = (char)toupper((unsigned char)c);
        if (colUpper == langUpper)
        {
            langIdx = static_cast<int>(i);
            break;
        }
    }

    if (langIdx < 0)
    {
        std::cerr << "Error: language '" << language << "' not found" << std::endl;
        std::cerr << "Available: ";
        for (size_t i = 0; i < table.languages.size(); i++)
        {
            if (i > 0)
                std::cerr << ", ";
            std::cerr << table.languages[i];
        }
        std::cerr << std::endl;
        return 1;
    }
    std::string keyUpper = key;
    for (auto& c : keyUpper)
        c = (char)toupper((unsigned char)c);

    bool fileIsUtf8 = HasUtf8CsvSuffix(inputPath);
    Poseidon::Codepage cp =
        fileIsUtf8 ? Poseidon::Codepage::Utf8 : Poseidon::CodepageForLanguage(table.languages[langIdx].c_str());

    for (const auto& row : table.rows)
    {
        std::string rowKeyUpper = row.key;
        for (auto& c : rowKeyUpper)
        {
            c = (char)toupper((unsigned char)c);
        }
        if (rowKeyUpper == keyUpper)
        {
            if (static_cast<size_t>(langIdx) < row.values.size())
            {
                // Transcode to UTF-8 so the output is consistent regardless of source encoding
                std::cout << Poseidon::TranscodeToUtf8(row.values[langIdx], cp) << std::endl;
            }
            return 0;
        }
    }

    std::cerr << "Error: key '" << key << "' not found" << std::endl;
    return 1;
}

namespace
{

std::string LowerStr(std::string s)
{
    for (auto& c : s)
        c = (char)tolower((unsigned char)c);
    return s;
}

std::string UpperStr(std::string s)
{
    for (auto& c : s)
        c = (char)toupper((unsigned char)c);
    return s;
}

bool HasSuffix(const std::string& nameLower, const char* suffix)
{
    size_t n = std::strlen(suffix);
    return nameLower.size() >= n && nameLower.compare(nameLower.size() - n, n, suffix) == 0;
}

bool HasCsvSuffix(const std::string& nameLower)
{
    return HasSuffix(nameLower, ".csv");
}

// Basename (after the last '/' or '\\') of an in-PBO entry name, lower-cased.
std::string EntryBaseLower(const std::string& name)
{
    std::string lower = LowerStr(name);
    size_t pos = lower.find_last_of("/\\");
    return pos == std::string::npos ? lower : lower.substr(pos + 1);
}

// List addon .pbo files directly inside a directory (non-recursive).
void ListPbos(const std::string& dir, std::vector<std::string>& out)
{
    namespace fs = std::filesystem;
    std::error_code ec;
    if (!fs::is_directory(fs::path(dir), ec))
        return;
    for (const auto& e : fs::directory_iterator(fs::path(dir), ec))
    {
        if (e.is_regular_file(ec) && HasSuffix(LowerStr(e.path().filename().string()), ".pbo"))
            out.push_back(e.path().string());
    }
    std::sort(out.begin(), out.end());
}

// Strip a trailing .pbo (QFBank::open re-appends it).
std::string StripPbo(const std::string& path)
{
    return HasSuffix(LowerStr(path), ".pbo") ? path.substr(0, path.size() - 4) : path;
}

// Resolve a config path: if it is a directory, look for config.bin/config.cpp in it and a bin/ subdir.
std::string ResolveConfigPath(const std::string& in)
{
    namespace fs = std::filesystem;
    std::error_code ec;
    if (!fs::is_directory(fs::path(in), ec))
        return in;
    for (const fs::path& dir : {fs::path(in), fs::path(in) / "bin", fs::path(in) / "BIN"})
    {
        if (!fs::is_directory(dir, ec))
            continue;
        for (const auto& e : fs::directory_iterator(dir, ec))
        {
            std::string name = LowerStr(e.path().filename().string());
            if (name == "config.bin" || name == "config.cpp")
                return e.path().string();
        }
    }
    return "";
}

// Expand each stringtable input (file or directory) into a list of *stringtable*.csv files.
void CollectStringtableFiles(const std::string& in, std::vector<std::string>& out)
{
    namespace fs = std::filesystem;
    std::error_code ec;
    if (!fs::is_directory(fs::path(in), ec))
    {
        out.push_back(in);
        return;
    }
    for (const fs::path& dir : {fs::path(in), fs::path(in) / "bin", fs::path(in) / "BIN"})
    {
        if (!fs::is_directory(dir, ec))
            continue;
        for (const auto& e : fs::directory_iterator(dir, ec))
        {
            if (!e.is_regular_file(ec))
                continue;
            std::string name = LowerStr(e.path().filename().string());
            if (HasCsvSuffix(name) && name.find("stringtable") != std::string::npos)
                out.push_back(e.path().string());
        }
    }
}

std::string TrimCopy(std::string s)
{
    while (!s.empty() && isspace((unsigned char)s.front()))
        s.erase(s.begin());
    while (!s.empty() && isspace((unsigned char)s.back()))
        s.pop_back();
    return s;
}

} // namespace

void StringtableCommand::AddTableKeys(const Table& table, const std::string& source, KeyMap& out)
{
    for (const auto& row : table.rows)
    {
        KeyInfo& info = out[UpperStr(row.key)];
        if (info.source.empty())
            info.source = source;
        for (size_t i = 0; i < table.languages.size(); i++)
        {
            const std::string& lang = table.languages[i];
            info.langs.insert(lang);
            if (i < row.values.size() && !TrimCopy(row.values[i]).empty())
                info.filled.insert(lang);
        }
    }
}

int StringtableCommand::ReportMissing(const std::vector<std::pair<std::string, std::string>>& refs,
                                      const std::vector<const KeyMap*>& keySets, const std::string& label,
                                      bool checkLangs, std::set<std::string>& referenced)
{
    int failures = 0;
    for (const auto& [loc, value] : refs)
    {
        std::string key = value;
        if (!key.empty() && key[0] == '$')
            key.erase(0, 1);
        std::string up = UpperStr(key);
        referenced.insert(up);

        // Merge coverage across every consulted table that defines this key.
        bool present = false;
        std::set<std::string> declared, filled;
        for (const auto* ks : keySets)
        {
            auto it = ks->find(up);
            if (it == ks->end())
                continue;
            present = true;
            declared.insert(it->second.langs.begin(), it->second.langs.end());
            filled.insert(it->second.filled.begin(), it->second.filled.end());
        }

        if (!present)
        {
            std::cout << "MISSING " << value << "  <- " << label << loc << std::endl;
            failures++;
        }
        else if (checkLangs)
        {
            std::vector<std::string> gaps;
            for (const auto& lang : declared)
                if (filled.find(lang) == filled.end())
                    gaps.push_back(lang);
            if (!gaps.empty())
            {
                std::cout << "INCOMPLETE " << value << "  no value for: ";
                for (size_t i = 0; i < gaps.size(); i++)
                    std::cout << (i ? ", " : "") << gaps[i];
                std::cout << "  <- " << label << loc << std::endl;
                failures++;
            }
        }
    }
    return failures;
}

int StringtableCommand::CheckPbo(const std::string& pboPath, const KeyMap& extraKeys, const std::string& prefix,
                                 bool reportUnused, bool checkLangs)
{
    const std::string label = std::filesystem::path(pboPath).filename().string();

    QFBank bank;
    if (!bank.open(RString(StripPbo(pboPath).c_str())))
    {
        std::cerr << "Error: cannot open PBO: " << pboPath << std::endl;
        return -1;
    }
    bank.Lock();

    // Locate the config and stringtable entries (names/case vary between addons).
    PboInfo info = InspectPbo(pboPath);
    std::string configEntry;
    bool configIsBin = false;
    std::vector<std::string> stringtableEntries;
    for (const auto& e : info.entries)
    {
        std::string base = EntryBaseLower(e.name);
        if (configEntry.empty() && (base == "config.bin" || base == "config.cpp"))
        {
            configEntry = e.name;
            configIsBin = (base == "config.bin");
        }
        else if (HasCsvSuffix(base) && base.find("stringtable") != std::string::npos)
        {
            stringtableEntries.push_back(e.name);
        }
    }

    if (configEntry.empty())
    {
        // No config — nothing to check (e.g. a model/texture-only addon).
        bank.Unlock();
        return 0;
    }

    ParamFile cfg;
    bool parsed = false;
    if (configIsBin)
    {
        parsed = cfg.ParseBin(bank, configEntry.c_str());
    }
    else
    {
        // Rare: a text config.cpp inside the PBO. Extract to a temp file and parse via the file path.
        Ref<IFileBuffer> data = bank.Read(configEntry.c_str());
        if (data && data->GetSize() > 0)
        {
            std::filesystem::path tmp =
                std::filesystem::temp_directory_path() / ("poseidon_st_" + label + "_config.cpp");
            std::ofstream o(tmp, std::ios::binary);
            o.write(data->GetData(), data->GetSize());
            o.close();
            parsed = ConfigCommand::ParseConfig(cfg, tmp.string());
            std::error_code ec;
            std::filesystem::remove(tmp, ec);
        }
    }
    if (!parsed)
    {
        std::cerr << "Warning: " << label << ": failed to parse " << configEntry << std::endl;
        bank.Unlock();
        return -1;
    }

    std::vector<std::pair<std::string, std::string>> refs;
    ConfigCommand::CollectStrings(cfg, "", prefix, refs);

    // Keys defined in the addon's own stringtable(s).
    KeyMap ownKeys;
    for (const auto& st : stringtableEntries)
    {
        Ref<IFileBuffer> data = bank.Read(st.c_str());
        if (!data || data->GetSize() == 0)
            continue;
        std::string content(static_cast<const char*>(data->GetData()), static_cast<size_t>(data->GetSize()));
        Table table;
        if (!ParseCSVContent(content, table))
            continue;
        AddTableKeys(table, label + ":" + st, ownKeys);
    }
    bank.Unlock();

    std::set<std::string> referenced;
    int failures =
        ReportMissing(refs, {&ownKeys, &extraKeys}, label + ":" + configEntry + " -> ", checkLangs, referenced);

    if (reportUnused)
    {
        for (const auto& [up, info] : ownKeys)
        {
            if (referenced.find(up) == referenced.end())
                std::cout << "UNUSED  " << up << "  (" << info.source << ")" << std::endl;
        }
    }

    if (failures > 0 || !refs.empty())
    {
        std::cerr << "  " << label << ": " << refs.size() << " refs, " << ownKeys.size() << " own keys, " << failures
                  << (checkLangs ? " missing/incomplete" : " missing") << std::endl;
    }
    return failures;
}

int StringtableCommand::CheckConfig(const std::string& configPath, const std::vector<std::string>& stringtablePaths,
                                    const std::string& prefix, bool reportUnused, bool checkLangs)
{
    // Build the shared extra-key set from explicit stringtable inputs (e.g. the global table).
    std::vector<std::string> csvFiles;
    for (const auto& s : stringtablePaths)
        CollectStringtableFiles(s, csvFiles);
    KeyMap extraKeys;
    for (const auto& f : csvFiles)
    {
        Table table;
        if (!ParseCSV(f, table))
            continue;
        AddTableKeys(table, f, extraKeys);
    }

    const char* failWord = checkLangs ? "missing/incomplete" : "missing";
    std::vector<std::string> pbos;
    if (std::filesystem::is_directory(std::filesystem::path(configPath)))
        ListPbos(configPath, pbos);
    if (!pbos.empty())
    {
        std::cerr << "Addons: " << pbos.size() << " PBO(s) in " << configPath;
        if (!extraKeys.empty())
            std::cerr << " (+ " << extraKeys.size() << " extra keys)";
        std::cerr << std::endl;
        int totalFailures = 0, addonsWithFailures = 0, errors = 0;
        for (const auto& pbo : pbos)
        {
            int m = CheckPbo(pbo, extraKeys, prefix, reportUnused, checkLangs);
            if (m < 0)
                errors++;
            else if (m > 0)
            {
                totalFailures += m;
                addonsWithFailures++;
            }
        }
        std::cerr << pbos.size() << " addons checked, " << addonsWithFailures << " with issues, " << totalFailures
                  << " " << failWord << " total" << (errors ? ", " + std::to_string(errors) + " parse errors" : "")
                  << std::endl;
        return (totalFailures > 0 || errors > 0) ? 1 : 0;
    }
    if (HasSuffix(LowerStr(configPath), ".pbo"))
    {
        int m = CheckPbo(configPath, extraKeys, prefix, reportUnused, checkLangs);
        std::cerr << (m < 0 ? 0 : m) << " " << failWord << std::endl;
        return m > 0 ? 1 : (m < 0 ? 2 : 0);
    }
    std::string resolvedConfig = ResolveConfigPath(configPath);
    if (resolvedConfig.empty())
    {
        std::cerr << "Error: no config.bin/config.cpp found under: " << configPath << std::endl;
        return 2;
    }
    if (extraKeys.empty())
    {
        std::cerr << "Error: no stringtable CSV files given/found (required for a plain config)" << std::endl;
        return 2;
    }
    ParamFile cfg;
    if (!ConfigCommand::ParseConfig(cfg, resolvedConfig))
    {
        std::cerr << "Error: failed to parse config: " << resolvedConfig << std::endl;
        return 2;
    }
    std::vector<std::pair<std::string, std::string>> refs;
    ConfigCommand::CollectStrings(cfg, "", prefix, refs);

    std::cerr << "Config:       " << resolvedConfig << std::endl;
    std::cerr << "Stringtables: " << csvFiles.size() << " file(s), " << extraKeys.size() << " keys" << std::endl;

    std::set<std::string> referenced;
    int failures = ReportMissing(refs, {&extraKeys}, "", checkLangs, referenced);
    std::cerr << refs.size() << " references checked, " << failures << " " << failWord << std::endl;

    if (reportUnused)
    {
        int unused = 0;
        for (const auto& [up, info] : extraKeys)
        {
            if (referenced.find(up) == referenced.end())
            {
                std::cout << "UNUSED  " << up << "  (" << info.source << ")" << std::endl;
                unused++;
            }
        }
        std::cerr << unused << " keys not referenced by this config" << std::endl;
    }

    return failures > 0 ? 1 : 0;
}

} // namespace PoseidonTools
