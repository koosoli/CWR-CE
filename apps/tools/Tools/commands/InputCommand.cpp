#include "InputCommand.hpp"
#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <SDL3/SDL.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_gamepad.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_joystick.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_video.h>
#include <stdint.h>
#include <CLI/App.hpp>
#include <functional>
#include <string>

namespace PoseidonTools
{

void InputCommand::Setup(CLI::App& app)
{
    app.add_subcommand("check-input", "Verify SDL3 input + gamepad subsystem initializes (CI-safe, no window)")
        ->callback([]() { std::exit(ExecuteCheck()); });

    app.add_subcommand("test-input", "Interactive input event logger — keyboard, mouse, gamepad | Esc to quit")
        ->callback([]() { std::exit(ExecuteTest()); });
}

int InputCommand::ExecuteCheck()
{
    if (!SDL_Init(SDL_INIT_EVENTS | SDL_INIT_GAMEPAD))
    {
        std::cerr << "[check-input] SDL_Init failed: " << SDL_GetError() << "\n";
        return 1;
    }

    int numkeys = 0;
    const bool* keys = SDL_GetKeyboardState(&numkeys);
    if (!keys || numkeys == 0)
    {
        std::cerr << "[check-input] SDL_GetKeyboardState returned empty table\n";
        SDL_Quit();
        return 1;
    }
    std::cout << "[check-input] Keyboard: " << numkeys << " key slots\n";

    int count = 0;
    SDL_JoystickID* ids = SDL_GetGamepads(&count);
    std::cout << "[check-input] Gamepads connected: " << count << "\n";
    for (int i = 0; i < count; i++)
    {
        const char* name = SDL_GetGamepadNameForID(ids[i]);
        std::cout << "[check-input]   [" << i << "] " << (name ? name : "unknown") << "\n";
    }
    SDL_free(ids);

    std::cout << "[check-input] OK\n" << std::flush;
    SDL_Quit();
    return 0;
}

int InputCommand::ExecuteTest()
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMEPAD))
    {
        std::cerr << "[test-input] SDL_Init failed: " << SDL_GetError() << "\n";
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("test-input | keyboard / mouse / gamepad | Esc to quit", 520, 160, 0);
    if (!window)
    {
        std::cerr << "[test-input] SDL_CreateWindow failed: " << SDL_GetError() << "\n";
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer)
    {
        std::cerr << "[test-input] SDL_CreateRenderer failed: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_SetRenderDrawColor(renderer, 30, 30, 40, 255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);

    // open any already-connected gamepads
    SDL_Gamepad* gamepad = nullptr;
    {
        int count = 0;
        SDL_JoystickID* ids = SDL_GetGamepads(&count);
        if (count > 0)
        {
            gamepad = SDL_OpenGamepad(ids[0]);
            const char* name = SDL_GetGamepadName(gamepad);
            std::cout << "[test-input] Gamepad connected: " << (name ? name : "unknown") << "\n";
        }
        SDL_free(ids);
    }
    if (!gamepad)
        std::cout << "[test-input] No gamepad connected — plug one in to test\n";

    std::cout << "[test-input] Window open. Press Esc or close to quit.\n"
              << "[test-input] Gamepad: RT (trigger) = shoot rumble, all buttons/axes logged.\n"
              << std::flush;

    static const struct
    {
        uint8_t r, g, b;
    } kPalette[] = {
        {60, 120, 220}, {220, 80, 60}, {60, 200, 100}, {200, 160, 40}, {160, 60, 220}, {40, 200, 200},
    };
    int palIdx = 0;

    auto repaint = [&](const char* label)
    {
        auto& c = kPalette[palIdx % 6];
        SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, 255);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 30, 30, 40, 255);
        SDL_FRect inner = {4, 4, 520 - 8, 160 - 8};
        SDL_RenderFillRect(renderer, &inner);
        SDL_RenderPresent(renderer);
        char title[256];
        std::snprintf(title, sizeof(title), "test-input | %s", label);
        SDL_SetWindowTitle(window, title);
    };

    SDL_Event e;
    bool running = true;
    while (running && SDL_WaitEvent(&e))
    {
        switch (e.type)
        {
            case SDL_EVENT_QUIT:
                running = false;
                break;

            case SDL_EVENT_KEY_DOWN:
                if (e.key.scancode == SDL_SCANCODE_ESCAPE)
                {
                    running = false;
                    break;
                }
                if (!e.key.repeat)
                {
                    char label[64];
                    std::snprintf(label, sizeof(label), "KEY_DOWN %s", SDL_GetScancodeName(e.key.scancode));
                    std::cout << label << "\n" << std::flush;
                    repaint(label);
                    ++palIdx;
                }
                break;

            case SDL_EVENT_KEY_UP:
            {
                char label[64];
                std::snprintf(label, sizeof(label), "KEY_UP %s", SDL_GetScancodeName(e.key.scancode));
                std::cout << label << "\n" << std::flush;
                repaint(label);
                break;
            }

            case SDL_EVENT_MOUSE_MOTION:
                if (e.motion.xrel || e.motion.yrel)
                {
                    char label[64];
                    std::snprintf(label, sizeof(label), "MOUSE_MOVE dx=%d dy=%d", (int)e.motion.xrel,
                                  (int)e.motion.yrel);
                    std::cout << label << "\n" << std::flush;
                    repaint(label);
                }
                break;

            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            {
                char label[64];
                std::snprintf(label, sizeof(label), "MOUSE_DOWN btn=%d", (int)e.button.button);
                std::cout << label << "\n" << std::flush;
                repaint(label);
                ++palIdx;
                SDL_SetWindowRelativeMouseMode(window, true);
                break;
            }

            case SDL_EVENT_MOUSE_BUTTON_UP:
            {
                char label[64];
                std::snprintf(label, sizeof(label), "MOUSE_UP btn=%d", (int)e.button.button);
                std::cout << label << "\n" << std::flush;
                repaint(label);
                break;
            }

            case SDL_EVENT_MOUSE_WHEEL:
            {
                char label[64];
                std::snprintf(label, sizeof(label), "MOUSE_WHEEL y=%.1f", e.wheel.y);
                std::cout << label << "\n" << std::flush;
                repaint(label);
                ++palIdx;
                break;
            }

            case SDL_EVENT_GAMEPAD_ADDED:
            {
                if (!gamepad)
                {
                    gamepad = SDL_OpenGamepad(e.gdevice.which);
                    const char* name = SDL_GetGamepadName(gamepad);
                    char label[64];
                    std::snprintf(label, sizeof(label), "GAMEPAD_CONNECTED %s", name ? name : "unknown");
                    std::cout << label << "\n" << std::flush;
                    repaint(label);
                    ++palIdx;
                }
                break;
            }

            case SDL_EVENT_GAMEPAD_REMOVED:
            {
                if (gamepad && SDL_GetGamepadID(gamepad) == e.gdevice.which)
                {
                    std::cout << "GAMEPAD_DISCONNECTED\n" << std::flush;
                    SDL_CloseGamepad(gamepad);
                    gamepad = nullptr;
                    repaint("GAMEPAD_DISCONNECTED");
                }
                break;
            }

            case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
            {
                const char* btnName = SDL_GetGamepadStringForButton((SDL_GamepadButton)e.gbutton.button);
                char label[64];
                std::snprintf(label, sizeof(label), "PAD_BTN_DOWN %s", btnName ? btnName : "?");
                std::cout << label << "\n" << std::flush;
                repaint(label);
                ++palIdx;
                break;
            }

            case SDL_EVENT_GAMEPAD_BUTTON_UP:
            {
                const char* btnName = SDL_GetGamepadStringForButton((SDL_GamepadButton)e.gbutton.button);
                char label[64];
                std::snprintf(label, sizeof(label), "PAD_BTN_UP %s", btnName ? btnName : "?");
                std::cout << label << "\n" << std::flush;
                repaint(label);
                break;
            }

            case SDL_EVENT_GAMEPAD_AXIS_MOTION:
            {
                // only log significant movement to avoid flood
                if (abs(e.gaxis.value) > 4000 || e.gaxis.value == 0)
                {
                    const char* axName = SDL_GetGamepadStringForAxis((SDL_GamepadAxis)e.gaxis.axis);
                    char label[64];
                    std::snprintf(label, sizeof(label), "PAD_AXIS %s=%d", axName ? axName : "?", (int)e.gaxis.value);
                    std::cout << label << "\n" << std::flush;
                    repaint(label);
                }
                // RT = shoot: rumble on threshold crossing (edge detect)
                if (e.gaxis.axis == SDL_GAMEPAD_AXIS_RIGHT_TRIGGER && gamepad)
                {
                    static bool rtWasDown = false;
                    bool rtDown = e.gaxis.value > 16000;
                    if (rtDown && !rtWasDown)
                        SDL_RumbleGamepad(gamepad, 0x1000, 0xC000, 150);
                    rtWasDown = rtDown;
                }
                break;
            }

            default:
                break;
        }
    }

    if (gamepad)
        SDL_CloseGamepad(gamepad);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    std::cout << "[test-input] Done.\n" << std::flush;
    return 0;
}

} // namespace PoseidonTools
