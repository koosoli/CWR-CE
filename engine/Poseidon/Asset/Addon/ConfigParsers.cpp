#include <Poseidon/Core/Global.hpp>
#include <Poseidon/Core/Application.hpp>
#include <Poseidon/IO/Streams/QBStream.hpp>
#include <Poseidon/IO/ParamFile/ParamFile.hpp>
#include <Poseidon/IO/ParamFileExt.hpp>
#include <Poseidon/UI/Locale/Stringtable/Stringtable.hpp>
#include <Poseidon/IO/Filesystem/DirScanner.hpp>
#include <stdio.h>
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <initializer_list>
#include <Poseidon/Foundation/Framework/Log.hpp>
#include <Poseidon/Foundation/Strings/RString.hpp>
#include <Poseidon/Foundation/Types/Pointers.hpp>
#include <Poseidon/Foundation/platform.hpp>
#ifdef _WIN32
#include <windows.h>

#endif

namespace Poseidon
{
extern bool g_stringtableSystemAvailable;

static RString MakeBinDir(RStringB dir, bool upperCase)
{
    if (dir.GetLength() == 0)
        return upperCase ? RString("BIN") : RString("bin");
    return dir + RString(upperCase ? "/BIN" : "/bin");
}

static bool ResolveFileInDir(RStringB dir, const char* name, RString& fullPath, RString& resolvedName)
{
    RString candidate = dir + RString("/") + RString(name);
    if (QIFStreamB::FileExist(candidate))
    {
        fullPath = candidate;
        resolvedName = name;
        return true;
    }

    DirScanner scan;
    if (!scan.First(dir, nullptr))
        return false;

    do
    {
        const char* entry = scan.GetName();
        if (strcmpi(entry, name) == 0)
        {
            resolvedName = entry;
            fullPath = dir + RString("/") + resolvedName;
            return true;
        }
    } while (scan.Next());

    return false;
}

// ParamFile::Parse resolves #include directives relative to the current working directory,
// so we chdir into the file's directory, parse the basename, and restore.
// Always parse the basename — passing dir/file.cpp after chdir to dir would double
// the prefix and silently parse nothing.
static bool ParseTextFileFromResolvedPath(ParamFile& target, RStringB fullPath, RStringB parseName)
{
    LSError err = target.Parse(fullPath);
    if (err == LSOK)
        return true;

    const char* fullPathStr = fullPath;
    const char* slash = strrchr(fullPathStr, '/');
    if (!slash)
        return false;

    char resolvedDir[512];
    snprintf(resolvedDir, sizeof(resolvedDir), "%.*s", (int)(slash - fullPathStr), fullPathStr);
    char buffer[512];
    ::GetCurrentDirectory(512, buffer);
    SetCurrentDirectory(resolvedDir);
    err = target.Parse(parseName);
    SetCurrentDirectory(buffer);
    return err == LSOK;
}

static SRef<ParamFile> s_deferredModConfig;

// True when a mod's bin/resource won the resource enumeration (replaced the base
// menu resource). Read by the main menu to keep the community addon's custom menu
// (and hijack it) instead of flipping to the remaster menu, and by config init to
// restore the base remaster UI resources the mod's resource shadowed.
static bool s_menuOverriddenByMod = false;

bool IsMenuOverriddenByMod()
{
    return s_menuOverriddenByMod;
}

void MergeBaseResourceExtra()
{
    // The base game's resource-extra.cpp carries the remaster's UI additions
    // (RscOptionsShell, RscDisplayMainRemaster, …) as new classes. The resource
    // enumeration (ParseResource) stops at the first mod that ships a bin/resource
    // — and Res.ParseBin clears Res on entry — so when a community mod replaces the
    // menu resource the base extra is never reached, leaving the new Options screen
    // empty and the Mods entry gone. Merge it on top of whatever the mod loaded so
    // the remaster UI survives any resource override.
    for (bool upperCase : {false, true})
    {
        RString binDir = MakeBinDir("", upperCase);
        RString extraFile;
        RString extraFileName;
        if (ResolveFileInDir(binDir, "resource-extra.cpp", extraFile, extraFileName))
        {
            ParamFile extra;
            ParseTextFileFromResolvedPath(extra, extraFile, extraFileName);
            Res.Update(extra);
            // Update copies array values verbatim, keeping their _file back-pointer aimed at the
            // stack-local `extra`; re-point them at Res before `extra` dies, or a later
            // string-expression GetFloat (e.g. a control's position/up/direction) derefs freed
            // stack (stack-use-after-return opening the Mods screen).
            Res.SetFile(&Res);
            LOG_INFO(Config, "MergeBaseResourceExtra: restored base {} over the mod resource", (const char*)extraFile);
            return;
        }
    }
}

static bool ParseConfigFromDir(const RStringB& dir, ParamFile& target)
{
    RString file;
    RString fileName;
    if (ResolveFileInDir(dir, "config.cpp", file, fileName))
        return ParseTextFileFromResolvedPath(target, file, fileName);
    if (ResolveFileInDir(dir, "config.bin", file, fileName))
    {
        target.ParseBin(file);
        return true;
    }
    return false;
}

bool ParseStringtable(RStringB dir, void* context)
{
    for (bool upperCase : {false, true})
    {
        RString binDir = MakeBinDir(dir, upperCase);
        RString file;
        RString fileName;
        if (ResolveFileInDir(binDir, "stringtable.csv", file, fileName))
        {
            LoadStringtable("Global", file, 0, false);
            g_stringtableSystemAvailable = true;
            return true;
        }
    }
    return false;
}

bool ParseConfig(RStringB dir, void* context)
{
    bool isMod = (dir.GetLength() > 0);
    if (isMod)
    {
        for (bool upperCase : {false, true})
        {
            RString binDir = MakeBinDir(dir, upperCase);
            SRef<ParamFile> modConfig = new ParamFile;
            if (ParseConfigFromDir(binDir, *modConfig))
            {
                s_deferredModConfig = modConfig;
                LOG_INFO(Config, "  Mod config found in {}, deferring merge", (const char*)binDir);
                break;
            }
        }
        return false;
    }

    for (bool upperCase : {false, true})
    {
        RString binDir = MakeBinDir(dir, upperCase);
        if (ParseConfigFromDir(binDir, Pars))
        {
            if (s_deferredModConfig)
            {
                LOG_INFO(Config, "  Merging deferred mod config into base");
                Pars.Update(*s_deferredModConfig);
                s_deferredModConfig = nullptr;
            }
            return true;
        }
    }

    return false;
}

bool ParseRemaster(RStringB dir, void* context)
{
    for (bool upperCase : {false, true})
    {
        RString binDir = MakeBinDir(dir, upperCase);
        RString file;
        RString fileName;
        if (ResolveFileInDir(binDir, "remaster.cpp", file, fileName))
            return ParseTextFileFromResolvedPath(Remaster, file, fileName);
        if (ResolveFileInDir(binDir, "remaster.bin", file, fileName))
        {
            Remaster.ParseBin(file);
            return true;
        }
    }
    return false;
}

bool ParseResource(RStringB dir, void* context)
{
    bool ok = false;
    RString binDirUsed;
    for (bool upperCase : {false, true})
    {
        RString binDir = MakeBinDir(dir, upperCase);
        RString file;
        RString fileName;
        if (ResolveFileInDir(binDir, "resource.cpp", file, fileName))
        {
            ParseTextFileFromResolvedPath(Res, file, fileName);
            ok = true;
            binDirUsed = binDir;
            break;
        }
        if (ResolveFileInDir(binDir, "resource.bin", file, fileName))
        {
            Res.ParseBin(file);
            ok = true;
            binDirUsed = binDir;
            break;
        }
    }

    if (!ok)
        return false;

    // Optional supplemental resources — resource-extra.cpp in the same bin/ directory
    // is merged via Update() so it can add new displays/templates without rebuilding
    // the pre-compiled RESOURCE.BIN. Parse + Update is required because ParamFile::Parse
    // clears entries on entry (paramFile.cpp:3206).
    {
        RString extraFile;
        RString extraFileName;
        if (ResolveFileInDir(binDirUsed, "resource-extra.cpp", extraFile, extraFileName))
        {
            ParamFile extra;
            ParseTextFileFromResolvedPath(extra, extraFile, extraFileName);
            Res.Update(extra);
            // Re-point merged array values at Res before the stack-local `extra` dies — Update
            // leaves their _file aimed at the source. Otherwise a string-expression GetFloat
            // (control position/up/direction) derefs freed stack (stack-use-after-return).
            Res.SetFile(&Res);
            LOG_INFO(Config, "ParseResource: merged {} into Res", (const char*)extraFile);
        }
    }

    // The winning resource (enumeration stops here) came from a mod when dir is
    // non-empty — the base menu resource was replaced.
    s_menuOverriddenByMod = (dir.GetLength() > 0);
    return true;
}

} // namespace Poseidon
