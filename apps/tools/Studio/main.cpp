#include "StudioApp.hpp"
#include "StudioConfig.hpp"
#include <Poseidon/Audio/AudioFactory.hpp>
#include <Poseidon/Audio/Voice/VoiceBackend.hpp>
#include <Poseidon/Graphics/Textures/TextureBank.hpp>
#include <CLI/CLI.hpp>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <algorithm>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_surface.h>
#include <SDL3/SDL_video.h>
#include <CLI/App.hpp>
#pragma push_macro("DebugLog")
#undef DebugLog
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <imgui_freetype.h>
#pragma pop_macro("DebugLog")
#include <Poseidon/Foundation/Common/PlatformPaths.hpp>
#include <cstdio>
#include <filesystem>
#include <string>

#ifdef _WIN32
#include <windows.h>
#include <shobjidl.h>

static std::string showFolderDialog()
{
    std::string result;
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    IFileDialog* dlg = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, IID_IFileDialog, (void**)&dlg)))
    {
        DWORD opts;
        dlg->GetOptions(&opts);
        dlg->SetOptions(opts | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
        dlg->SetTitle(L"Select Game Content Directory");
        if (SUCCEEDED(dlg->Show(nullptr)))
        {
            IShellItem* item = nullptr;
            if (SUCCEEDED(dlg->GetResult(&item)))
            {
                PWSTR wpath = nullptr;
                if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &wpath)))
                {
                    int len = WideCharToMultiByte(CP_UTF8, 0, wpath, -1, nullptr, 0, nullptr, nullptr);
                    result.resize(len - 1);
                    WideCharToMultiByte(CP_UTF8, 0, wpath, -1, result.data(), len, nullptr, nullptr);
                    CoTaskMemFree(wpath);
                }
                item->Release();
            }
        }
        dlg->Release();
    }
    CoUninitialize();
    return result;
}
#endif

static ImFont* monoFont = nullptr;

static const char* findFont(const char* const* paths)
{
    for (int i = 0; paths[i]; i++)
    {
        FILE* f = fopen(paths[i], "rb");
        if (f)
        {
            fclose(f);
            return paths[i];
        }
    }
    return nullptr;
}

static void loadFont()
{
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->SetFontLoader(ImGuiFreeType::GetFontLoader());
    ImFontConfig cfg;
    cfg.FontLoaderFlags = ImGuiFreeTypeLoaderFlags_ForceAutoHint;

#ifdef _WIN32
    const char* fontPath = "C:\\Windows\\Fonts\\segoeui.ttf";
    const char* monoPath = "C:\\Windows\\Fonts\\consola.ttf";
#else
    const char* sansPaths[] = {"/usr/share/fonts/dejavu-sans-fonts/DejaVuSans.ttf",
                               "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", "/usr/share/fonts/TTF/DejaVuSans.ttf",
                               nullptr};
    const char* monoPaths[] = {"/usr/share/fonts/dejavu-sans-mono-fonts/DejaVuSansMono.ttf",
                               "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
                               "/usr/share/fonts/TTF/DejaVuSansMono.ttf", nullptr};
    const char* fontPath = findFont(sansPaths);
    const char* monoPath = findFont(monoPaths);
#endif

    if (!fontPath || !io.Fonts->AddFontFromFileTTF(fontPath, 16.0f, &cfg))
    {
        fprintf(stderr, "Sans font not found, using default\n");
        io.Fonts->AddFontDefault(&cfg);
    }

    if (monoPath)
        monoFont = io.Fonts->AddFontFromFileTTF(monoPath, 14.0f, &cfg);
    if (!monoFont)
        monoFont = io.Fonts->AddFontDefault(&cfg);
}

static std::string iniFilePath;

static const char* getConfigIniPath()
{
    iniFilePath = Poseidon::Foundation::getUserConfigDir("PoseidonStudio") + "/imgui.ini";
    return iniFilePath.c_str();
}

static void saveScreenshot(SDL_Renderer* renderer, const char* path)
{
    SDL_Surface* surface = SDL_RenderReadPixels(renderer, nullptr);
    if (!surface)
        return;
    SDL_Surface* rgb = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGB24);
    SDL_SaveBMP(rgb ? rgb : surface, path);
    if (rgb)
        SDL_DestroySurface(rgb);
    SDL_DestroySurface(surface);
    fprintf(stdout, "Screenshot saved: %s\n", path);
}

int main(int argc, char* argv[])
{
    Poseidon::RegisterDummyAudioBackend();
    Poseidon::RegisterTextAudioBackend();
    Poseidon::RegisterOpenALAudioBackend();
    Poseidon::RegisterOpenALVoiceBackend();

    CLI::App cliApp{"PoseidonStudio - Asset browser and previewer for Arma: Cold War Assault - Remastered"};

    std::string gamePath;
    std::string openFile;
    std::string dragDropPath;
    bool headless = false;
    std::string screenshotFile;

    cliApp.add_option("-C,--content", gamePath, "Game content directory (like game -C flag)");
    cliApp.add_option("--open", openFile, "Open file on startup (path relative to content dir)");
    cliApp.add_flag("--headless", headless, "Run without display (CI/test mode)");
    cliApp.add_option("--screenshot", screenshotFile, "Capture screenshot and exit");
    cliApp.add_option("path", dragDropPath, "Directory or file to open (drag-and-drop)");
    CLI11_PARSE(cliApp, argc, argv);

    if (gamePath.empty() && !dragDropPath.empty())
    {
        namespace fs = std::filesystem;
        if (fs::is_directory(dragDropPath))
        {
            gamePath = dragDropPath;
        }
        else if (fs::is_regular_file(dragDropPath))
        {
            gamePath = fs::path(dragDropPath).parent_path().string();
        }
    }

    Poseidon::StudioConfig cfg;
    if (!headless)
        cfg.load();

#ifdef _WIN32
    if (!headless)
        FreeConsole();
#endif

    if (gamePath.empty() && !headless)
    {
#ifdef _WIN32
        gamePath = showFolderDialog();
        if (gamePath.empty())
            return 0; // user cancelled
#else
        fprintf(stderr, "Error: No content directory specified. Use -C <path> or pass a directory as argument.\n");
        return 1;
#endif
    }

    // Studio previews assets without a full runtime world.
    Poseidon::NoTextures = true;

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("PoseidonStudio", cfg.winW, cfg.winH, SDL_WINDOW_RESIZABLE);
    if (!window)
    {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    if (!headless)
    {
        if (cfg.winX >= 0 && cfg.winY >= 0)
            SDL_SetWindowPosition(window, cfg.winX, cfg.winY);
        if (cfg.maximized)
            SDL_MaximizeWindow(window);
    }

    const char* rendererDriver = headless ? "software" : nullptr;
    SDL_Renderer* renderer = SDL_CreateRenderer(window, rendererDriver);
    if (!renderer)
    {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = getConfigIniPath();
    io.DisplaySize = ImVec2((float)cfg.winW, (float)cfg.winH);

    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);
    loadFont();

    Poseidon::StudioApp app;
    app.setupTheme();
    app.headless = headless;
    app.openFilePath = openFile;
    app.monoFont = monoFont;
    app.sidebarRatio = std::clamp(cfg.sidebarPct, 10, 80) / 100.0f;
    app.init(gamePath, renderer);

    if (!dragDropPath.empty() && std::filesystem::is_regular_file(dragDropPath))
        app.selectSingleFile(std::filesystem::absolute(dragDropPath).string());

    bool running = true;
    int frameCount = 0;
    const int headlessFrames = 3;

    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
                running = false;
            if (event.type == SDL_EVENT_KEY_DOWN && event.key.scancode == SDL_SCANCODE_ESCAPE)
                running = false;
        }

        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        app.render(io.DisplaySize);
        ImGui::Render();

        SDL_SetRenderDrawColor(renderer, 25, 25, 28, 255);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
        frameCount++;

        if (headless && frameCount >= headlessFrames)
        {
            if (!screenshotFile.empty())
                saveScreenshot(renderer, screenshotFile.c_str());
            running = false;
        }
    }

    if (!headless)
    {
        SDL_GetWindowPosition(window, &cfg.winX, &cfg.winY);
        SDL_GetWindowSize(window, &cfg.winW, &cfg.winH);
        cfg.maximized = (SDL_GetWindowFlags(window) & SDL_WINDOW_MAXIMIZED) ? 1 : 0;
        cfg.sidebarPct = (int)(app.sidebarRatio * 100.0f);
        cfg.save();
    }

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(renderer);
    SDL_HideWindow(window);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
