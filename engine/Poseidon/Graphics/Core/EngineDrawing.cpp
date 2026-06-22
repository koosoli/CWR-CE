#include <Poseidon/Graphics/Core/Engine.hpp>
#include <Poseidon/Graphics/Cursor/ICursorOverlay.hpp>
#include <Poseidon/World/World.hpp>
#include <Poseidon/Graphics/Rendering/Draw/Font.hpp>
#include <Poseidon/Graphics/Textures/TextureBank.hpp>
#include <Poseidon/Core/Global.hpp>
#include <Poseidon/World/Scene/Scene.hpp>
#include <Poseidon/IO/ParamFileExt.hpp>
#include <Poseidon/Foundation/Platform/AppConfig.hpp>
#include <Poseidon/Input/InputSubsystem.hpp>
#include <Poseidon/Foundation/Strings/Bstring.hpp>
#include <Poseidon/Foundation/Platform/AppFrameExt.hpp>
#include <SDL3/SDL.h>

#include <stdarg.h>

#include <Poseidon/Foundation/Strings/Mbcs.hpp>
#include <SDL3/SDL_scancode.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <Poseidon/Foundation/Containers/Array.hpp>
#include <Poseidon/Foundation/Framework/AppFrame.hpp>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Poseidon/Foundation/Types/Memtype.h>
#include <Poseidon/Foundation/Types/Pointers.hpp>
#include <Poseidon/Foundation/platform.hpp>

using namespace Poseidon;
namespace Poseidon
{

void Engine::DrawFinishTexts()
{
#ifndef ACCESS_ONLY
    if (ShowFps() >= 1)
    {
        // TahomaB24 is hinted for screen at small sizes (the bigger B36
        // variant rasterised crisp but the readout took up ~half the
        // screen at 11 lines).  size=0.025 → 15-px tall; integer-pixel
        // shadow keeps it sharp.
        Ref<Font> font = LoadFont(GetFontID("TahomaB24"));
        PoseidonAssert(font);
        const float size = 0.025f;

        float fps = (_lastFrameDuration > 0) ? 1000.0f / _lastFrameDuration : 1000.0f;
        float afps = 1000.0f / GetAvgFrameDuration();
        float aLod = -10 * log10f(GScene->GetLodInvWidth());

        const float margin = 0.01f;
        const PackedColor color(Color(1.0f, 0.9f, 0.8f, 1.0f));
        float y = margin;
        // Integer-pixel shadow offset: sub-pixel offsets land both copies
        // on the same texel and the alpha-blend just hazes the glyph
        // edges instead of producing a clean drop-shadow.
        const float xso = 1.0f / Width2D();
        const float yso = 1.0f / Height2D();

        auto drawRight = [&](float yPos, const char* fmt, double val)
        {
            char buf[64];
            snprintf(buf, sizeof(buf), fmt, val);
            float w = GetTextWidth(size, font, buf);
            float x = 1.0f - margin - w;
            DrawTextF(Point2DFloat(x + xso, yPos + yso), size, font, PackedBlack, "%s", buf);
            DrawTextF(Point2DFloat(x, yPos), size, font, color, "%s", buf);
        };

        drawRight(y, "iFPS %7.2f", fps);
        y += size;
        drawRight(y, "aFPS %7.2f", afps);
        y += size;
        drawRight(y, "aLOD %7d", (int)aLod);
        y += size;
    }

    // Viewer-mode help overlay (FreeType monospaced — see --fps comment
    // above). `?` toggles visibility; --no-help starts hidden. When
    // hidden we draw the "Press ? for help" hint so the keymap is
    // discoverable.  Cursor itself is rendered by ICursorOverlay
    // wired in World::Init — not here.  This block stays gated on
    // IsViewerMode() because the help overlay text is genuinely
    // viewer-specific (game mode has no equivalent help screen).
    if (AppConfig::Instance().IsViewerMode() && GLOB_ENGINE && GLOB_ENGINE->TextBank())
    {
        static bool helpVisible = !AppConfig::Instance().ViewerNoHelp();
        static bool toggleLatched = false;
        auto& input = InputSubsystem::Instance();
        // F1 is the universal toggle (works on every keyboard layout).
        // Shift+/ is the secondary "?" combo — on US ANSI it produces `?`,
        // but on Czech / DE / FR layouts SDL_SCANCODE_SLASH is a different
        // physical key (`-` on Czech) so the combo silently does nothing.  Kept
        // for US users who reach for `?` first.
        bool toggleDown = input.IsKeyDown(SDL_SCANCODE_F1) ||
                          (input.IsKeyDown(SDL_SCANCODE_SLASH) &&
                           (input.IsKeyDown(SDL_SCANCODE_LSHIFT) || input.IsKeyDown(SDL_SCANCODE_RSHIFT)));
        if (toggleDown && !toggleLatched)
            helpVisible = !helpVisible;
        toggleLatched = toggleDown;

        // Same TahomaB24 as the FPS branch above.
        Ref<Font> font = LoadFont(GetFontID("TahomaB24"));
        PoseidonAssert(font);
        const float size = 0.025f;
        const float xso = 1.0f / Width2D();
        const float yso = 1.0f / Height2D();
        const PackedColor fg(Color(0.85f, 0.95f, 1.0f, 1.0f));

        auto drawAt = [&](float x, float y, const char* text)
        {
            DrawTextF(Point2DFloat(x + xso, y + yso), size, font, PackedBlack, "%s", text);
            DrawTextF(Point2DFloat(x, y), size, font, fg, "%s", text);
        };

        if (helpVisible)
        {
            // Read lock state from the active cursor overlay (one
            // virtual call, no globals).  GWorld owns the overlay;
            // viewer-mode init installs a ViewerCursorOverlay.
            const ICursorOverlay* cur = ::GWorld ? ::GWorld->GetCursorOverlay() : nullptr;
            const bool locked = cur && cur->IsLocked();
            char lockLine[64];
            snprintf(lockLine, sizeof(lockLine), "L           cursor lock [%s]", locked ? "locked" : "unlocked");

            const char* head[] = {
                "Viewer mode",
                "LMB drag    move object",
                "RMB drag    rotate object",
                "MMB drag    pan camera",
                "Wheel       zoom",
                "Space       play / pause anim",
                "[ / ]       scrub anim",
                "R           reset anim",
                "O           open anim (end)",
                "F5          reload textures",
                "F6          reset viewer state",
            };
            const char* tail[] = {
                "Esc         exit viewer",
                "F1 / ?      toggle this help",
            };
            float y = 0.01f;
            for (const char* line : head)
            {
                drawAt(0.01f, y, line);
                y += size;
            }
            drawAt(0.01f, y, lockLine);
            y += size;
            for (const char* line : tail)
            {
                drawAt(0.01f, y, line);
                y += size;
            }
        }
        else
        {
            drawAt(0.01f, 0.01f, "Press F1 for help");
        }

        // Cursor itself is rendered by GWorld->_cursorOverlay (set up
        // in World::Init for viewer mode); it draws inside the world's
        // render pass, not here.
    }

    // draw all ShowText buffers
    int i;
    PackedColor black(Color(0, 0, 0, 0.8));
    MipInfo mip = GLOB_ENGINE->TextBank()->UseMipmap(nullptr, 0, 0);
    int w = Width2D(), h = Height2D();
    for (i = 0; i < _texts.Size();)
    {
        TextInfo& info = _texts[i];
        float width = GetTextWidth(info._size, info._font, info._text);
        GEngine->Draw2D(mip, black, Rect2DPixel(info._x * w, info._y * h, width * w, info._font->Height() * h));
        DrawText(Point2DFloat(info._x, info._y), info._size, info._font, PackedWhite, info._text);
        if ((int)(_frameTime - info._hideTime) >= 0)
        {
            _texts.Delete(i);
        }
        else
        {
            i++;
        }
    }
#endif
}

void CCALL Engine::ShowMessage(int timeMs, const char* msg, ...)
{
    BString<512> message;
    va_list va;
    va_start(va, msg);
    vsprintf(message, msg, va);
    va_end(va);
    if (_messageHandle >= 0)
    {
        RemoveText(_messageHandle);
    }
    _messageHandle = ShowText(timeMs, 25, 2, message);
}

void CWRFrameFunctions::ShowMessage(int timeMs, const char* msg, va_list argptr)
{
    if (!GEngine)
    {
        return;
    }

    BString<512> message;
    vsprintf(message, msg, argptr);
    GEngine->ShowMessage(timeMs, message);
}

TextInfo::TextInfo(int handle, Engine* engine, DWORD hideTime, Font* font, PackedColor color, float size, float x,
                   float y, const char* text)
    : _hideTime(hideTime), _font(font), _color(color), _x(x), _y(y), _size(size), _text(text, strlen(text) + 1),
      _handle(handle)
{
}

TextInfo::TextInfo(const TextInfo& src)
    : _hideTime(src._hideTime), _font(src._font), _color(src._color), _x(src._x), _y(src._y), _size(src._size),
      _text(src._text, strlen(src._text) + 1), _handle(src._handle)
{
}
TextInfo& TextInfo::operator=(const TextInfo& src)
{
    _hideTime = src._hideTime, _font = src._font, _color = src._color;
    _x = src._x, _y = src._y;
    _size = src._size;
    if (src._text)
    {
        _text.Realloc(src._text, strlen(src._text) + 1);
    }
    else
    {
        _text.Free();
    }
    _handle = src._handle;
    return *this;
}

void Engine::ShowFont(Font* font, PackedColor color, float size)
{
    _showTextFont = font;
    _showTextColor = color;
    _showTextSize = size;
}

void Engine::RemoveText(int handle)
{
    if (handle >= 0)
    {
        for (int i = 0; i < _texts.Size(); i++)
        {
            if (_texts[i]._handle == handle)
            {
                _texts.Delete(i);
                return;
            }
        }
        // text may already be expired
    }
}

int Engine::ShowText(DWORD timeToLive, int x, int y, const char* text)
{
#ifndef ACCESS_ONLY
    if (!_showTextFont)
    {
        _showTextFont = LoadFont(GetFontID("tahomaB36"));
        _showTextSize = 0.021;
    }
    // x,y given for 800x600 screen
    int handle = _textHandle++;
    _texts.Add(TextInfo(handle, this, Poseidon::Foundation::GlobalTickCount() + timeToLive, _showTextFont,
                        _showTextColor, _showTextSize, x * (1.0 / 800), y * (1.0 / 600), text));
    return handle;
#else
    return 0;
#endif
}

int CCALL Engine::ShowTextF(DWORD timeToLive, int x, int y, const char* format, ...)
{
    BString<512> buf;
    va_list arglist;
    va_start(arglist, format);
    vsprintf(buf, format, arglist);
    va_end(arglist);
    return ShowText(timeToLive, x, y, buf);
}

} // namespace Poseidon
