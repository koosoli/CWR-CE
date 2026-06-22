#pragma once

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

#include <Poseidon/Asset/Probes/AssetInfo.hpp>
#include <Poseidon/Asset/Probes/SoundPlayer.hpp>
#include <Poseidon/IO/ParamFile/ParamFile.hpp>
#include "FileCategory.hpp"
#pragma push_macro("DebugLog")
#undef DebugLog
#include <imgui.h>
#pragma pop_macro("DebugLog")
#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <set>
#include <vector>
#include <algorithm>

struct SDL_Renderer;
struct SDL_Texture;

namespace Poseidon
{

struct FileEntry
{
    std::string name; // display name (relative path)
    std::string fullPath;
    std::string extension;
    int64_t size = 0;
    bool isDirectory = false;
    bool isPbo = false;

    std::string pboPath;
    std::string pboFilename;
    bool isVirtual() const { return !pboPath.empty(); }
};

struct AssetRef
{
    std::string name;    // original ref path (from P3D/font)
    std::string vfsPath; // resolved VFS display path for selection matching
    bool exists = false;
};

class StudioApp
{
  public:
    ~StudioApp();
    void setupTheme();
    void init(const std::string& gamePath, SDL_Renderer* renderer);
    void selectSingleFile(const std::string& absPath);
    void changeDirectory(const std::string& newPath);
    void refresh();
    void selectFile(const std::string& filename);
    void render(const ImVec2& displaySize);

    bool headless = false;
    bool singleFileMode = false;
    std::string openFilePath; // --open <path>
    ImFont* monoFont = nullptr;
    float sidebarRatio = 0.30f;

#ifdef IMGUI_ENABLE_TEST_ENGINE
    int testFilteredCount() const { return (int)filteredFiles.size(); }
    int testSelectedIndex() const { return selectedIndex; }
    FileCategory testCategory() const { return activeCategory; }
    void testSetCategory(FileCategory cat)
    {
        activeCategory = cat;
        applyFilter();
    }
    bool testGridMode() const { return shouldShowGrid(); }
    bool testLogCollapsed() const { return logCollapsed; }
    int testPreviewType() const { return (int)currentPreview; }
    float testZoom() const { return textureZoom; }
    const std::string& testSelectedFile() const { return selectedFile; }
    int testOsFileCount() const { return (int)osFiles.size(); }
    int testVfsFileCount() const { return (int)vfsFiles.size(); }
    void testResetSearch()
    {
        searchFilter.clear();
        applyFilter();
    }
    const std::string& testSearchFilter() const { return searchFilter; }
    void testRefresh() { refresh(); }
    int testTexW() const { return previewTexW; }
    int testTexH() const { return previewTexH; }
    int testMipCount() const { return textureMipCount; }
    int testOrigW() const { return textureOrigW; }
    int testOrigH() const { return textureOrigH; }
    int testModelLodCount() const { return modelLodCount; }
    const std::vector<std::string>& testModelLodNames() const { return modelLodNames; }
    const std::string& testPreviewInfo() const { return cachedInfoText; }
    int testActiveTab() const { return (int)activeTab; }
    void testSetTab(int tab)
    {
        activeTab = (BrowserTab)tab;
        pendingTabSwitch = tab;
        applyFilter();
    }
    int testSelectionCount() const { return (int)selectedIndices.size(); }
    bool testIsSelected(int idx) const { return selectedIndices.count(idx) > 0; }
    bool testGridDetailMode() const { return gridDetailMode; }
    int testGridDetailIndex() const { return gridDetailIndex; }
    void testSelectMultiple(const std::vector<int>& indices)
    {
        selectedIndices.clear();
        for (int i : indices)
            selectedIndices.insert(i);
        if (!indices.empty())
        {
            selectedIndex = indices.back();
            lastClickedIndex = indices.back();
        }
    }
    void testClearSelection()
    {
        selectedIndices.clear();
        selectedIndex = -1;
        gridDetailMode = false;
    }
    int testUsesCount() const { return (int)assetUses.size(); }
    int testUsedByCount() const { return (int)assetUsedBy.size(); }
    int testUsesFoundCount() const
    {
        int n = 0;
        for (const auto& r : assetUses)
            if (r.exists)
                n++;
        return n;
    }
    bool testUsedByScanning() const { return usedByScanning; }
    bool testUsedByScanDone() const { return usedByScanDone; }
    void testTriggerUsedByScan() { startUsedByScan(); }
    const std::vector<AssetRef>& testGetUses() const { return assetUses; }
    const std::vector<AssetRef>& testGetUsedBy() const { return assetUsedBy; }
    void testSelectFoundUses() { selectFoundRefs(assetUses); }
    void testSelectFoundUsedBy() { selectFoundRefs(assetUsedBy); }
    int testFindRef(const std::string& name) const { return findRefInFiltered(name); }
    void testNavigateTo(int idx)
    {
        selectedIndices.clear();
        selectedIndices.insert(idx);
        selectFileAt(idx);
    }
    void testSetGridDetail(int idx)
    {
        if (idx >= 0 && idx < (int)filteredFiles.size())
        {
            selectFileAt(idx);
            gridDetailMode = true;
            gridDetailIndex = idx;
            gridDetailEntry = filteredFiles[idx];
        }
    }
    int testConfigTopLevelCount() const { return configFile ? configFile->GetEntryCount() : 0; }
    int testConfigSearchResultCount() const { return (int)configSearchResults.size(); }
    bool testConfigSearchActive() const { return configSearchActive; }
    void testSetConfigSearch(const char* q)
    {
        strncpy(configSearch, q, sizeof(configSearch) - 1);
        configSearch[sizeof(configSearch) - 1] = '\0';
        runConfigSearch();
    }
#endif

  private:
    std::vector<FileEntry> osFiles;
    std::vector<FileEntry> vfsFiles;
    std::vector<FileEntry> filteredFiles;
    std::string searchFilter;
    FileCategory activeCategory = FileCategory::All;
    int categoryCounts[FileCategoryCount] = {};
    std::string selectedFile; // unique key: name for matching
    FileEntry selectedEntry;  // copy of the selected entry
    std::string gamePath;
    bool gamePathValid = false;
    int selectedIndex = -1; // index in filteredFiles (last clicked)
    bool scrollToSelected = false;
    std::set<int> selectedIndices; // multi-select: set of selected indices
    int lastClickedIndex = -1;     // for shift-click range selection

    // Grid detail mode: viewing a single file's detail from grid click
    bool gridDetailMode = false;
    int gridDetailIndex = -1;
    FileEntry gridDetailEntry;

    // Tab selection
    enum class BrowserTab
    {
        FileSystem,
        GameVFS
    };
    BrowserTab activeTab = BrowserTab::GameVFS;

    // SDL renderer for texture creation
    SDL_Renderer* sdlRenderer = nullptr;

    // Preview texture
    SDL_Texture* previewTexture = nullptr;
    int previewTexW = 0;
    int previewTexH = 0;
    std::string previewTextureFile; // tracks which file the texture is for

    void clearPreviewTexture();
    void loadImagePreview(const std::string& path);
    void loadTerrainPreview(const std::string& path);
    void loadModelPreview(const std::string& path);
    void selectFileAt(int index);

    // VFS: extract virtual file from PBO to temp for preview/inspect
    std::string extractVirtualFile(const FileEntry& entry);
    std::string resolveFilePath(const FileEntry& entry);
    std::string tempExtractedFile; // last extracted temp file

    // Cached inspection results (avoid re-inspecting every frame)
    std::string cachedInfoFile;
    std::string cachedInfoText;
    void updateCachedInfo(const FileEntry& entry);

    // Cached PBO info (avoid load/unload every frame)
    std::string cachedPboFile;
    PboInfo cachedPboInfo;

    // Panels
    void renderFileBrowser(float x, float y, float w, float h);
    void renderPreviewPanel(float x, float y, float w, float h);
    void renderInfoPanel(float x, float y, float w, float h);
    void renderLogPanel(float x, float y, float w, float h);
    void renderLoadingScreen(const ImVec2& displaySize);
    void renderFileGrid(float w, float h);
    bool shouldShowGrid() const;

    // File scanning (background thread)
    void scanFiles();
    void scanFilesThread();
    void applyFilter();
    void handleScanComplete();
    std::thread scanThread;
    std::atomic<bool> scanning{false};
    std::atomic<bool> scanDone{false};
    bool tabSwitchPending{false};
    int pendingTabSwitch{-1}; // -1=none, 0=FS, 1=VFS
    std::mutex scanStatusMutex;
    std::string scanStatusText;

    // Log
    std::vector<std::string> logMessages;
    void log(const std::string& msg);
    bool logCollapsed = false;

    // Preview state
    enum class PreviewType
    {
        None,
        Image,
        Model,
        Sound,
        Terrain,
        Pbo,
        Font,
        Config,
        Unknown
    };
    PreviewType currentPreview = PreviewType::None;
    std::string previewInfo;

    // Texture preview controls
    float textureZoom = 1.0f;
    ImVec2 texturePanOffset{0, 0};
    bool texturePanning = false;
    int textureMipLevel = 0;
    int textureMipCount = 0;
    int textureOrigW = 0; // mip 0 dimensions for percentage labels
    int textureOrigH = 0;

    // File grid browser
    int gridThumbSize = 128;
    int gridThumbMipTarget = 0; // tracks thumb size cache was built for
    struct GridThumb
    {
        SDL_Texture* tex = nullptr;
        int w = 0, h = 0;
        bool failed = false;
    };
    std::unordered_map<std::string, GridThumb> gridThumbs;
    void clearGridThumbs();
    GridThumb& getOrLoadThumb(const FileEntry& entry);
    void evictOffscreenThumbs(int visibleStart, int visibleEnd, int cols);

    // Asset cross-references ("Uses" = what this asset needs, "Used By" = what references it)
    std::vector<AssetRef> assetUses;   // textures used by current model/font
    std::vector<AssetRef> assetUsedBy; // models/fonts referencing current texture
    std::mutex usedByMutex;            // protects assetUsedBy
    std::string usedByScanKey;         // file for which usedBy was computed
    std::atomic<bool> usedByScanning{false};
    std::atomic<bool> usedByScanDone{false};
    std::atomic<int> usedByScanProgress{0};
    std::atomic<bool> usedByCancelFlag{false};
    int usedByScanTotal = 0;
    std::thread usedByThread;

    void startUsedByScan();
    void renderCrossRefs(float w);
    int findRefInFiltered(const std::string& refName) const;
    void selectFoundRefs(const std::vector<AssetRef>& refs);
    static std::string extractPboFile(const std::string& pboPath, const std::string& pboFilename);

    // Model preview controls
    int modelLodIndex = 0;
    int modelLodCount = 0;
    std::vector<std::string> modelLodNames; // "0: Geometry", "1: Memory", etc.
    std::string modelView = "front";
    std::string modelPreviewKey; // cache key: "path:lod:view"

    // Sound playback via shared SoundPlayer
    std::unique_ptr<Poseidon::SoundPlayer> soundPlayer;
    bool soundPlaying = false;
    std::string soundPlayingFile;
    std::chrono::steady_clock::time_point soundStartTime;
    void playSoundFile(const std::string& path);
    void playSoundEntry(const FileEntry& entry);
    void stopSound();

    // Config viewer
    struct ConfigSearchResult
    {
        std::string path;
        const ParamEntry* entry = nullptr;
    };
    std::shared_ptr<ParamFile> configFile;
    std::string configFilePath;
    char configSearch[256] = {};
    bool configSearchActive = false;
    std::vector<ConfigSearchResult> configSearchResults;
    int configForceOpen = 0;

    void renderConfigPanel(float x, float y, float w, float h);
    void runConfigSearch();
    static void collectConfigResults(const ParamClass& cls, const std::string& prefix, const std::string& query,
                                     std::vector<ConfigSearchResult>& out);
    static void renderConfigClassTree(const ParamClass& cls, int forceOpen);
    static StudioApp::PreviewType previewTypeFromExtension(const std::string& ext);
};

} // namespace Poseidon
