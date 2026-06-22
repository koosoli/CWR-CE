#pragma once
#include <CLI/CLI.hpp>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace PoseidonTools
{

class StringtableCommand
{
  public:
    static void Setup(CLI::App& app);

  private:
    struct Row
    {
        std::string key;
        std::vector<std::string> values;
    };
    struct Table
    {
        std::vector<std::string> languages;
        std::vector<Row> rows;
    };

    // Per-key translation coverage, merged across every stringtable that defines the key.
    struct KeyInfo
    {
        std::string source;           // first file/PBO entry that defined the key
        std::set<std::string> langs;  // language columns declared by tables that define this key
        std::set<std::string> filled; // languages with a non-empty value somewhere
    };
    using KeyMap = std::unordered_map<std::string, KeyInfo>;

    static bool ParseCSV(const std::string& path, Table& table);
    static bool ParseCSVContent(const std::string& content, Table& table);
    static std::string ReadField(const char*& p, const char* end);

    // Merge a parsed table's rows (keys + per-language non-empty coverage) into `out` (keyed upper-cased).
    static void AddTableKeys(const Table& table, const std::string& source, KeyMap& out);
    // Report config $STR references missing from `keySets` (MISSING) and, when checkLangs, references whose
    // key exists but lacks a value in some declared language (INCOMPLETE). Returns the count of failures.
    static int ReportMissing(const std::vector<std::pair<std::string, std::string>>& refs,
                             const std::vector<const KeyMap*>& keySets, const std::string& label, bool checkLangs,
                             std::set<std::string>& referenced);

    static int Validate(const std::string& inputPath);
    static int Inspect(const std::string& inputPath);
    static int Lookup(const std::string& inputPath, const std::string& language, const std::string& key);
    static int CheckConfig(const std::string& configPath, const std::vector<std::string>& stringtablePaths,
                           const std::string& prefix, bool reportUnused, bool checkLangs);
    // Check a single addon PBO: parse its config, gather $STR refs, and verify each is defined in the
    // addon's own stringtable.csv or in `extraKeys` (e.g. the global table). Returns failure count, -1 on error.
    static int CheckPbo(const std::string& pboPath, const KeyMap& extraKeys, const std::string& prefix,
                        bool reportUnused, bool checkLangs);
};

} // namespace PoseidonTools
