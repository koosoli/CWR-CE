#include <Evaluator/Validate.hpp>
#include <Evaluator/EvalState.hpp>
#include <Evaluator/SqsRunner.hpp>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <Poseidon/Foundation/Strings/RString.hpp>

#ifdef _WIN32
#include <io.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <dirent.h>
#endif

static std::vector<std::string> listFiles(const std::string& dir, const std::string& ext)
{
    std::vector<std::string> files;
#ifdef _WIN32
    // Normalize slashes for Windows
    std::string normDir = dir;
    for (auto& c : normDir)
        if (c == '/')
            c = '\\';
    // Remove trailing separator
    while (!normDir.empty() && (normDir.back() == '\\' || normDir.back() == '/'))
        normDir.pop_back();

    WIN32_FIND_DATAA fd;
    std::string pattern = normDir + "\\*" + ext;
    HANDLE h = FindFirstFileA(pattern.c_str(), &fd);
    if (h != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                files.push_back(normDir + "\\" + fd.cFileName);
        } while (FindNextFileA(h, &fd));
        FindClose(h);
    }
#else
    DIR* d = opendir(dir.c_str());
    if (d)
    {
        struct dirent* e;
        while ((e = readdir(d)))
        {
            std::string name = e->d_name;
            if (name.size() >= ext.size() && name.substr(name.size() - ext.size()) == ext)
                files.push_back(dir + "/" + name);
        }
        closedir(d);
    }
#endif
    std::sort(files.begin(), files.end());
    return files;
}

static std::string readAll(const std::string& path)
{
    std::ifstream f(path);
    if (!f.is_open())
        return "";
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

struct CodeBlock
{
    std::string code;
    std::string lang; // "sqf" or "sqs"
    int lineNumber;
};

static std::vector<CodeBlock> extractCodeBlocks(const std::string& content)
{
    std::vector<CodeBlock> blocks;
    std::istringstream iss(content);
    std::string line;
    int lineNum = 0;
    bool inBlock = false;
    std::string lang;
    std::string code;
    int blockStart = 0;

    while (std::getline(iss, line))
    {
        lineNum++;
        if (!inBlock)
        {
            if (line.find("```sqf") == 0 || line.find("```sqs") == 0)
            {
                inBlock = true;
                lang = line.substr(3, 3);
                code.clear();
                blockStart = lineNum;
            }
        }
        else
        {
            if (line.find("```") == 0)
            {
                inBlock = false;
                if (!code.empty())
                    blocks.push_back({code, lang, blockStart});
            }
            else
            {
                code += line + "\n";
            }
        }
    }
    return blocks;
}

// Replace non-ASCII bytes with spaces to avoid ctype assertion crashes
static std::string sanitizeAscii(const std::string& s)
{
    std::string out = s;
    for (auto& c : out)
        if ((unsigned char)c > 127)
            c = ' ';
    return out;
}

// Commands that need game engine objects and can't be evaluated standalone
static bool needsGameObjects(const std::string& code)
{
    static const char* markers[] = {"loadFile",         "addEventHandler",
                                    "addWaypoint",      "createTrigger",
                                    "setTrigger",       "setWaypointStatements",
                                    "onMapSingleClick", "onEachFrame",
                                    "playSound",        "playSoundAt",
                                    "playMusic",        "titleText",
                                    "cutText",          "titleRsc",
                                    "cutRsc",           "createDialog",
                                    "createDisplay",    "camCreate",
                                    "camSetTarget",     "camSetPos",
                                    "camCommit",        "camDestroy",
                                    "setRain",          "setFog",
                                    "setOvercast",      "setDate",
                                    "enableRadio",      "showCommandingMenu",
                                    "showMap",          "doMove",
                                    "doFire",           "doTarget",
                                    "commandMove",      "commandFire",
                                    "moveInDriver",     "moveInCargo",
                                    "moveInGunner",     "moveInCommander",
                                    "addMagazine",      "addWeapon",
                                    "removeWeapon",     "removeMagazine",
                                    "setObjectTexture", "setObjectMaterial",
                                    "forceAddUniform",  "addVest",
                                    "addBackpack",      nullptr};
    for (int i = 0; markers[i]; i++)
        if (code.find(markers[i]) != std::string::npos)
            return true;
    return false;
}

static ValidationResult tryEval(const std::string& source, int index, const std::string& code, const std::string& lang,
                                EvalState& state)
{
    ValidationResult res;
    res.source = source;
    res.index = index;
    res.code = code;
    res.status = "pass";

    if (needsGameObjects(code))
    {
        res.status = "skip";
        res.error = "uses game-object commands";
        return res;
    }

    std::string safeCode = sanitizeAscii(code);
    state.clearOutput();

    if (lang == "sqs")
    {
        SqsRunner runner(source);
        runner.parse(safeCode);
        int ret = runner.run(state, GameValue());
        if (ret != 0)
        {
            res.status = "fail";
            RString errText = state.GetLastErrorText();
            if (errText.GetLength() > 0)
                res.error = (const char*)errText;
            else
                res.error = "SQS execution error";
        }
    }
    else
    {
        GameVarSpace local(state.GetContext());
        state.BeginContext(&local);
        state.EvaluateMultiple(safeCode.c_str());
        EvalError err = state.GetLastError();
        state.EndContext();
        if (err != EvalOK)
        {
            res.status = "fail";
            RString errText = state.GetLastErrorText();
            if (errText.GetLength() > 0)
                res.error = (const char*)errText;
            else
                res.error = "evaluation error";
        }
    }
    return res;
}

ValidationSummary validateBook(const std::string& bookDir, GameState& gs, bool verbose)
{
    ValidationSummary summary;
    EvalState& state = static_cast<EvalState&>(gs);
    auto files = listFiles(bookDir, ".md");

    for (auto& file : files)
    {
        std::string content = readAll(file);
        if (content.empty())
            continue;

        auto blocks = extractCodeBlocks(content);
        // Extract just filename for display
        std::string fname = file;
        size_t sep = fname.find_last_of("/\\");
        if (sep != std::string::npos)
            fname = fname.substr(sep + 1);

        for (int i = 0; i < (int)blocks.size(); i++)
        {
            std::string src = fname + ":" + std::to_string(blocks[i].lineNumber);
            auto res = tryEval(src, i, blocks[i].code, blocks[i].lang, state);

            if (res.status == "pass")
                summary.passed++;
            else if (res.status == "fail")
                summary.failed++;
            else
                summary.skipped++;

            if (verbose || res.status == "fail")
                summary.results.push_back(res);
        }
    }
    return summary;
}

// Simple JSON string array extraction - handles nested brackets in strings
static std::string extractJsonStringArray(const std::string& json, const std::string& key)
{
    std::string needle = "\"" + key + "\"";
    size_t pos = json.find(needle);
    if (pos == std::string::npos)
        return "";
    pos = json.find('[', pos);
    if (pos == std::string::npos)
        return "";
    // Find matching ] accounting for strings
    int depth = 0;
    bool inStr = false;
    for (size_t i = pos; i < json.size(); i++)
    {
        if (inStr)
        {
            if (json[i] == '\\')
            {
                i++;
                continue;
            }
            if (json[i] == '"')
                inStr = false;
            continue;
        }
        if (json[i] == '"')
        {
            inStr = true;
            continue;
        }
        if (json[i] == '[')
            depth++;
        if (json[i] == ']')
        {
            depth--;
            if (depth == 0)
                return json.substr(pos, i - pos + 1);
        }
    }
    return "";
}

static std::vector<std::string> parseStringArray(const std::string& arr)
{
    std::vector<std::string> result;
    size_t pos = 0;
    while (true)
    {
        pos = arr.find('"', pos);
        if (pos == std::string::npos)
            break;
        pos++;
        size_t end = pos;
        // handle escaped quotes
        while (end < arr.size())
        {
            if (arr[end] == '\\')
            {
                end += 2;
                continue;
            }
            if (arr[end] == '"')
                break;
            end++;
        }
        if (end >= arr.size())
            break;
        std::string s = arr.substr(pos, end - pos);
        // unescape
        std::string unesc;
        for (size_t i = 0; i < s.size(); i++)
        {
            if (s[i] == '\\' && i + 1 < s.size())
            {
                unesc += s[i + 1];
                i++;
            }
            else
                unesc += s[i];
        }
        result.push_back(unesc);
        pos = end + 1;
    }
    return result;
}

static std::string extractJsonString(const std::string& json, const std::string& key)
{
    std::string needle = "\"" + key + "\"";
    size_t pos = json.find(needle);
    if (pos == std::string::npos)
        return "";
    pos = json.find('"', pos + needle.size());
    if (pos == std::string::npos)
        return "";
    pos++;
    size_t end = json.find('"', pos);
    if (end == std::string::npos)
        return "";
    return json.substr(pos, end - pos);
}

ValidationSummary validateRef(const std::string& refDir, GameState& gs, bool verbose)
{
    ValidationSummary summary;
    EvalState& state = static_cast<EvalState&>(gs);
    auto files = listFiles(refDir, ".json");

    for (auto& file : files)
    {
        std::string content = readAll(file);
        if (content.empty())
            continue;

        std::string fname = extractJsonString(content, "name");
        if (fname.empty())
        {
            size_t sep = file.find_last_of("/\\");
            fname = sep != std::string::npos ? file.substr(sep + 1) : file;
        }

        std::string exArr = extractJsonStringArray(content, "examples");
        auto examples = parseStringArray(exArr);

        for (int i = 0; i < (int)examples.size(); i++)
        {
            std::string src = fname;
            auto res = tryEval(src, i, examples[i], "sqf", state);

            if (res.status == "pass")
                summary.passed++;
            else if (res.status == "fail")
                summary.failed++;
            else
                summary.skipped++;

            if (verbose || res.status != "pass")
                summary.results.push_back(res);
        }
    }
    return summary;
}
