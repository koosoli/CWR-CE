#include <Poseidon/Foundation/Platform/InitBridge.hpp>
#include <ctype.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <exception>
#include <Poseidon/Foundation/Types/Memtype.h>
#include <Poseidon/Foundation/platform.hpp>

// WLX API window handles are pointers on Linux hosts.
#ifndef _WIN32
#define WLX_HWND intptr_t
#else
#define WLX_HWND HWND
#endif

#include "listplug.h"
#include <Poseidon/Foundation/Framework/AppFrame.hpp>
#include <Poseidon/IO/ParamFile/InitLibraryElement.hpp>
#include <Poseidon/Foundation/Platform/PoseidonInit.hpp>
#include <Poseidon/Graphics/Core/Engine.hpp>
#include <Poseidon/Graphics/Dummy/GraphicsEngineDummy.hpp>
#include <Poseidon/Graphics/Textures/PAADecoder.hpp>
#include <Poseidon/Graphics/Textures/ModelRenderer.hpp>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdio>

#ifndef _WIN32
#include <dlfcn.h>
#include <unistd.h>
#endif

static FILE* g_logFile = nullptr;

#ifndef _WIN32
static void ListerLogInit()
{
    if (g_logFile)
        return;
    Dl_info info;
    if (dladdr((void*)&ListerLogInit, &info) && info.dli_fname)
    {
        std::string logPath(info.dli_fname);
        auto pos = logPath.find_last_of('/');
        if (pos != std::string::npos)
            logPath = logPath.substr(0, pos + 1);
        logPath += "poseidon_lister.log";
        g_logFile = fopen(logPath.c_str(), "w");
    }
    if (!g_logFile)
        g_logFile = fopen("/tmp/poseidon_lister.log", "w");
}
#else
static void ListerLogInit()
{
    if (g_logFile)
        return;
    char path[MAX_PATH];
    HMODULE hMod = nullptr;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCSTR)&ListerLogInit, &hMod);
    GetModuleFileNameA(hMod, path, MAX_PATH);
    std::string logPath(path);
    auto pos = logPath.find_last_of("\\/");
    if (pos != std::string::npos)
        logPath = logPath.substr(0, pos + 1);
    logPath += "poseidon_lister.log";
    g_logFile = fopen(logPath.c_str(), "w");
}
#endif

static void ListerLog(const char* fmt, ...)
{
    if (!g_logFile)
        ListerLogInit();
    if (!g_logFile)
        return;
    va_list ap;
    va_start(ap, fmt);
    vfprintf(g_logFile, fmt, ap);
    va_end(ap);
    fprintf(g_logFile, "\n");
    fflush(g_logFile);
}

class TcListerAppFrame : public AppFrameFunctions
{
  public:
    void ErrorMessage(ErrorMessageLevel, const char*, va_list) override {}
    void ErrorMessage(const char*, va_list) override {}
    void WarningMessage(const char*, va_list) override {}
    void ShowMessage(int, const char*, va_list) override {}
    DWORD TickCount() override { return GetTickCount(); }
};

static TcListerAppFrame g_appFrame;
static bool g_engineInitialized = false;

static void EnsureEngineInit()
{
    if (g_engineInitialized)
        return;
    ListerLog("EnsureEngineInit: starting");
    CurrentAppFrameFunctions = &g_appFrame;
    SetMemorySystemReady(true);
    InitLibraryElement();
    Poseidon::InitDefaults();
    Poseidon::GEngine = Poseidon::CreateEngineDummy();
    Poseidon::SetModelRendererLog([](const char* msg) { ListerLog("  [MR] %s", msg); });
    g_engineInitialized = true;
    ListerLog("EnsureEngineInit: done");
}

static std::string GetExtension(const std::string& path)
{
    auto dot = path.find_last_of('.');
    if (dot == std::string::npos)
        return {};
    std::string ext = path.substr(dot);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext;
}

static bool IsConfigFile(const std::string& ext)
{
    return ext == ".bin" || ext == ".cpp" || ext == ".hpp";
}

static bool IsSupportedExtension(const std::string& path)
{
    auto ext = GetExtension(path);
    return ext == ".paa" || ext == ".pac" || ext == ".p3d" || IsConfigFile(ext);
}

static bool IsTextureFile(const std::string& path)
{
    auto ext = GetExtension(path);
    return ext == ".paa" || ext == ".pac";
}

static bool IsModelFile(const std::string& path)
{
    auto ext = GetExtension(path);
    return ext == ".p3d";
}

#include <Poseidon/IO/ParamFile/ParamFile.hpp>

static void ParamClassToText(const ParamClass& cls, std::string& out, int indent)
{
    std::string pad(indent * 4, ' ');
    for (int i = 0; i < cls.GetEntryCount(); i++)
    {
        const ParamEntry& e = cls.GetEntry(i);
        std::string name = e.GetName().Data();
        if (e.IsClass())
        {
            const ParamClass* sub = e.GetClassInterface();
            const char* base = sub ? sub->GetBaseName() : nullptr;
            out += pad + name;
            if (base)
            {
                out += " : ";
                out += base;
            }
            out += "\r\n" + pad + "{\r\n";
            if (sub)
                ParamClassToText(*sub, out, indent + 1);
            out += pad + "};\r\n";
        }
        else if (e.IsArray())
        {
            out += pad + name + "[] = {};\r\n";
        }
        else if (e.IsFloatValue())
        {
            char buf[64];
            snprintf(buf, sizeof(buf), "%g", (float)e);
            out += pad + name + " = " + buf + ";\r\n";
        }
        else if (e.IsIntValue())
        {
            char buf[32];
            snprintf(buf, sizeof(buf), "%d", (int)e);
            out += pad + name + " = " + buf + ";\r\n";
        }
        else
        {
            out += pad + name + " = \"" + e.GetValue().Data() + "\";\r\n";
        }
    }
}

static std::string DecodeParamFileToText(const std::string& path, const std::string& ext)
{
    ParamFile pf;
    bool ok = false;
    if (ext == ".bin")
        ok = pf.ParseBin(path.c_str());
    else
        ok = pf.ParseBin(path.c_str()) || pf.Parse(path.c_str()) == LSOK;
    if (!ok)
        return {};
    std::string text;
    ParamClassToText(pf, text, 0);
    return text;
}

struct ListerData
{
    std::vector<uint8_t> pixels; // BGRA format, 4 bytes per pixel
    int imgW = 0;
    int imgH = 0;
};

static ListerData* DecodeFile(const std::string& path, int parentW, int parentH)
{
    auto* data = new ListerData();

    if (IsTextureFile(path))
    {
        ListerLog("DecodeFile: decoding texture '%s'", path.c_str());
        auto img = Poseidon::DecodePAAFile(path);
        if (!img.valid())
        {
            ListerLog("DecodeFile: texture decode failed");
            delete data;
            return nullptr;
        }
        data->imgW = img.width;
        data->imgH = img.height;
        data->pixels.resize(img.width * img.height * 4);
        // Convert RGBA → BGRA
        for (int i = 0; i < img.width * img.height; i++)
        {
            data->pixels[i * 4 + 0] = img.rgba[i * 4 + 2]; // B
            data->pixels[i * 4 + 1] = img.rgba[i * 4 + 1]; // G
            data->pixels[i * 4 + 2] = img.rgba[i * 4 + 0]; // R
            data->pixels[i * 4 + 3] = img.rgba[i * 4 + 3]; // A
        }
        ListerLog("DecodeFile: texture %dx%d decoded", data->imgW, data->imgH);
    }
    else if (IsModelFile(path))
    {
        ListerLog("DecodeFile: loading P3D model '%s'", path.c_str());
        int sz = (parentW < parentH) ? parentW : parentH;
        if (sz < 256)
            sz = 800;
        auto model = Poseidon::RenderP3DFile(path, sz, sz, "quad", 0, 24, 24, 24);
        if (!model.valid())
        {
            ListerLog("DecodeFile: P3D render failed");
            delete data;
            return nullptr;
        }
        data->imgW = model.width;
        data->imgH = model.height;
        data->pixels.resize(model.width * model.height * 4);
        // Convert RGB → BGRA
        for (int i = 0; i < model.width * model.height; i++)
        {
            data->pixels[i * 4 + 0] = model.rgb[i * 3 + 2]; // B
            data->pixels[i * 4 + 1] = model.rgb[i * 3 + 1]; // G
            data->pixels[i * 4 + 2] = model.rgb[i * 3 + 0]; // R
            data->pixels[i * 4 + 3] = 255;                  // A
        }
        ListerLog("DecodeFile: P3D rendered %dx%d", data->imgW, data->imgH);
    }
    else
    {
        delete data;
        return nullptr;
    }

    return data;
}

#ifdef _WIN32
static const wchar_t* WND_CLASS = L"PoseidonLister";
static bool g_classRegistered = false;

static void RegisterListerClass();
static LRESULT CALLBACK ListerWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

static std::string WideToUtf8(const wchar_t* wide)
{
    if (!wide)
        return {};
    int len = WideCharToMultiByte(CP_UTF8, 0, wide, -1, nullptr, 0, nullptr, nullptr);
    if (len <= 0)
        return {};
    std::string s(len - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide, -1, s.data(), len, nullptr, nullptr);
    return s;
}

static HWND LoadConfigWindow(HWND parentWin, const std::string& path)
{
    EnsureEngineInit();
    auto ext = GetExtension(path);
    ListerLog("LoadConfigWindow: '%s'", path.c_str());
    std::string text = DecodeParamFileToText(path, ext);
    if (text.empty())
    {
        ListerLog("LoadConfigWindow: parse failed");
        return nullptr;
    }
    int wlen = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    std::wstring wtext(wlen - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, wtext.data(), wlen);

    RECT rc;
    GetClientRect(parentWin, &rc);
    HWND edit = CreateWindowExW(0, L"EDIT", L"",
                                WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_READONLY |
                                    ES_AUTOVSCROLL | ES_AUTOHSCROLL,
                                0, 0, rc.right, rc.bottom, parentWin, nullptr, GetModuleHandle(nullptr), nullptr);
    if (!edit)
        return nullptr;
    SetWindowTextW(edit, wtext.c_str());
    ListerLog("LoadConfigWindow: success, %zu chars", text.size());
    return edit;
}

static HWND LoadFile(HWND parentWin, const std::string& path, int showFlags)
{
    ListerLog("LoadFile: path='%s' showFlags=%d", path.c_str(), showFlags);
    if (!IsSupportedExtension(path))
        return nullptr;

    if (IsConfigFile(GetExtension(path)))
        return LoadConfigWindow(parentWin, path);

    EnsureEngineInit();
    RECT rc;
    GetClientRect(parentWin, &rc);
    auto* data = DecodeFile(path, rc.right, rc.bottom);
    if (!data)
        return nullptr;

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = data->imgW;
    bmi.bmiHeader.biHeight = -data->imgH;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HBITMAP hBmp = CreateDIBSection(nullptr, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!hBmp || !bits)
    {
        delete data;
        return nullptr;
    }
    memcpy(bits, data->pixels.data(), data->pixels.size());

    RegisterListerClass();
    GetClientRect(parentWin, &rc);

    HWND hwnd = CreateWindowExW(0, WND_CLASS, L"", WS_CHILD | WS_VISIBLE, 0, 0, rc.right, rc.bottom, parentWin, nullptr,
                                GetModuleHandle(nullptr), nullptr);
    if (!hwnd)
    {
        DeleteObject(hBmp);
        delete data;
        return nullptr;
    }

    struct WinListerData
    {
        HBITMAP hBitmap;
        int imgW, imgH;
    };
    auto* wdata = new WinListerData{hBmp, data->imgW, data->imgH};
    SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(wdata));
    delete data;

    ListerLog("LoadFile: success, hwnd=%p", hwnd);
    return hwnd;
}

static LRESULT CALLBACK ListerWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    struct WinListerData
    {
        HBITMAP hBitmap;
        int imgW, imgH;
    };

    switch (msg)
    {
        case WM_PAINT:
        {
            auto* data = reinterpret_cast<WinListerData*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            RECT rc;
            GetClientRect(hwnd, &rc);
            int winW = rc.right, winH = rc.bottom;

            HBRUSH bg = CreateSolidBrush(RGB(255, 255, 255));
            FillRect(hdc, &rc, bg);
            DeleteObject(bg);

            if (data && data->hBitmap)
            {
                float scaleX = (float)winW / data->imgW;
                float scaleY = (float)winH / data->imgH;
                float scale = (scaleX < scaleY) ? scaleX : scaleY;
                if (scale > 1.0f)
                    scale = 1.0f;
                int dstW = (int)(data->imgW * scale);
                int dstH = (int)(data->imgH * scale);
                int dstX = (winW - dstW) / 2;
                int dstY = (winH - dstH) / 2;

                HDC memDC = CreateCompatibleDC(hdc);
                HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, data->hBitmap);
                SetStretchBltMode(hdc, HALFTONE);
                StretchBlt(hdc, dstX, dstY, dstW, dstH, memDC, 0, 0, data->imgW, data->imgH, SRCCOPY);
                SelectObject(memDC, oldBmp);
                DeleteDC(memDC);
            }

            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_SIZE:
            InvalidateRect(hwnd, nullptr, TRUE);
            return 0;
        case WM_ERASEBKGND:
            return 1;
        case WM_DESTROY:
        {
            auto* data = reinterpret_cast<WinListerData*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
            if (data)
            {
                if (data->hBitmap)
                    DeleteObject(data->hBitmap);
                delete data;
                SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
            }
            return 0;
        }
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

static int LoadNextFile(HWND pluginWin, const std::string& path, int showFlags)
{
    if (!IsSupportedExtension(path))
        return LISTPLUGIN_ERROR;
    // Config files need a different window type, so let the host recreate the view.
    if (IsConfigFile(GetExtension(path)))
        return LISTPLUGIN_ERROR;

    struct WinListerData
    {
        HBITMAP hBitmap;
        int imgW, imgH;
    };

    RECT rc;
    GetClientRect(pluginWin, &rc);
    auto* data = DecodeFile(path, rc.right, rc.bottom);
    if (!data)
        return LISTPLUGIN_ERROR;

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = data->imgW;
    bmi.bmiHeader.biHeight = -data->imgH;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HBITMAP hBmp = CreateDIBSection(nullptr, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!hBmp || !bits)
    {
        delete data;
        return LISTPLUGIN_ERROR;
    }
    memcpy(bits, data->pixels.data(), data->pixels.size());

    auto* old = reinterpret_cast<WinListerData*>(GetWindowLongPtrW(pluginWin, GWLP_USERDATA));
    if (old)
    {
        if (old->hBitmap)
            DeleteObject(old->hBitmap);
        delete old;
    }

    auto* wdata = new WinListerData{hBmp, data->imgW, data->imgH};
    SetWindowLongPtrW(pluginWin, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(wdata));
    delete data;

    InvalidateRect(pluginWin, nullptr, TRUE);
    return LISTPLUGIN_OK;
}

static void RegisterListerClass()
{
    if (g_classRegistered)
        return;
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = ListerWndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = WND_CLASS;
    RegisterClassExW(&wc);
    g_classRegistered = true;
}

#else
// Double Commander uses GTK2. Resolve the symbols at runtime to avoid a compile-time dependency.

typedef struct _GtkWidget GtkWidget;
typedef struct _GdkEventExpose GdkEventExpose;
typedef struct _GdkWindow GdkWindow;
typedef struct _cairo cairo_t;
typedef struct _cairo_surface cairo_surface_t;

enum
{
    CAIRO_FORMAT_ARGB32 = 0
};

typedef GtkWidget* (*pf_gtk_drawing_area_new)(void);
typedef void (*pf_gtk_widget_show)(GtkWidget*);
typedef void (*pf_gtk_widget_destroy)(GtkWidget*);
typedef void (*pf_gtk_widget_set_size_request)(GtkWidget*, int, int);
typedef void (*pf_gtk_widget_queue_draw)(GtkWidget*);
typedef GdkWindow* (*pf_gtk_widget_get_window)(GtkWidget*);
typedef void (*pf_gtk_container_add)(GtkWidget*, GtkWidget*);
typedef unsigned long (*pf_g_signal_connect_data)(void*, const char*, void*, void*, void*, int);
typedef void (*pf_g_object_set_data)(void*, const char*, void*);
typedef void* (*pf_g_object_get_data)(void*, const char*);
typedef cairo_t* (*pf_gdk_cairo_create)(GdkWindow*);
typedef void (*pf_gtk_widget_get_allocation)(GtkWidget*, void*);

typedef cairo_surface_t* (*pf_cairo_image_surface_create_for_data)(unsigned char*, int, int, int, int);
typedef void (*pf_cairo_set_source_surface)(cairo_t*, cairo_surface_t*, double, double);
typedef void (*pf_cairo_paint)(cairo_t*);
typedef void (*pf_cairo_destroy)(cairo_t*);
typedef void (*pf_cairo_surface_destroy)(cairo_surface_t*);
typedef void (*pf_cairo_scale)(cairo_t*, double, double);
typedef void (*pf_cairo_set_source_rgb)(cairo_t*, double, double, double);
typedef void (*pf_cairo_rectangle)(cairo_t*, double, double, double, double);
typedef void (*pf_cairo_fill)(cairo_t*);
typedef void (*pf_cairo_translate)(cairo_t*, double, double);

struct GtkAllocation
{
    int x, y, width, height;
};

static struct GtkFuncs
{
    pf_gtk_drawing_area_new drawing_area_new;
    pf_gtk_widget_show widget_show;
    pf_gtk_widget_destroy widget_destroy;
    pf_gtk_widget_set_size_request widget_set_size_request;
    pf_gtk_widget_queue_draw widget_queue_draw;
    pf_gtk_widget_get_window widget_get_window;
    pf_gtk_container_add container_add;
    pf_g_signal_connect_data signal_connect_data;
    pf_g_object_set_data object_set_data;
    pf_g_object_get_data object_get_data;
    pf_gdk_cairo_create gdk_cairo_create;
    pf_gtk_widget_get_allocation widget_get_allocation;

    pf_cairo_image_surface_create_for_data cairo_image_surface_create_for_data;
    pf_cairo_set_source_surface cairo_set_source_surface;
    pf_cairo_paint cairo_paint;
    pf_cairo_destroy cairo_destroy;
    pf_cairo_surface_destroy cairo_surface_destroy;
    pf_cairo_scale cairo_scale;
    pf_cairo_set_source_rgb cairo_set_source_rgb;
    pf_cairo_rectangle cairo_rectangle;
    pf_cairo_fill cairo_fill;
    pf_cairo_translate cairo_translate;

    bool loaded;
} g_gtk = {};

static bool LoadGtkFunctions()
{
    if (g_gtk.loaded)
        return true;

    void* gtk = dlopen("libgtk-x11-2.0.so.0", RTLD_LAZY | RTLD_NOLOAD);
    void* gdk = dlopen("libgdk-x11-2.0.so.0", RTLD_LAZY | RTLD_NOLOAD);
    void* gobj = dlopen("libgobject-2.0.so.0", RTLD_LAZY | RTLD_NOLOAD);
    void* glib = dlopen("libglib-2.0.so.0", RTLD_LAZY | RTLD_NOLOAD);
    void* cairo = dlopen("libcairo.so.2", RTLD_LAZY | RTLD_NOLOAD);

    if (!gtk || !gobj || !cairo)
    {
        gtk = dlopen("libgtk-x11-2.0.so.0", RTLD_LAZY);
        gdk = dlopen("libgdk-x11-2.0.so.0", RTLD_LAZY);
        gobj = dlopen("libgobject-2.0.so.0", RTLD_LAZY);
        glib = dlopen("libglib-2.0.so.0", RTLD_LAZY);
        cairo = dlopen("libcairo.so.2", RTLD_LAZY);
    }

    if (!gtk || !gobj || !cairo)
    {
        ListerLog("LoadGtkFunctions: failed to load GTK2/Cairo (gtk=%p gobj=%p cairo=%p)", gtk, gobj, cairo);
        return false;
    }

    g_gtk.drawing_area_new = (pf_gtk_drawing_area_new)dlsym(gtk, "gtk_drawing_area_new");
    g_gtk.widget_show = (pf_gtk_widget_show)dlsym(gtk, "gtk_widget_show");
    g_gtk.widget_destroy = (pf_gtk_widget_destroy)dlsym(gtk, "gtk_widget_destroy");
    g_gtk.widget_set_size_request = (pf_gtk_widget_set_size_request)dlsym(gtk, "gtk_widget_set_size_request");
    g_gtk.widget_queue_draw = (pf_gtk_widget_queue_draw)dlsym(gtk, "gtk_widget_queue_draw");
    g_gtk.widget_get_window = (pf_gtk_widget_get_window)dlsym(gtk, "gtk_widget_get_window");
    g_gtk.container_add = (pf_gtk_container_add)dlsym(gtk, "gtk_container_add");
    g_gtk.widget_get_allocation = (pf_gtk_widget_get_allocation)dlsym(gtk, "gtk_widget_get_allocation");

    g_gtk.signal_connect_data = (pf_g_signal_connect_data)dlsym(gobj, "g_signal_connect_data");
    g_gtk.object_set_data = (pf_g_object_set_data)dlsym(gobj, "g_object_set_data");
    g_gtk.object_get_data = (pf_g_object_get_data)dlsym(gobj, "g_object_get_data");

    if (gdk)
        g_gtk.gdk_cairo_create = (pf_gdk_cairo_create)dlsym(gdk, "gdk_cairo_create");

    g_gtk.cairo_image_surface_create_for_data =
        (pf_cairo_image_surface_create_for_data)dlsym(cairo, "cairo_image_surface_create_for_data");
    g_gtk.cairo_set_source_surface = (pf_cairo_set_source_surface)dlsym(cairo, "cairo_set_source_surface");
    g_gtk.cairo_paint = (pf_cairo_paint)dlsym(cairo, "cairo_paint");
    g_gtk.cairo_destroy = (pf_cairo_destroy)dlsym(cairo, "cairo_destroy");
    g_gtk.cairo_surface_destroy = (pf_cairo_surface_destroy)dlsym(cairo, "cairo_surface_destroy");
    g_gtk.cairo_scale = (pf_cairo_scale)dlsym(cairo, "cairo_scale");
    g_gtk.cairo_set_source_rgb = (pf_cairo_set_source_rgb)dlsym(cairo, "cairo_set_source_rgb");
    g_gtk.cairo_rectangle = (pf_cairo_rectangle)dlsym(cairo, "cairo_rectangle");
    g_gtk.cairo_fill = (pf_cairo_fill)dlsym(cairo, "cairo_fill");
    g_gtk.cairo_translate = (pf_cairo_translate)dlsym(cairo, "cairo_translate");

    if (!g_gtk.drawing_area_new || !g_gtk.widget_show || !g_gtk.container_add || !g_gtk.signal_connect_data ||
        !g_gtk.gdk_cairo_create || !g_gtk.cairo_image_surface_create_for_data || !g_gtk.cairo_paint)
    {
        ListerLog("LoadGtkFunctions: missing critical symbols");
        return false;
    }

    g_gtk.loaded = true;
    ListerLog("LoadGtkFunctions: all symbols loaded successfully");
    return true;
}

static int OnExpose(GtkWidget* widget, GdkEventExpose* /*event*/, void* /*user_data*/)
{
    auto* data = (ListerData*)g_gtk.object_get_data(widget, "lister-data");
    if (!data || data->pixels.empty())
        return FALSE;

    GdkWindow* win = g_gtk.widget_get_window(widget);
    if (!win)
        return FALSE;

    cairo_t* cr = g_gtk.gdk_cairo_create(win);

    GtkAllocation alloc;
    g_gtk.widget_get_allocation(widget, &alloc);
    int winW = alloc.width, winH = alloc.height;

    g_gtk.cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    g_gtk.cairo_rectangle(cr, 0, 0, winW, winH);
    g_gtk.cairo_fill(cr);

    // Cairo ARGB32 is native-endian premultiplied ARGB, which is BGRA on little-endian x86.
    cairo_surface_t* surf = g_gtk.cairo_image_surface_create_for_data(data->pixels.data(), CAIRO_FORMAT_ARGB32,
                                                                      data->imgW, data->imgH, data->imgW * 4);

    float scaleX = (float)winW / data->imgW;
    float scaleY = (float)winH / data->imgH;
    float scale = (scaleX < scaleY) ? scaleX : scaleY;
    if (scale > 1.0f)
        scale = 1.0f;
    int dstW = (int)(data->imgW * scale);
    int dstH = (int)(data->imgH * scale);
    double dstX = (winW - dstW) / 2.0;
    double dstY = (winH - dstH) / 2.0;

    g_gtk.cairo_translate(cr, dstX, dstY);
    g_gtk.cairo_scale(cr, scale, scale);
    g_gtk.cairo_set_source_surface(cr, surf, 0, 0);
    g_gtk.cairo_paint(cr);

    g_gtk.cairo_surface_destroy(surf);
    g_gtk.cairo_destroy(cr);

    return TRUE;
}

static void OnDestroy(GtkWidget* widget, void* /*user_data*/)
{
    auto* data = (ListerData*)g_gtk.object_get_data(widget, "lister-data");
    if (data)
    {
        delete data;
        g_gtk.object_set_data(widget, "lister-data", nullptr);
    }
}

static WLX_HWND LoadFile(WLX_HWND parentWin, const std::string& path, int showFlags)
{
    ListerLog("LoadFile: path='%s' showFlags=%d parentWin=%p", path.c_str(), showFlags, (void*)parentWin);
    if (!IsSupportedExtension(path))
        return (WLX_HWND)0;

    if (!LoadGtkFunctions())
    {
        ListerLog("LoadFile: GTK2 not available");
        return (WLX_HWND)0;
    }

    EnsureEngineInit();

    auto* data = DecodeFile(path, 800, 800);
    if (!data)
        return (WLX_HWND)0;

    GtkWidget* drawArea = g_gtk.drawing_area_new();
    if (!drawArea)
    {
        ListerLog("LoadFile: gtk_drawing_area_new failed");
        delete data;
        return (WLX_HWND)0;
    }

    g_gtk.widget_set_size_request(drawArea, data->imgW, data->imgH);

    g_gtk.object_set_data(drawArea, "lister-data", data);

    g_gtk.signal_connect_data(drawArea, "expose-event", (void*)OnExpose, nullptr, nullptr, 0);
    g_gtk.signal_connect_data(drawArea, "destroy", (void*)OnDestroy, nullptr, nullptr, 0);

    GtkWidget* parent = (GtkWidget*)parentWin;
    g_gtk.container_add(parent, drawArea);
    g_gtk.widget_show(drawArea);

    ListerLog("LoadFile: success, widget=%p imgW=%d imgH=%d", drawArea, data->imgW, data->imgH);
    return (WLX_HWND)drawArea;
}

static int LoadNextFile(WLX_HWND pluginWin, const std::string& path, int showFlags)
{
    if (!IsSupportedExtension(path))
        return LISTPLUGIN_ERROR;

    GtkWidget* widget = (GtkWidget*)pluginWin;
    auto* oldData = (ListerData*)g_gtk.object_get_data(widget, "lister-data");

    auto* data = DecodeFile(path, 800, 800);
    if (!data)
        return LISTPLUGIN_ERROR;

    delete oldData;
    g_gtk.object_set_data(widget, "lister-data", data);

    g_gtk.widget_queue_draw(widget);
    return LISTPLUGIN_OK;
}

#endif // _WIN32 / Linux

extern "C"
{
    WLX_HWND __stdcall ListLoad(WLX_HWND ParentWin, char* FileToLoad, int ShowFlags)
    {
        ListerLog("ListLoad: '%s'", FileToLoad ? FileToLoad : "(null)");
        try
        {
            return LoadFile(ParentWin, FileToLoad ? FileToLoad : "", ShowFlags);
        }
        catch (const std::exception& e)
        {
            ListerLog("ListLoad EXCEPTION: %s", e.what());
            return (WLX_HWND)0;
        }
        catch (...)
        {
            ListerLog("ListLoad EXCEPTION: unknown");
            return (WLX_HWND)0;
        }
    }

#ifdef _WIN32
    HWND __stdcall ListLoadW(HWND ParentWin, WCHAR* FileToLoad, int ShowFlags)
    {
        auto path = WideToUtf8(FileToLoad);
        ListerLog("ListLoadW: '%s'", path.c_str());
        try
        {
            return LoadFile(ParentWin, path, ShowFlags);
        }
        catch (const std::exception& e)
        {
            ListerLog("ListLoadW EXCEPTION: %s", e.what());
            return nullptr;
        }
        catch (...)
        {
            ListerLog("ListLoadW EXCEPTION: unknown");
            return nullptr;
        }
    }

    int __stdcall ListLoadNext(HWND ParentWin, HWND PluginWin, char* FileToLoad, int ShowFlags)
    {
        ListerLog("ListLoadNext: '%s'", FileToLoad ? FileToLoad : "(null)");
        try
        {
            return LoadNextFile(PluginWin, FileToLoad ? FileToLoad : "", ShowFlags);
        }
        catch (const std::exception& e)
        {
            ListerLog("ListLoadNext EXCEPTION: %s", e.what());
            return LISTPLUGIN_ERROR;
        }
        catch (...)
        {
            ListerLog("ListLoadNext EXCEPTION: unknown");
            return LISTPLUGIN_ERROR;
        }
    }

    int __stdcall ListLoadNextW(HWND ParentWin, HWND PluginWin, WCHAR* FileToLoad, int ShowFlags)
    {
        auto path = WideToUtf8(FileToLoad);
        ListerLog("ListLoadNextW: '%s'", path.c_str());
        try
        {
            return LoadNextFile(PluginWin, path, ShowFlags);
        }
        catch (const std::exception& e)
        {
            ListerLog("ListLoadNextW EXCEPTION: %s", e.what());
            return LISTPLUGIN_ERROR;
        }
        catch (...)
        {
            ListerLog("ListLoadNextW EXCEPTION: unknown");
            return LISTPLUGIN_ERROR;
        }
    }
#endif

    void __stdcall ListCloseWindow(WLX_HWND ListWin)
    {
        ListerLog("ListCloseWindow");
#ifdef _WIN32
        DestroyWindow(ListWin);
#else
        if (g_gtk.loaded && g_gtk.widget_destroy)
        {
            GtkWidget* widget = (GtkWidget*)ListWin;
            g_gtk.widget_destroy(widget);
        }
#endif
    }

    void __stdcall ListGetDetectString(char* DetectString, int maxlen)
    {
        strncpy(DetectString, "EXT=\"PAA\" | EXT=\"PAC\" | EXT=\"P3D\"", maxlen - 1);
        DetectString[maxlen - 1] = '\0';
    }

    void __stdcall ListSetDefaultParams(ListDefaultParamStruct* dps)
    {
        (void)dps;
    }

} // extern "C"

#ifdef _WIN32
BOOL APIENTRY DllMain(HMODULE, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
        ListerLogInit();
    return TRUE;
}
#else
__attribute__((constructor)) static void PluginInit()
{
    ListerLogInit();
}
#endif
