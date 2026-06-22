#include <Poseidon/IO/Streams/QStream.hpp>
#include <Poseidon/Foundation/Common/Win.h>
#include <Poseidon/Core/Application.hpp>
#include <SDL3/SDL.h>
#include <Poseidon/Foundation/Framework/Log.hpp>
#include <SDL3/SDL_clipboard.h>
#include <SDL3/SDL_error.h>
#include <string>

// At global scope so its RTTI name stays unmangled.
static class WindowsClipboardFunctions : public Poseidon::ClipboardFunctions
{
  public:
    void Copy(const char* buffer, int size) override;
} GWindowsClipboardFunctions;

void WindowsClipboardFunctions::Copy(const char* buffer, int size)
{
    std::string text(buffer, size);
    if (!SDL_SetClipboardText(text.c_str()))
    {
        LOG_WARN(Core, "SDL_SetClipboardText failed: {}", SDL_GetError());
    }
}

void InitClipboardFunctions()
{
    Poseidon::QOFStream::SetDefaultClipboardFunctions(&GWindowsClipboardFunctions);
}
