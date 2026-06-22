#include "ConfigCommand.hpp"
#include <Poseidon/IO/ParamFile/ParamFile.hpp>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <ctype.h>
#include <CLI/App.hpp>
#include <CLI/Option.hpp>
#include <CLI/TypeTools.hpp>
#include <cstdlib>
#include <functional>

namespace PoseidonTools
{

static std::string JsonEscape(const char* s)
{
    std::string out;
    for (; *s; ++s)
    {
        switch (*s)
        {
            case '"':
                out += "\\\"";
                break;
            case '\\':
                out += "\\\\";
                break;
            case '\n':
                out += "\\n";
                break;
            case '\r':
                out += "\\r";
                break;
            case '\t':
                out += "\\t";
                break;
            default:
                out += *s;
        }
    }
    return out;
}

static std::string Indent(int n)
{
    return std::string(n * 2, ' ');
}

bool ConfigCommand::ParseConfig(ParamFile& cfg, const std::string& path)
{
    std::ifstream check(path);
    if (!check.good())
        return false;
    check.close();
    if (cfg.ParseBin(path.c_str()))
        return true;
    return cfg.Parse(path.c_str()) == LSOK;
}

void ConfigCommand::WriteArrayValueJson(std::ostream& out, const IParamArrayValue& val)
{
    if (val.IsArrayValue())
    {
        out << "[";
        for (int i = 0; i < val.GetItemCount(); i++)
        {
            if (i > 0)
                out << ", ";
            WriteArrayValueJson(out, val[i]);
        }
        out << "]";
    }
    else if (val.IsFloatValue())
        out << val.GetFloat();
    else if (val.IsIntValue())
        out << val.GetInt();
    else
        out << "\"" << JsonEscape(val.GetValue().Data()) << "\"";
}

void ConfigCommand::WriteArrayJson(std::ostream& out, const ParamEntry& entry)
{
    out << "[";
    for (int i = 0; i < entry.GetSize(); i++)
    {
        if (i > 0)
            out << ", ";
        WriteArrayValueJson(out, entry[i]);
    }
    out << "]";
}

void ConfigCommand::WriteEntryJson(std::ostream& out, const ParamEntry& entry, int indent)
{
    if (entry.IsClass())
    {
        const ParamClass* cls = entry.GetClassInterface();
        out << Indent(indent) << "\"" << JsonEscape(entry.GetName().Data()) << "\": ";
        WriteClassJson(out, *cls, indent);
    }
    else if (entry.IsArray())
    {
        out << Indent(indent) << "\"" << JsonEscape(entry.GetName().Data()) << "\": ";
        WriteArrayJson(out, entry);
    }
    else if (entry.IsFloatValue())
        out << Indent(indent) << "\"" << JsonEscape(entry.GetName().Data()) << "\": " << (float)entry;
    else if (entry.IsIntValue())
        out << Indent(indent) << "\"" << JsonEscape(entry.GetName().Data()) << "\": " << (int)entry;
    else
        out << Indent(indent) << "\"" << JsonEscape(entry.GetName().Data()) << "\": \""
            << JsonEscape(entry.GetValue().Data()) << "\"";
}

void ConfigCommand::WriteClassJson(std::ostream& out, const ParamClass& cls, int indent)
{
    out << "{\n";
    for (int i = 0; i < cls.GetEntryCount(); i++)
    {
        if (i > 0)
            out << ",\n";
        WriteEntryJson(out, cls.GetEntry(i), indent + 1);
    }
    out << "\n" << Indent(indent) << "}";
}

void ConfigCommand::CollectStrings(const ParamClass& cls, const std::string& path, const std::string& prefix,
                                   std::vector<std::pair<std::string, std::string>>& out)
{
    for (int i = 0; i < cls.GetEntryCount(); i++)
    {
        const ParamEntry& entry = cls.GetEntry(i);
        std::string entryPath =
            path.empty() ? std::string(entry.GetName().Data()) : path + "." + std::string(entry.GetName().Data());

        if (entry.IsClass())
        {
            const ParamClass* sub = entry.GetClassInterface();
            if (sub)
                CollectStrings(*sub, entryPath, prefix, out);
        }
        else if (entry.IsArray())
        {
            for (int j = 0; j < entry.GetSize(); j++)
            {
                const IParamArrayValue& val = entry[j];
                if (val.IsTextValue())
                {
                    const char* v = val.GetValueRaw().Data();
                    if (v && v[0] == '$' && strncmp(v + 1, prefix.c_str(), prefix.size()) == 0)
                    {
                        std::string idx = entryPath + "[" + std::to_string(j) + "]";
                        out.emplace_back(idx, v);
                    }
                }
            }
        }
        else if (entry.IsTextValue())
        {
            const char* v = entry.GetValueRaw().Data();
            if (v && v[0] == '$' && strncmp(v + 1, prefix.c_str(), prefix.size()) == 0)
                out.emplace_back(entryPath, v);
        }
    }
}

static std::string Trim(std::string s)
{
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front())))
        s.erase(0, 1);
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back())))
        s.pop_back();
    return s;
}

static std::vector<std::string> SplitConfigPath(const std::string& path)
{
    std::vector<std::string> parts;
    std::string rem = path;
    while (true)
    {
        auto pos = rem.find(">>");
        if (pos == std::string::npos)
        {
            std::string part = Trim(rem);
            if (!part.empty())
                parts.push_back(part);
            break;
        }

        std::string part = Trim(rem.substr(0, pos));
        if (!part.empty())
            parts.push_back(part);
        rem = rem.substr(pos + 2);
    }
    return parts;
}

void ConfigCommand::Setup(CLI::App& app)
{
    auto* cfg = app.add_subcommand("config", "Config file operations (raP binary / cpp text)");
    {
        auto* sub = cfg->add_subcommand("debin", "Debinarize a raP config to text (cpp)");
        static std::string inputPath, outputPath;
        sub->add_option("input", inputPath, "Input binary config file (.bin)")->required();
        sub->add_option("-o,--output", outputPath, "Output text file (default: stdout)");
        sub->callback([&]() { std::exit(Debin(inputPath, outputPath)); });
    }
    {
        auto* sub = cfg->add_subcommand("bin", "Binarize a text config (cpp) to raP binary");
        static std::string inputPath, outputPath;
        sub->add_option("input", inputPath, "Input text config file (.cpp)")->required();
        sub->add_option("-o,--output", outputPath, "Output binary file (.bin)")->required();
        sub->callback([&]() { std::exit(Bin(inputPath, outputPath)); });
    }
    {
        auto* sub = cfg->add_subcommand(
            "merge", "Merge an overlay config into a base config, writing raP binary. The base is read as-is "
                     "(binary stays faithful); only overlay classes are added/updated.");
        static std::string basePath, overlayPath, outputPath;
        static bool forceOverwrite = false;
        sub->add_option("base", basePath, "Base config (.bin or .cpp) — kept verbatim")->required();
        sub->add_option("overlay", overlayPath, "Overlay config (.cpp or .bin) to merge in")->required();
        sub->add_option("-o,--output", outputPath, "Output binary file (.bin)")->required();
        sub->add_flag("--force-overwrite", forceOverwrite,
                      "Ignore config access locks while applying the overlay. Intended for asset tooling that patches "
                      "trusted binary configs and needs existing arrays/values replaced.");
        sub->callback([&]() { std::exit(Merge(basePath, overlayPath, outputPath, forceOverwrite)); });
    }

    // remove subcommand
    {
        auto* sub = cfg->add_subcommand("remove", "Remove explicit config entries/classes by >> path");
        static std::string inputPath, outputPath;
        static std::vector<std::string> paths;
        static bool forceOverwrite = false;
        static bool ignoreMissing = false;
        sub->add_option("input", inputPath, "Input config (.bin or .cpp)")->required();
        sub->add_option("paths", paths, "Entry/class paths to remove, e.g. \"CfgVoice >> Rob\"")->required();
        sub->add_option("-o,--output", outputPath, "Output binary file (.bin)")->required();
        sub->add_flag("--force-overwrite", forceOverwrite,
                      "Ignore config access locks while removing trusted entries from binary configs.");
        sub->add_flag("--ignore-missing", ignoreMissing, "Succeed when a requested path is already absent.");
        sub->callback([&]() { std::exit(Remove(inputPath, outputPath, paths, forceOverwrite, ignoreMissing)); });
    }
    {
        auto* sub = cfg->add_subcommand("dump", "Dump a specific class from a binary config");
        static std::string inputPath, className;
        sub->add_option("input", inputPath, "Input binary config file (.bin)")->required();
        sub->add_option("class", className, "Class name to dump (e.g. RscDisplayMain)")->required();
        sub->callback([&]() { std::exit(Dump(inputPath, className)); });
    }
    {
        auto* sub = cfg->add_subcommand("tojson", "Convert config to JSON");
        static std::string inputPath, outputPath;
        sub->add_option("input", inputPath, "Input config file (.bin or .cpp)")->required();
        sub->add_option("-o,--output", outputPath, "Output JSON file (default: stdout)");
        sub->callback([&]() { std::exit(ToJson(inputPath, outputPath)); });
    }
    {
        auto* sub = cfg->add_subcommand("strings", "Extract translation string references ($STR_)");
        static std::string inputPath, prefix;
        sub->add_option("input", inputPath, "Input config file (.bin or .cpp)")->required();
        sub->add_option("-p,--prefix", prefix, "Key prefix filter (default: STR_)")->default_val("STR_");
        sub->callback([&]() { std::exit(Strings(inputPath, prefix)); });
    }
    {
        auto* sub = cfg->add_subcommand("search", "Search for a class/entry by name or >> path");
        static std::string inputPath, query;
        sub->add_option("input", inputPath, "Input config file (.bin or .cpp)")->required();
        sub->add_option("query", query, "Entry name or path (e.g. \"CfgVehicles >> Car\")")->required();
        sub->callback([&]() { std::exit(Search(inputPath, query)); });
    }
}

int ConfigCommand::Debin(const std::string& inputPath, const std::string& outputPath)
{
    ParamFile cfg;
    if (!cfg.ParseBin(inputPath.c_str()))
    {
        std::cerr << "Error: failed to parse binary config: " << inputPath << std::endl;
        return 1;
    }

    if (outputPath.empty())
    {
        QOStream buf;
        cfg.Save(buf, 0);
        std::cout.write(buf.str(), buf.tellp());
    }
    else
    {
        LSError err = cfg.Save(outputPath.c_str());
        if (err != LSOK)
        {
            std::cerr << "Error: failed to write output: " << outputPath << std::endl;
            return 1;
        }
        std::cerr << "Written: " << outputPath << std::endl;
    }
    return 0;
}

int ConfigCommand::Bin(const std::string& inputPath, const std::string& outputPath)
{
    ParamFile cfg;
    const bool fromBinary = cfg.ParseBin(inputPath.c_str());
    if (!fromBinary && cfg.Parse(inputPath.c_str()) != LSOK)
    {
        std::cerr << "Error: failed to parse config: " << inputPath << std::endl;
        return 1;
    }

    if (!fromBinary)
    {
        // A rapified config carries a trailing enum/variable table (named constants like cpcommander)
        // that the text form does not represent, so a text -> binary conversion drops it. To edit an
        // existing binary config without losing it, use `config merge` (its base goes through ParseBin).
        std::cerr << "Note: binarising from text does not reconstruct the enum/variable table; "
                     "use `config merge` to edit an existing binary config and keep it."
                  << std::endl;
    }

    if (!cfg.SaveBin(outputPath.c_str()))
    {
        std::cerr << "Error: failed to write binary config: " << outputPath << std::endl;
        return 1;
    }
    std::cerr << "Written: " << outputPath << std::endl;
    return 0;
}

int ConfigCommand::Merge(const std::string& basePath, const std::string& overlayPath, const std::string& outputPath,
                         bool forceOverwrite)
{
    ParamFile cfg;
    if (!ParseConfig(cfg, basePath))
    {
        std::cerr << "Error: failed to parse base config: " << basePath << std::endl;
        return 1;
    }

    ParamFile overlay;
    if (!ParseConfig(overlay, overlayPath))
    {
        std::cerr << "Error: failed to parse overlay config: " << overlayPath << std::endl;
        return 1;
    }

    if (forceOverwrite)
    {
        cfg.SetAccessModeForAll(PADefault);
    }

    cfg.Update(overlay);

    if (!cfg.SaveBin(outputPath.c_str()))
    {
        std::cerr << "Error: failed to write binary config: " << outputPath << std::endl;
        return 1;
    }
    std::cerr << "Written: " << outputPath << std::endl;
    return 0;
}

int ConfigCommand::Remove(const std::string& inputPath, const std::string& outputPath,
                          const std::vector<std::string>& paths, bool forceOverwrite, bool ignoreMissing)
{
    ParamFile cfg;
    if (!ParseConfig(cfg, inputPath))
    {
        std::cerr << "Error: failed to parse config: " << inputPath << std::endl;
        return 1;
    }

    if (forceOverwrite)
    {
        cfg.SetAccessModeForAll(PADefault);
    }

    for (const std::string& path : paths)
    {
        std::vector<std::string> parts = SplitConfigPath(path);
        if (parts.empty())
        {
            std::cerr << "Error: empty path" << std::endl;
            return 1;
        }

        ParamClass* parent = &cfg;
        std::string pathSoFar;
        for (size_t i = 0; i + 1 < parts.size(); ++i)
        {
            const std::string& part = parts[i];
            pathSoFar += (pathSoFar.empty() ? "" : " >> ") + part;
            ParamEntry* entry = parent->FindEntryNoInheritance(part.c_str());
            if (!entry)
            {
                if (ignoreMissing)
                {
                    std::cerr << "Missing: " << pathSoFar << std::endl;
                    parent = nullptr;
                    break;
                }
                std::cerr << "Error: not found: " << pathSoFar << std::endl;
                return 1;
            }
            parent = entry->GetClassInterface();
            if (!parent)
            {
                std::cerr << "Error: not a class: " << pathSoFar << std::endl;
                return 1;
            }
        }
        if (!parent)
        {
            continue;
        }

        const std::string& leaf = parts.back();
        if (!parent->FindEntryNoInheritance(leaf.c_str()))
        {
            if (ignoreMissing)
            {
                std::cerr << "Missing: " << path << std::endl;
                continue;
            }
            std::cerr << "Error: not found: " << path << std::endl;
            return 1;
        }
        parent->Delete(leaf.c_str());
        std::cerr << "Removed: " << path << std::endl;
    }

    if (!cfg.SaveBin(outputPath.c_str()))
    {
        std::cerr << "Error: failed to write binary config: " << outputPath << std::endl;
        return 1;
    }
    std::cerr << "Written: " << outputPath << std::endl;
    return 0;
}

int ConfigCommand::Dump(const std::string& inputPath, const std::string& className)
{
    ParamFile cfg;
    if (!cfg.ParseBin(inputPath.c_str()))
    {
        std::cerr << "Error: failed to parse binary config: " << inputPath << std::endl;
        return 1;
    }

    const ParamEntry* entry = cfg.FindEntry(className.c_str());
    if (!entry)
    {
        std::cerr << "Error: class '" << className << "' not found in " << inputPath << std::endl;
        std::cerr << "Available top-level entries (" << cfg.GetEntryCount() << "):" << std::endl;
        for (int i = 0; i < cfg.GetEntryCount() && i < 50; i++)
        {
            std::cerr << "  " << cfg.GetEntry(i).GetName() << std::endl;
        }
        return 1;
    }

    QOStream buf;
    entry->Save(buf, 0);
    std::cout.write(buf.str(), buf.tellp());
    std::cout << std::endl;
    return 0;
}

int ConfigCommand::ToJson(const std::string& inputPath, const std::string& outputPath)
{
    ParamFile cfg;
    if (!ParseConfig(cfg, inputPath))
    {
        std::cerr << "Error: failed to parse config: " << inputPath << std::endl;
        return 1;
    }

    if (outputPath.empty())
    {
        WriteClassJson(std::cout, cfg, 0);
        std::cout << std::endl;
    }
    else
    {
        std::ofstream out(outputPath);
        if (!out)
        {
            std::cerr << "Error: failed to open output: " << outputPath << std::endl;
            return 1;
        }
        WriteClassJson(out, cfg, 0);
        out << std::endl;
        std::cerr << "Written: " << outputPath << std::endl;
    }
    return 0;
}

int ConfigCommand::Strings(const std::string& inputPath, const std::string& prefix)
{
    ParamFile cfg;
    if (!ParseConfig(cfg, inputPath))
    {
        std::cerr << "Error: failed to parse config: " << inputPath << std::endl;
        return 1;
    }

    std::vector<std::pair<std::string, std::string>> results;
    CollectStrings(cfg, "", prefix, results);

    for (const auto& [path, key] : results)
        std::cout << path << "," << key << std::endl;

    std::cerr << results.size() << " string references found" << std::endl;
    return 0;
}

int ConfigCommand::Search(const std::string& inputPath, const std::string& query)
{
    ParamFile cfg;
    if (!ParseConfig(cfg, inputPath))
    {
        std::cerr << "Error: failed to parse config: " << inputPath << std::endl;
        return 1;
    }

    std::string q = query;
    while (!q.empty() && q.front() == ' ')
        q.erase(0, 1);
    while (!q.empty() && q.back() == ' ')
        q.pop_back();

    if (q.empty())
    {
        std::cerr << "Error: empty query" << std::endl;
        return 1;
    }

    if (q.find(">>") != std::string::npos)
    {
        std::vector<std::string> parts;
        std::string rem = q;
        while (true)
        {
            auto pos = rem.find(">>");
            if (pos == std::string::npos)
            {
                while (!rem.empty() && rem.front() == ' ')
                    rem.erase(0, 1);
                while (!rem.empty() && rem.back() == ' ')
                    rem.pop_back();
                if (!rem.empty())
                    parts.push_back(rem);
                break;
            }
            std::string part = rem.substr(0, pos);
            while (!part.empty() && part.front() == ' ')
                part.erase(0, 1);
            while (!part.empty() && part.back() == ' ')
                part.pop_back();
            if (!part.empty())
                parts.push_back(part);
            rem = rem.substr(pos + 2);
        }

        const ParamClass* cur = &cfg;
        std::string pathSoFar;
        for (size_t pi = 0; pi < parts.size() && cur; pi++)
        {
            const std::string& p = parts[pi];
            pathSoFar += (pathSoFar.empty() ? "" : " >> ") + p;
            const ParamEntry* found = cur->FindEntryNoInheritance(p.c_str());
            if (!found)
            {
                std::cerr << "Not found: " << pathSoFar << std::endl;
                return 1;
            }
            if (pi == parts.size() - 1)
            {
                QOStream buf;
                found->Save(buf, 0);
                std::cout.write(buf.str(), buf.tellp());
                std::cout << std::endl;
                return 0;
            }
            cur = found->GetClassInterface();
            if (!cur)
            {
                std::cerr << "Not a class: " << pathSoFar << std::endl;
                return 1;
            }
        }
    }
    else
    {
        struct Finder
        {
            static void Find(const ParamClass& cls, const std::string& prefix, const std::string& name,
                             std::vector<std::string>& results)
            {
                std::string nameLower = name;
                std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
                for (int i = 0; i < cls.GetEntryCount(); i++)
                {
                    const ParamEntry& e = cls.GetEntry(i);
                    std::string en = e.GetName().Data();
                    std::string enLower = en;
                    std::transform(enLower.begin(), enLower.end(), enLower.begin(), ::tolower);
                    std::string path = prefix.empty() ? en : prefix + " >> " + en;
                    if (enLower.find(nameLower) != std::string::npos)
                        results.push_back(path);
                    if (e.IsClass())
                    {
                        const ParamClass* sub = e.GetClassInterface();
                        if (sub)
                            Find(*sub, path, name, results);
                    }
                }
            }
        };
        std::vector<std::string> results;
        Finder::Find(cfg, "", q, results);
        for (const auto& r : results)
            std::cout << r << std::endl;
        std::cerr << results.size() << " match(es) found" << std::endl;
    }
    return 0;
}

} // namespace PoseidonTools
