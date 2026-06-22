#include <Poseidon/Foundation/Common/Win.h>

#include <Poseidon/Dev/Debug/DebugWin.hpp>
#include <Poseidon/Dev/Debug/DebugWinImpl.hpp>

#define WIN_STYLE (WS_OVERLAPPEDWINDOW)
#define WIN_EX_STYLE 0

extern HINSTANCE GHInstance;
#include <Poseidon/Core/Game/GameLoop.hpp>

struct RefWindow
{
    HWND _hwnd;
    Ref<Poseidon::Dev::DebugWindow> _window;
    Poseidon::Dev::DebugWindow* operator->() const { return _window; }
    operator Poseidon::Dev::DebugWindow*() const { return _window; }

    RefWindow() = default;
    RefWindow(Poseidon::Dev::DebugWindow* window, HWND hwnd) : _hwnd(hwnd), _window(window) {}
};

namespace Poseidon::Dev
{

static AutoArray<RefWindow> GDebugWindows;

HWND GetWindowHandle(DebugWindow* window)
{
    for (int i = 0; i < GDebugWindows.Size(); i++)
    {
        const RefWindow& rwnd = GDebugWindows[i];
        if (rwnd._window.GetRef() == window)
        {
            return rwnd._hwnd;
        }
    }
    return nullptr;
}

DebugWindow::DebugWindow(const char* title)
{
    HWND hwnd = CreateWindowEx(WIN_EX_STYLE,
                               "DEBUG WINDOW",                          // Class name
                               title,                                   // Caption
                               WIN_STYLE, CW_USEDEFAULT, CW_USEDEFAULT, // Position
                               CW_USEDEFAULT, CW_USEDEFAULT,            // Size
                               nullptr,                                 // Parent window (no parent)
                               nullptr,                                 // use class menu
                               GHInstance,                              // handle to window instance
                               nullptr                                  // no params to pass on
    );
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    GDebugWindows.Add(RefWindow(this, hwnd));
}

DebugWindow::~DebugWindow() = default;

void DebugWindow::Update()
{
    HWND hwnd = GetWindowHandle(this);
    RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_NOERASE | RDW_UPDATENOW);
}

void DebugWindow::Close()
{
    HWND hwnd = GetWindowHandle(this);
    DestroyWindow(hwnd);
}

void WaitForClose(DebugWindow* win)
{
    Link<DebugWindow> link = win;
    while (link)
    {
        Poseidon::ProcessMessagesNoWait();
        Sleep(100);
    }
}

bool DebugWindow::IsOpen()
{
    return GetWindowHandle(this) != nullptr;
}

LONG CALLBACK DebugWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_ERASEBKGND:
            return TRUE;
        case WM_PAINT:
            for (int i = 0; i < GDebugWindows.Size(); i++)
            {
                const RefWindow& rwnd = GDebugWindows[i];
                if (rwnd._hwnd == hwnd)
                {
                    PAINTSTRUCT paintStruct;
                    OnPaintContext pc;
                    pc.dc = BeginPaint(hwnd, &paintStruct);
                    rwnd->OnPaint(pc);
                    EndPaint(hwnd, &paintStruct);
                    break;
                }
            }
            return TRUE;
        case WM_DESTROY:
            for (int i = 0; i < GDebugWindows.Size(); i++)
            {
                const RefWindow& rwnd = GDebugWindows[i];
                if (rwnd._hwnd == hwnd)
                {
                    GDebugWindows.Delete(i);
                    break;
                }
            }
            return TRUE;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

DebugMemWindow::DebugMemWindow(const char* title, int w, int h) : base(title)
{
    _w = w;
    _h = h;
    _data.Realloc(h * w);

    HWND hwnd = GetWindowHandle(this);

    RECT rect;
    GetWindowRect(hwnd, &rect);
    SetWindowPos(hwnd, HWND_TOP, rect.left, rect.top, w, h, 0);
}

DebugMemWindow::~DebugMemWindow() = default;

void DebugMemWindow::OnPaint(const OnPaintContext& pc)
{
    HDC dc = pc.dc;
    BITMAPINFOHEADER info;
    info.biSize = sizeof(info);
    info.biWidth = _w;
    info.biHeight = _h;
    info.biPlanes = 1;
    info.biBitCount = 16;
    info.biCompression = BI_RGB;
    info.biSizeImage = _w * _h * sizeof(WORD);
    info.biXPelsPerMeter = 5000;
    info.biYPelsPerMeter = 5000;
    info.biClrUsed = 0;
    info.biClrImportant = 0;

    // create bitmap object from prepared data
    HBITMAP hbmp = ::CreateDIBitmap(dc, &info, CBM_INIT, _data.Data(), (BITMAPINFO*)&info, DIB_RGB_COLORS);
    if (hbmp)
    {
        // copy bitmap into the canvas
        HDC hdcBits = ::CreateCompatibleDC(dc);
        if (hdcBits)
        {
            HGDIOBJ prevObj = ::SelectObject(hdcBits, hbmp);
            if (!prevObj)
                Fail("SelectObject failed");
            BOOL success = ::BitBlt(dc, 0, 0, _w, _h, hdcBits, 0, 0, SRCCOPY);
            if (!success)
                Fail("BitBlt failed");
            ::SelectObject(hdcBits, prevObj);
            ::DeleteDC(hdcBits);
        }
        else
        {
            Fail("CreateCompatibleDC failed");
        }
        ::DeleteObject(hbmp);
    }
    else
    {
        Fail("CreateDIBitmap failed");
    }
}

DebugPixel DebugMemWindow::Get(int x, int y) const
{
    int i = y * _w + x;
    return _data[i];
}

DebugPixel& DebugMemWindow::Set(int x, int y)
{
    int i = y * _w + x;
    return _data[i];
}

void DebugMemWindow::Plot(int x, int y, DebugPixel color)
{
    if (x >= 0 && y >= 0 && x < _w && y < _h)
    {
        Set(x, y) = color;
    }
}

void DebugMemWindow::Line(int x0, int y0, int x1, int y1, DebugPixel color)
{
    if (x0 < 0 && x1 < 0)
        return;
    if (x0 >= _w && x1 >= _w)
        return;
    if (y0 < 0 && y1 < 0)
        return;
    if (y0 >= _h && y1 >= _h)
        return;

    // too big lines are not drawn correctly
    const int maxClip = 15000;
    if (x0 < -maxClip || x0 > +maxClip)
        return;
    if (x1 < -maxClip || x1 > +maxClip)
        return;
    if (y0 < -maxClip || y0 > +maxClip)
        return;
    if (y1 < -maxClip || y1 > +maxClip)
        return;

    /* DDA */
    long x = x0;
    long y = y0;
    long dx = x1 - x, adx = dx;
    long dy = y1 - y, ady = dy;
    if (adx < 0)
        adx = -adx;
    if (ady < 0)
        ady = -ady;

    if (adx < ady)
    { // vertical line
        long ddx = (dx << 16) / ady;
        long ddy = dy < 0 ? -1 : +1;
        long vdd = dy < 0 ? -_w : +_w;
        DebugPixel* lineAddr = _data.Data() + y * _w;
        x <<= 16;
        x += 0x8000;
        while (ady-- >= 0)
        {
            int xg = x >> 16, yg = y;
            if (xg >= 0 && yg >= 0 && xg < _w && yg < _h)
                lineAddr[xg] = color;
            x += ddx, y += ddy;
            lineAddr += vdd;
        }
    }
    else if (adx > 0)
    { // horizontal line
        long ddy = (dy << 16) / adx;
        long ddx = dx < 0 ? -1 : +1;
        long oy = y;
        DebugPixel* lineAddr = _data.Data() + y * _w;
        long vdd = dy < 0 ? -_w : +_w;
        y <<= 16;
        y += 0x8000;
        while (adx-- >= 0)
        {
            long ay = y >> 16;
            if (ay != oy)
            {
                lineAddr += vdd, oy = ay;
            }
            int yg = ay, xg = x;
            if (xg >= 0 && yg >= 0 && xg < _w && yg < _h)
                lineAddr[xg] = color;
            x += ddx, y += ddy;
        }
    }
}

} // namespace Poseidon::Dev
