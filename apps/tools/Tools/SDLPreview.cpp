#include "SDLPreview.hpp"
#include <CLI/CLI.hpp>
#include <iostream>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>
#include <CLI/Error.hpp>
#include <utility>

namespace PoseidonTools
{

static void runEventLoop(SDL_Window* window, SDL_Renderer* renderer, SDL_Texture*& tex, int& imgW, int& imgH,
                         SDL_PixelFormat pixelFormat, int bytesPerPixel, std::vector<uint8_t>& pixelsCopy,
                         ResizeCallback onResize)
{
    SDL_Event e;
    bool running = true;
    while (running)
    {
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_EVENT_QUIT)
                running = false;
            if (e.type == SDL_EVENT_KEY_DOWN && e.key.scancode == SDL_SCANCODE_ESCAPE)
                running = false;
            if (e.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED && onResize)
            {
                imgW = e.window.data1;
                imgH = e.window.data2;
                auto resized = onResize(imgW, imgH);
                if (!resized.empty())
                    pixelsCopy = std::move(resized);
                SDL_DestroyTexture(tex);
                tex = SDL_CreateTexture(renderer, pixelFormat, SDL_TEXTUREACCESS_STATIC, imgW, imgH);
                SDL_UpdateTexture(tex, nullptr, pixelsCopy.data(), imgW * bytesPerPixel);
            }
        }
        SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
        SDL_RenderClear(renderer);
        SDL_RenderTexture(renderer, tex, nullptr, nullptr);
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }
}

static void displayWindow(const std::string& title, int imgW, int imgH, const uint8_t* pixels,
                          SDL_PixelFormat pixelFormat, int bytesPerPixel, int minWinSize, ResizeCallback onResize)
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        std::cerr << "Error: SDL_Init failed: " << SDL_GetError() << std::endl;
        throw CLI::RuntimeError(1);
    }

    int winW = imgW, winH = imgH;
    int scale = 1;
    while (winW * scale < minWinSize && winH * scale < minWinSize)
        scale *= 2;
    winW *= scale;
    winH *= scale;

    SDL_Window* window = SDL_CreateWindow(title.c_str(), winW, winH, SDL_WINDOW_RESIZABLE);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    SDL_Texture* tex = SDL_CreateTexture(renderer, pixelFormat, SDL_TEXTUREACCESS_STATIC, imgW, imgH);
    SDL_UpdateTexture(tex, nullptr, pixels, imgW * bytesPerPixel);

    std::vector<uint8_t> pixelsCopy(pixels, pixels + imgW * imgH * bytesPerPixel);
    runEventLoop(window, renderer, tex, imgW, imgH, pixelFormat, bytesPerPixel, pixelsCopy, onResize);

    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void DisplayWindowRGBA(const std::string& title, int imgW, int imgH, const uint8_t* pixels)
{
    displayWindow(title, imgW, imgH, pixels, SDL_PIXELFORMAT_ABGR8888, 4, 256, nullptr);
}

void DisplayWindowRGB(const std::string& title, int imgW, int imgH, const uint8_t* pixels, ResizeCallback onResize)
{
    displayWindow(title, imgW, imgH, pixels, SDL_PIXELFORMAT_RGB24, 3, 256, onResize);
}

} // namespace PoseidonTools
