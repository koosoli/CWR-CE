#pragma once

#include <CLI/CLI.hpp>
#include <Poseidon/IO/ParamFile/ParamFile.hpp>
#include <string>
#include <utility>
#include <vector>

namespace PoseidonTools
{

class ConfigCommand
{
  public:
    static void Setup(CLI::App& app);

    // Parse a config file (.bin binary raP or .cpp text). Reusable by other commands.
    static bool ParseConfig(ParamFile& cfg, const std::string& path);
    // Recursively collect translation references (values like "$STR...") as (path, value) pairs.
    // `prefix` matches the text after the leading '$' (e.g. "STR" catches $STR/$STRM/$STRN...).
    static void CollectStrings(const ParamClass& cls, const std::string& path, const std::string& prefix,
                               std::vector<std::pair<std::string, std::string>>& out);

  private:
    static int Debin(const std::string& inputPath, const std::string& outputPath);
    static int Bin(const std::string& inputPath, const std::string& outputPath);
    static int Merge(const std::string& basePath, const std::string& overlayPath, const std::string& outputPath,
                     bool forceOverwrite);
    static int Remove(const std::string& inputPath, const std::string& outputPath,
                      const std::vector<std::string>& paths, bool forceOverwrite, bool ignoreMissing);
    static int Dump(const std::string& inputPath, const std::string& className);
    static int ToJson(const std::string& inputPath, const std::string& outputPath);
    static int Strings(const std::string& inputPath, const std::string& prefix);
    static int Search(const std::string& inputPath, const std::string& query);

    static void WriteClassJson(std::ostream& out, const ParamClass& cls, int indent);
    static void WriteEntryJson(std::ostream& out, const ParamEntry& entry, int indent);
    static void WriteArrayJson(std::ostream& out, const ParamEntry& entry);
    static void WriteArrayValueJson(std::ostream& out, const IParamArrayValue& val);
};

} // namespace PoseidonTools
