#include "ScanCommand.hpp"
#include <Poseidon/Asset/Probes/AssetInfo.hpp>
#include <Poseidon/IO/Streams/QBStream.hpp>
#include <Poseidon/IO/PackFiles.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <map>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <ctype.h>
#include <stdint.h>
#include <CLI/App.hpp>
#include <CLI/Option.hpp>
#include <CLI/Validators.hpp>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <format>
#include <fstream>
#include <functional>
#include <system_error>
#include <utility>
#include <Poseidon/Foundation/Strings/RString.hpp>
#include <Poseidon/Foundation/Types/Pointers.hpp>

namespace fs = std::filesystem;

namespace PoseidonTools
{

enum class FileCategory
{
    Model,     // .p3d
    Texture,   // .paa, .pac
    Animation, // .rtm
    Terrain,   // .wrp
    Sound,     // .wss, .ogg, .wav
    Pbo,       // .pbo
    Font,      // .fxy
    Config,    // .cpp, .bin, .ext, .sqm, .hpp
    Script,    // .sqs, .sqf
    LipSync,   // .lip
    Image,     // .jpg, .png, .bmp, .tga
    Document,  // .html, .txt, .csv, .pdf, .log
    Unknown
};

static const char* CategoryName(FileCategory cat)
{
    switch (cat)
    {
        case FileCategory::Model:
            return "Model (.p3d)";
        case FileCategory::Texture:
            return "Texture (.paa/.pac/.dds)";
        case FileCategory::Animation:
            return "Animation (.rtm)";
        case FileCategory::Terrain:
            return "Terrain (.wrp)";
        case FileCategory::Sound:
            return "Sound (.wss/.ogg/.wav)";
        case FileCategory::Pbo:
            return "PBO archive";
        case FileCategory::Font:
            return "Font (.fxy)";
        case FileCategory::Config:
            return "Config (ParamFile)";
        case FileCategory::Script:
            return "Script (.sqs/.sqf)";
        case FileCategory::LipSync:
            return "LipSync (.lip)";
        case FileCategory::Image:
            return "Image (.jpg/.png/.bmp/.tga)";
        case FileCategory::Document:
            return "Document (.html/.txt/.csv)";
        case FileCategory::Unknown:
            return "Unknown";
    }
    return "Unknown";
}

static FileCategory CategorizeByExtension(const std::string& ext)
{
    if (ext == ".p3d")
        return FileCategory::Model;
    if (ext == ".paa" || ext == ".pac" || ext == ".dds")
        return FileCategory::Texture;
    if (ext == ".rtm")
        return FileCategory::Animation;
    if (ext == ".wrp")
        return FileCategory::Terrain;
    if (ext == ".wss" || ext == ".ogg" || ext == ".wav")
        return FileCategory::Sound;
    if (ext == ".pbo")
        return FileCategory::Pbo;
    if (ext == ".fxy")
        return FileCategory::Font;
    if (ext == ".cpp" || ext == ".bin" || ext == ".ext" || ext == ".sqm" || ext == ".hpp" || ext == ".rvmat" ||
        ext == ".bisurf" || ext == ".bimpas" || ext == ".unit" || ext == ".cfg" || ext == ".desc")
        return FileCategory::Config;
    if (ext == ".sqs" || ext == ".sqf")
        return FileCategory::Script;
    if (ext == ".lip")
        return FileCategory::LipSync;
    if (ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".bmp" || ext == ".tga")
        return FileCategory::Image;
    if (ext == ".html" || ext == ".htm" || ext == ".txt" || ext == ".csv" || ext == ".pdf" || ext == ".log")
        return FileCategory::Document;
    return FileCategory::Unknown;
}

struct ScanItem
{
    std::string path;      // filesystem path (or pbo_path#entry for PBO internals)
    std::string pboPath;   // source PBO path (empty if loose file)
    std::string entryName; // entry name inside PBO (empty if loose file)
    std::string extension;
    int64_t size = 0;
    FileCategory category = FileCategory::Unknown;
};

struct ScanResult
{
    std::string path;
    FileCategory category = FileCategory::Unknown;
    std::string formatDetail; // e.g. "ODOL v7", "DXT1 256x256", "RTM v1.01 24 bones"
    int64_t size = 0;
    bool success = false;
    std::string error;
};

struct CategoryStats
{
    int total = 0;
    int passed = 0;
    int failed = 0;
    int64_t totalSize = 0;
};

// Global mutex for engine parsers that use shared state (RString, GFileServer, ModelCache)
static std::mutex g_engineMutex;

static std::string ExtractPboEntry(const std::string& pboPath, const std::string& entryName)
{
    std::string bankName = pboPath;
    if (bankName.size() >= 4)
    {
        std::string ext = bankName.substr(bankName.size() - 4);
        for (auto& c : ext)
            c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
        if (ext == ".pbo")
            bankName = bankName.substr(0, bankName.size() - 4);
    }

    std::lock_guard<std::mutex> lock(g_engineMutex);
    QFBank bank;
    if (!bank.open(RString(bankName.c_str())))
        return {};
    bank.Lock();
    if (bank.error())
    {
        bank.Unlock();
        return {};
    }

    const auto& fi = bank.FindFileInfo(entryName.c_str());
    if (QFBank::IsNull(fi))
    {
        bank.Unlock();
        return {};
    }

    auto buf = bank.Read(entryName.c_str());
    bank.Unlock();
    if (!buf || buf->GetSize() <= 0)
        return {};

    return std::string(buf->GetData(), buf->GetSize());
}

static std::atomic<uint64_t> g_tempFileCounter{0};

static std::string WriteTempFile(const std::string& data, const std::string& extension)
{
    auto tmpDir = std::filesystem::temp_directory_path();
    uint64_t id = g_tempFileCounter.fetch_add(1);
    auto tmpPath = tmpDir / ("poseidon-scan-" + std::to_string(id) + extension);
    std::ofstream out(tmpPath, std::ios::binary);
    if (!out)
        return {};
    out.write(data.data(), static_cast<std::streamsize>(data.size()));
    out.close();
    return tmpPath.string();
}

static ScanResult AnalyzeItem(const ScanItem& item)
{
    ScanResult result;
    result.path = item.pboPath.empty() ? item.path : (item.pboPath + "#" + item.entryName);
    result.category = item.category;
    result.size = item.size;

    try
    {
        std::string tempPath;
        std::string filePath = item.path;

        if (!item.pboPath.empty())
        {
            if (item.size == 0)
            {
                result.formatDetail = "0 B";
                result.success = true;
                return result;
            }
            auto data = ExtractPboEntry(item.pboPath, item.entryName);
            if (data.empty())
            {
                result.error = "Failed to extract from PBO";
                return result;
            }
            tempPath = WriteTempFile(data, item.extension);
            if (tempPath.empty())
            {
                result.error = "Failed to write temp file";
                return result;
            }
            filePath = tempPath;
        }
        switch (item.category)
        {
            case FileCategory::Model:
            {
                Poseidon::ModelInfo info;
                {
                    std::lock_guard<std::mutex> lock(g_engineMutex);
                    info = Poseidon::InspectModel(filePath);
                }
                if (info.valid)
                {
                    int totalVerts = 0;
                    for (const auto& lod : info.lods)
                        totalVerts += lod.points;
                    result.formatDetail = info.format + " v" + std::to_string(info.version) + " " +
                                          std::to_string(info.lodCount) + " LODs " + std::to_string(totalVerts) +
                                          " verts";
                    result.success = true;
                }
                else
                {
                    result.error = info.errorMessage.empty() ? "Parse failed" : info.errorMessage;
                }
                break;
            }
            case FileCategory::Texture:
            {
                Poseidon::TextureInfo info;
                {
                    std::lock_guard<std::mutex> lock(g_engineMutex);
                    info = Poseidon::InspectTexture(filePath);
                }
                if (info.valid)
                {
                    result.formatDetail = info.typeName + " " + info.formatName + " " + std::to_string(info.width) +
                                          "x" + std::to_string(info.height) + " " + std::to_string(info.mipmapCount) +
                                          " mips";
                    result.success = true;
                }
                else
                {
                    result.error = "PAA decode failed";
                }
                break;
            }
            case FileCategory::Animation:
            {
                Poseidon::AnimationInfo info;
                {
                    std::lock_guard<std::mutex> lock(g_engineMutex);
                    info = Poseidon::InspectAnimation(filePath);
                }
                if (info.valid)
                {
                    result.formatDetail = info.format + " " + std::to_string(info.boneCount) + " bones " +
                                          std::to_string(info.phaseCount) + " phases";
                    result.success = true;
                }
                else
                {
                    result.error = "RTM parse failed";
                }
                break;
            }
            case FileCategory::Terrain:
            {
                Poseidon::TerrainInfo info;
                {
                    std::lock_guard<std::mutex> lock(g_engineMutex);
                    info = Poseidon::InspectTerrain(filePath);
                }
                if (info.valid)
                {
                    result.formatDetail = info.formatName + " " + std::to_string(info.gridX) + "x" +
                                          std::to_string(info.gridZ) + " " + std::to_string(info.objectCount) +
                                          " objects";
                    result.success = true;
                }
                else
                {
                    result.error = "WRP parse failed";
                }
                break;
            }
            case FileCategory::Sound:
            {
                Poseidon::SoundInfo info;
                {
                    std::lock_guard<std::mutex> lock(g_engineMutex);
                    info = Poseidon::InspectSound(filePath);
                }
                if (info.valid)
                {
                    std::ostringstream ss;
                    ss << info.format << " " << info.channels << "ch " << info.sampleRate << "Hz " << std::fixed
                       << std::setprecision(1) << info.duration << "s";
                    result.formatDetail = ss.str();
                    result.success = true;
                }
                else
                {
                    result.error = "Sound parse failed";
                }
                break;
            }
            case FileCategory::Pbo:
            {
                Poseidon::PboInfo info;
                {
                    std::lock_guard<std::mutex> lock(g_engineMutex);
                    info = Poseidon::InspectPbo(filePath);
                }
                if (info.valid)
                {
                    result.formatDetail =
                        std::to_string(info.entries.size()) + " entries " + Poseidon::FormatSize(info.totalSize);
                    result.success = true;
                }
                else
                {
                    result.error = "PBO open failed";
                }
                break;
            }
            case FileCategory::Font:
            {
                Poseidon::FontInfo info;
                {
                    std::lock_guard<std::mutex> lock(g_engineMutex);
                    info = Poseidon::InspectFont(filePath);
                }
                if (info.valid)
                {
                    result.formatDetail = std::to_string(info.glyphCount) + " glyphs " +
                                          std::to_string(info.textureSetCount) + " texsets";
                    result.success = true;
                }
                else
                {
                    result.error = "FXY parse failed";
                }
                break;
            }
            case FileCategory::Config:
            {
                auto info = Poseidon::InspectConfig(filePath);
                if (info.valid)
                {
                    result.formatDetail = info.isBinarized ? ("ParamFile v" + std::to_string(info.version)) : "text";
                    result.formatDetail += " " + Poseidon::FormatSize(static_cast<long>(info.fileSize));
                    result.success = true;
                }
                else
                {
                    result.error = "Config read failed";
                }
                break;
            }
            case FileCategory::Script:
            {
                std::ifstream f(filePath);
                if (f)
                {
                    int lines = 0;
                    std::string line;
                    while (std::getline(f, line))
                        ++lines;
                    result.formatDetail = std::to_string(lines) + " lines";
                    result.success = true;
                }
                else
                {
                    result.error = "Cannot read script";
                }
                break;
            }
            case FileCategory::LipSync:
            {
                std::ifstream f(filePath, std::ios::binary | std::ios::ate);
                if (f)
                {
                    auto sz = f.tellg();
                    result.formatDetail = Poseidon::FormatSize(static_cast<long>(sz));
                    result.success = true;
                }
                else
                {
                    result.error = "Cannot read LIP file";
                }
                break;
            }
            case FileCategory::Unknown:
            {
                result.formatDetail = item.extension.empty() ? "(no extension)" : item.extension;
                result.success = true;
                break;
            }
            case FileCategory::Image:
            case FileCategory::Document:
            {
                std::error_code ec;
                auto sz = std::filesystem::file_size(filePath, ec);
                result.formatDetail =
                    Poseidon::FormatSize(static_cast<long>(ec ? item.size : static_cast<int64_t>(sz)));
                result.success = true;
                break;
            }
        }
        if (!tempPath.empty())
            std::remove(tempPath.c_str());
    }
    catch (const std::exception& e)
    {
        result.error = std::string("Exception: ") + e.what();
    }
    catch (...)
    {
        result.error = "Unknown exception";
    }

    return result;
}

struct PboCollectorCtx
{
    std::string pboPath;
    std::vector<ScanItem>* items;
};

static void CollectPboEntry(const FileInfoO& fi, const FileBankType*, void* ctx)
{
    auto* c = static_cast<PboCollectorCtx*>(ctx);
    std::string name = (const char*)fi.name;
    auto dot = name.find_last_of('.');
    std::string ext;
    if (dot != std::string::npos)
    {
        ext = name.substr(dot);
        for (auto& ch : ext)
            ch = static_cast<char>(tolower(static_cast<unsigned char>(ch)));
    }
    if (ext == ".pbo")
        return;

    ScanItem item;
    item.path = c->pboPath + "#" + name;
    item.pboPath = c->pboPath;
    item.entryName = name;
    item.extension = ext;
    item.size = fi.length;
    item.category = CategorizeByExtension(ext);
    c->items->push_back(std::move(item));
}

static std::string JsonEscape(const std::string& s)
{
    std::string out;
    out.reserve(s.size() + 8);
    for (char c : s)
    {
        switch (c)
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
                out += c;
                break;
        }
    }
    return out;
}

static void RunScan(const std::string& directory, int threadCount, bool verbose, bool jsonOutput)
{
    auto startTime = std::chrono::steady_clock::now();
    std::vector<ScanItem> items;
    int pboCount = 0;

    std::cout << "Scanning " << directory << " ..." << std::endl;

    for (auto& entry : fs::recursive_directory_iterator(directory, fs::directory_options::skip_permission_denied))
    {
        try
        {
            if (!entry.is_regular_file())
                continue;

            std::string path = entry.path().string();
            std::string ext = entry.path().extension().string();
            for (auto& c : ext)
                c = static_cast<char>(tolower(static_cast<unsigned char>(c)));

            std::error_code ec;
            int64_t fileSize = static_cast<int64_t>(entry.file_size(ec));
            if (ec)
                fileSize = 0;

            if (ext == ".pbo")
            {
                ScanItem pboItem;
                pboItem.path = path;
                pboItem.extension = ext;
                pboItem.size = fileSize;
                pboItem.category = FileCategory::Pbo;
                items.push_back(pboItem);
                std::string bankName = path.substr(0, path.size() - 4); // strip .pbo
                QFBank bank;
                if (bank.open(RString(bankName.c_str())))
                {
                    bank.Lock();
                    if (!bank.error())
                    {
                        PboCollectorCtx ctx{path, &items};
                        bank.ForEach(CollectPboEntry, &ctx);
                        pboCount++;
                    }
                    bank.Unlock();
                }
            }
            else
            {
                ScanItem item;
                item.path = path;
                item.extension = ext;
                item.size = fileSize;
                item.category = CategorizeByExtension(ext);
                items.push_back(std::move(item));
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "  Warning: skipping " << entry.path() << ": " << e.what() << std::endl;
        }
    }

    std::cout << "Found " << items.size() << " files (" << pboCount << " PBOs expanded)" << std::endl;

    if (items.empty())
    {
        std::cout << "Nothing to scan." << std::endl;
        return;
    }
    std::vector<ScanResult> results(items.size());
    std::atomic<size_t> nextItem{0};
    std::atomic<size_t> completed{0};
    std::mutex printMutex;

    auto worker = [&]()
    {
        while (true)
        {
            size_t idx = nextItem.fetch_add(1);
            if (idx >= items.size())
                break;

            results[idx] = AnalyzeItem(items[idx]);
            size_t done = completed.fetch_add(1) + 1;
            if (!verbose && (done % 500 == 0 || done == items.size()))
            {
                std::lock_guard<std::mutex> lock(printMutex);
                std::cout << "\r  Analyzed " << done << "/" << items.size() << " files" << std::flush;
            }

            if (verbose)
            {
                std::lock_guard<std::mutex> lock(printMutex);
                const auto& r = results[idx];
                std::cout << (r.success ? "  OK " : "FAIL ") << r.path;
                if (!r.formatDetail.empty())
                    std::cout << "  [" << r.formatDetail << "]";
                if (!r.error.empty())
                    std::cout << "  ERROR: " << r.error;
                std::cout << std::endl;
            }
        }
    };

    int nThreads = std::max(1, threadCount);
    if (nThreads == 1)
    {
        worker();
    }
    else
    {
        std::vector<std::thread> threads;
        threads.reserve(nThreads);
        for (int i = 0; i < nThreads; i++)
            threads.emplace_back(worker);
        for (auto& t : threads)
            t.join();
    }

    if (!verbose)
        std::cout << std::endl;

    auto endTime = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(endTime - startTime).count();
    std::map<FileCategory, CategoryStats> stats;
    std::vector<ScanResult> failures;
    std::map<std::string, int> unknownExtensions;

    for (const auto& r : results)
    {
        auto& s = stats[r.category];
        s.total++;
        s.totalSize += r.size;
        if (r.success)
            s.passed++;
        else
        {
            s.failed++;
            if (!r.error.empty())
                failures.push_back(r);
        }

        if (r.category == FileCategory::Unknown && !r.path.empty())
        {
            auto dot = r.path.find_last_of('.');
            std::string ext = (dot != std::string::npos) ? r.path.substr(dot) : "(none)";
            for (auto& c : ext)
                c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
            unknownExtensions[ext]++;
        }
    }

    if (jsonOutput)
    {
        std::cout << "{" << std::endl;
        std::cout << "  \"directory\": \"" << JsonEscape(directory) << "\"," << std::endl;
        std::cout << "  \"totalFiles\": " << items.size() << "," << std::endl;
        std::cout << "  \"pbosExpanded\": " << pboCount << "," << std::endl;
        std::cout << "  \"elapsedSeconds\": " << std::fixed << std::setprecision(2) << elapsed << "," << std::endl;
        std::cout << "  \"threads\": " << nThreads << "," << std::endl;
        std::cout << "  \"categories\": {" << std::endl;
        bool first = true;
        for (const auto& [cat, s] : stats)
        {
            if (!first)
                std::cout << "," << std::endl;
            first = false;
            std::cout << "    \"" << CategoryName(cat) << "\": {"
                      << "\"total\": " << s.total << ", \"passed\": " << s.passed << ", \"failed\": " << s.failed
                      << ", \"sizeBytes\": " << s.totalSize << "}";
        }
        std::cout << std::endl << "  }," << std::endl;
        std::cout << "  \"failures\": [" << std::endl;
        for (size_t i = 0; i < failures.size(); i++)
        {
            std::cout << "    {\"path\": \"" << JsonEscape(failures[i].path) << "\", \"error\": \""
                      << JsonEscape(failures[i].error) << "\"}";
            if (i + 1 < failures.size())
                std::cout << ",";
            std::cout << std::endl;
        }
        std::cout << "  ]" << std::endl;
        std::cout << "}" << std::endl;
    }
    else
    {
        std::cout << std::endl;
        std::cout << "╔══════════════════════════════╦════════╦════════╦════════╦═══════════════╗" << std::endl;
        std::cout << "║ Category                     ║  Total ║ Passed ║ Failed ║     Size      ║" << std::endl;
        std::cout << "╠══════════════════════════════╬════════╬════════╬════════╬═══════════════╣" << std::endl;

        int grandTotal = 0, grandPassed = 0, grandFailed = 0;
        int64_t grandSize = 0;

        for (const auto& [cat, s] : stats)
        {
            std::cout << "║ " << std::left << std::setw(29) << CategoryName(cat) << "║ " << std::right << std::setw(6)
                      << s.total << " ║ " << std::setw(6) << s.passed << " ║ " << std::setw(6) << s.failed << " ║ "
                      << std::setw(13) << Poseidon::FormatSize(static_cast<long>(s.totalSize)) << " ║" << std::endl;
            grandTotal += s.total;
            grandPassed += s.passed;
            grandFailed += s.failed;
            grandSize += s.totalSize;
        }

        std::cout << "╠══════════════════════════════╬════════╬════════╬════════╬═══════════════╣" << std::endl;
        std::cout << "║ " << std::left << std::setw(29) << "TOTAL"
                  << "║ " << std::right << std::setw(6) << grandTotal << " ║ " << std::setw(6) << grandPassed << " ║ "
                  << std::setw(6) << grandFailed << " ║ " << std::setw(13)
                  << Poseidon::FormatSize(static_cast<long>(grandSize)) << " ║" << std::endl;
        std::cout << "╚══════════════════════════════╩════════╩════════╩════════╩═══════════════╝" << std::endl;

        std::cout << std::endl
                  << "Scanned " << items.size() << " files in " << std::fixed << std::setprecision(2) << elapsed
                  << "s using " << nThreads << " thread(s)" << std::endl;
        if (!unknownExtensions.empty())
        {
            std::cout << std::endl << "Unknown extensions:" << std::endl;
            std::vector<std::pair<std::string, int>> sorted(unknownExtensions.begin(), unknownExtensions.end());
            std::sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) { return a.second > b.second; });
            for (const auto& [ext, count] : sorted)
                std::cout << "  " << std::setw(12) << ext << "  " << count << std::endl;
        }
        if (!failures.empty())
        {
            std::cout << std::endl << "Failed files (" << failures.size() << "):" << std::endl;
            for (const auto& f : failures)
                std::cout << "  " << f.path << ": " << f.error << std::endl;
        }
    }
}

void ScanCommand::Setup(CLI::App& app)
{
    auto* cmd = app.add_subcommand("scan", "Recursively scan directory for BIS file formats");

    static std::string directory;
    static int threads = 0;
    static bool verbose = false;
    static bool json = false;

    cmd->add_option("directory", directory, "Directory to scan")->required()->check(CLI::ExistingDirectory);
    cmd->add_option("-t,--threads", threads, "Number of worker threads (default: auto-detect)");
    cmd->add_flag("--verbose", verbose, "Print per-file results");
    cmd->add_flag("--json", json, "Output results as JSON");

    cmd->callback(
        [&]()
        {
            int t = threads;
            if (t <= 0)
                t = static_cast<int>(std::thread::hardware_concurrency());
            if (t <= 0)
                t = 4;

            RunScan(directory, t, verbose, json);
            std::exit(0);
        });
}

} // namespace PoseidonTools
