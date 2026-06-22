#include "VonCommand.hpp"
#include <Poseidon/Audio/Voice/VonApp.hpp>
#include <Poseidon/Audio/Voice/VonSpeaker.hpp>
#include <Poseidon/Audio/Voice/OpusCodec.hpp>
#include <Poseidon/Audio/Voice/VonNet.hpp>
#include <PoseidonOpenAL/OpenALRuntime.hpp>
#include <SDL3/SDL.h>
#include <iostream>
#include <vector>
#include <deque>
#include <chrono>
#include <cstring>
#include <cmath>
#include <AL/alc.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>
#include <stdint.h>
#include <CLI/App.hpp>
#include <CLI/Option.hpp>
#include <CLI/Validators.hpp>
#include <compare>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <memory>
#include <string>
#include <utility>

using namespace Poseidon;

namespace PoseidonTools
{

namespace
{

static constexpr int RATE = 16000;
static constexpr int FRAME = 320; // 20ms at 16kHz
static constexpr int WIN_W = 640;
static constexpr int WIN_H = 240;

struct DelayedPacket
{
    std::chrono::steady_clock::time_point deliverAt;
    std::vector<uint8_t> data;
};

static float rmsLevel(const int16_t* pcm, int n)
{
    if (n <= 0)
        return 0.0f;
    double sum = 0;
    for (int i = 0; i < n; ++i)
        sum += static_cast<double>(pcm[i]) * pcm[i];
    return static_cast<float>(std::sqrt(sum / n) / 32768.0);
}

static void drawMeter(SDL_Renderer* r, int x, int y, int w, int h, float level, uint8_t cr, uint8_t cg, uint8_t cb)
{
    SDL_SetRenderDrawColor(r, 40, 40, 50, 255);
    SDL_FRect bg = {static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h)};
    SDL_RenderFillRect(r, &bg);
    float fill = (level > 1.0f) ? 1.0f : level;
    int fw = static_cast<int>(fill * w);
    if (fw > 0)
    {
        SDL_SetRenderDrawColor(r, cr, cg, cb, 255);
        SDL_FRect bar = {static_cast<float>(x), static_cast<float>(y), static_cast<float>(fw), static_cast<float>(h)};
        SDL_RenderFillRect(r, &bar);
    }
    SDL_SetRenderDrawColor(r, 100, 100, 120, 255);
    SDL_FRect border = {static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h)};
    SDL_RenderRect(r, &border);
}

static void drawPanel(SDL_Renderer* r, int x, int y, int w, int h, const char* label, float level, bool active,
                      uint8_t cr, uint8_t cg, uint8_t cb, int packets)
{
    SDL_SetRenderDrawColor(r, active ? static_cast<uint8_t>(50) : static_cast<uint8_t>(30),
                           active ? static_cast<uint8_t>(50) : static_cast<uint8_t>(30),
                           active ? static_cast<uint8_t>(65) : static_cast<uint8_t>(40), 255);
    SDL_FRect panel = {static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h)};
    SDL_RenderFillRect(r, &panel);
    if (active)
    {
        SDL_SetRenderDrawColor(r, cr, cg, cb, 255);
        SDL_FRect indicator = {static_cast<float>(x + 8), static_cast<float>(y + 8), 12.0f, 12.0f};
        SDL_RenderFillRect(r, &indicator);
    }
    else
    {
        SDL_SetRenderDrawColor(r, 80, 80, 80, 255);
        SDL_FRect indicator = {static_cast<float>(x + 8), static_cast<float>(y + 8), 12.0f, 12.0f};
        SDL_RenderRect(r, &indicator);
    }
    drawMeter(r, x + 8, y + h - 28, w - 16, 20, level * 5.0f, cr, cg, cb);
}

static int ExecuteTest(int delayMs)
{
    if (!OpenALRuntime::EnsureLoaded())
    {
        std::cerr << "[von-test] OpenAL runtime unavailable: " << OpenALRuntime::LastError() << "\n";
        return 1;
    }
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
    {
        std::cerr << "[von-test] SDL_Init failed: " << SDL_GetError() << "\n";
        return 1;
    }
    ALCdevice* playDev = alcOpenDevice(nullptr);
    if (!playDev)
    {
        std::cerr << "[von-test] Failed to open OpenAL playback device\n";
        SDL_Quit();
        return 1;
    }
    ALCcontext* ctx = alcCreateContext(playDev, nullptr);
    alcMakeContextCurrent(ctx);
    VoNSystem vonSys;
    vonSys.initClient();
    vonSys.initServer();

    auto* client = vonSys.client();
    client->setCodecFactory([]() { return std::make_unique<OpusCodec>(); });
    client->setSenderId(1);
    client->setChatChannel(VoNChatChannel::Direct);
    client->createChannel(1, {}); // receive own echo

    // Direct delivery keeps the echo path inside the same client process.
    vonSys.server()->setRouting(1, VoNChatChannel::Direct, {1});
    VoNCapture capture;
    bool captureOk = capture.open(nullptr, RATE, RATE); // 1s buffer
    if (!captureOk)
    {
        std::cerr << "[von-test] WARNING: No capture device available — transmit will be silent\n";
    }
    OpusCodec encoder;
    std::vector<uint8_t> encBuf(encoder.maxEncodedSize());
    std::vector<int16_t> capBuf(FRAME);
    uint64_t origin = 0;
    std::deque<DelayedPacket> delayQueue;
    auto delayDuration = std::chrono::milliseconds(delayMs);
    char titleBuf[256];
    std::snprintf(titleBuf, sizeof(titleBuf), "VoN Echo Test | CapsLock=Transmit | Delay=%dms | Esc=Quit", delayMs);

    SDL_Window* window = SDL_CreateWindow(titleBuf, WIN_W, WIN_H, 0);
    if (!window)
    {
        std::cerr << "[von-test] SDL_CreateWindow failed: " << SDL_GetError() << "\n";
        vonSys.shutdown();
        alcMakeContextCurrent(nullptr);
        alcDestroyContext(ctx);
        alcCloseDevice(playDev);
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer)
    {
        std::cerr << "[von-test] SDL_CreateRenderer failed\n";
        SDL_DestroyWindow(window);
        vonSys.shutdown();
        alcMakeContextCurrent(nullptr);
        alcDestroyContext(ctx);
        alcCloseDevice(playDev);
        SDL_Quit();
        return 1;
    }

    std::cout << "[von-test] VoN Echo Test started\n"
              << "  Delay: " << delayMs << "ms\n"
              << "  Capture device: " << (captureOk ? "OK" : "NONE") << "\n"
              << "  Hold CapsLock to transmit, release to stop\n"
              << "  Press Esc or close window to quit\n"
              << std::flush;

    float txLevel = 0.0f, rxLevel = 0.0f;
    bool transmitting = false;
    int txPackets = 0, rxPackets = 0;

    // Shared speaker implementation (same code as game)
    VoNSpeaker speaker;
    speaker.init();

    bool running = true;
    auto lastDraw = std::chrono::steady_clock::now();

    while (running)
    {
        SDL_Event ev;
        while (SDL_PollEvent(&ev))
        {
            if (ev.type == SDL_EVENT_QUIT)
                running = false;
            if (ev.type == SDL_EVENT_KEY_DOWN && ev.key.scancode == SDL_SCANCODE_ESCAPE)
                running = false;
        }
        const bool* keys = SDL_GetKeyboardState(nullptr);
        bool capsDown = keys && keys[SDL_SCANCODE_CAPSLOCK];

        if (capsDown && !transmitting && captureOk)
        {
            capture.start();
            transmitting = true;
        }
        else if (!capsDown && transmitting)
        {
            capture.stop();
            transmitting = false;
            txLevel = 0.0f;
            origin = 0;
        }
        if (transmitting && captureOk)
        {
            while (capture.availableSamples() >= FRAME)
            {
                capture.read(capBuf.data(), FRAME);
                txLevel = rmsLevel(capBuf.data(), FRAME);

                int enc = encoder.encode(capBuf.data(), FRAME, encBuf.data(), static_cast<int>(encBuf.size()));
                if (enc > 0)
                {
                    std::vector<uint8_t> pkt(VoNDataPacket::HEADER_SIZE + enc);
                    auto* dp = reinterpret_cast<VoNDataPacket*>(pkt.data());
                    dp->init(1, VoNChatChannel::Direct, origin, FRAME, static_cast<uint16_t>(enc));
                    std::memcpy(dp->payload(), encBuf.data(), enc);

                    DelayedPacket dp2;
                    dp2.deliverAt = std::chrono::steady_clock::now() + delayDuration;
                    dp2.data = std::move(pkt);
                    delayQueue.push_back(std::move(dp2));

                    origin += FRAME;
                    ++txPackets;
                }
            }
        }
        auto now = std::chrono::steady_clock::now();
        while (!delayQueue.empty() && delayQueue.front().deliverAt <= now)
        {
            auto& pkt = delayQueue.front().data;
            VoNDataPacket hdr;
            std::memcpy(&hdr, pkt.data(), VoNDataPacket::HEADER_SIZE);
            client->onDataPacket(hdr, pkt.data() + VoNDataPacket::HEADER_SIZE);
            delayQueue.pop_front();
            ++rxPackets;
        }

        speaker.feed(client, 1);
        rxLevel = speaker.active ? speaker.level : 0.0f;
        if (!transmitting)
            txLevel *= 0.9f;
        rxLevel *= 0.95f;
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastDraw);
        if (elapsed.count() >= 33)
        {
            lastDraw = now;
            SDL_SetRenderDrawColor(renderer, 20, 20, 28, 255);
            SDL_RenderClear(renderer);

            int halfW = WIN_W / 2;
            SDL_SetRenderDrawColor(renderer, 80, 80, 100, 255);
            SDL_RenderLine(renderer, static_cast<float>(halfW), 0.0f, static_cast<float>(halfW),
                           static_cast<float>(WIN_H));
            drawPanel(renderer, 0, 0, halfW, WIN_H, "TX", txLevel, transmitting, 60, 200, 100, txPackets);
            bool receiving = (rxLevel > 0.001f);
            drawPanel(renderer, halfW, 0, halfW, WIN_H, "RX", rxLevel, receiving, 60, 120, 220, rxPackets);
            std::snprintf(titleBuf, sizeof(titleBuf), "VoN Echo | %s | TX:%d RX:%d | Delay:%dms | Esc=Quit",
                          transmitting ? "TRANSMITTING" : "idle", txPackets, rxPackets, delayMs);
            SDL_SetWindowTitle(window, titleBuf);

            SDL_RenderPresent(renderer);
        }

        SDL_Delay(5);
    }
    speaker.destroy();

    if (captureOk)
        capture.close();
    vonSys.shutdown();

    alcMakeContextCurrent(nullptr);
    alcDestroyContext(ctx);
    alcCloseDevice(playDev);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    std::cout << "[von-test] Done. TX=" << txPackets << " RX=" << rxPackets << "\n" << std::flush;
    return 0;
}

} // namespace

void VonCommand::Setup(CLI::App& app)
{
    auto* cmd = app.add_subcommand(
        "test-von", "VoN echo test — CapsLock=transmit, audio loops through Opus codec + VoN server with delay");
    static int delayMs = 2000;
    cmd->add_option("--delay,-d", delayMs, "Echo delay in milliseconds (default: 2000)")->check(CLI::Range(0, 30000));
    cmd->callback([]() { std::exit(ExecuteTest(delayMs)); });
}

} // namespace PoseidonTools
