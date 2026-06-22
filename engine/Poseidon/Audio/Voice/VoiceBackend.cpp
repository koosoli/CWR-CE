#include <Poseidon/Audio/Voice/VoiceBackend.hpp>

#include <algorithm>
#include <string_view>

namespace Poseidon
{
namespace
{
class DummyVoiceCaptureBackend : public IVoiceCaptureBackend
{
  public:
    bool open(const char*, int sampleRate, int) override
    {
        _sampleRate = sampleRate;
        return false;
    }

    void close() override { _sampleRate = 0; }
    void start() override {}
    void stop() override {}
    int availableSamples() const override { return 0; }
    int read(int16_t*, int) override { return 0; }
    float lastFramePeak() const override { return 0.0f; }
    bool isOpen() const override { return false; }
    bool isCapturing() const override { return false; }
    int sampleRate() const override { return _sampleRate; }

  private:
    int _sampleRate = 0;
};

class DummyVoiceSpeakerBackend : public IVoiceSpeakerBackend
{
  public:
    void init() override {}
    void destroy() override {}
    void setPosition(float, float, float) override {}
    void stopStream() override {}
    bool feed(VoNClient*, uint32_t) override { return false; }
    bool isActive() const override { return false; }
    float level() const override { return 0.0f; }
};

class DummyVoiceLoopbackBackend : public IVoiceLoopbackBackend
{
  public:
    bool open(int) override { return false; }
    void close() override {}
    void tick(VoNCapture&) override {}
    bool isOpen() const override { return false; }
};

bool IsBackendAvailable(const VoiceBackendDescriptor& backend)
{
    return backend.isAvailable == nullptr || backend.isAvailable();
}

std::unique_ptr<IVoiceCaptureBackend> CreateDummyCapture()
{
    return std::make_unique<DummyVoiceCaptureBackend>();
}

std::unique_ptr<IVoiceSpeakerBackend> CreateDummySpeaker()
{
    return std::make_unique<DummyVoiceSpeakerBackend>();
}

std::unique_ptr<IVoiceLoopbackBackend> CreateDummyLoopback()
{
    return std::make_unique<DummyVoiceLoopbackBackend>();
}

std::vector<std::string> ListDummyDevices()
{
    return {};
}

VoiceBackendDescriptor& DummyBackend()
{
    static VoiceBackendDescriptor backend{
        .codeName = "dummy",
        .priority = 0,
        .isAvailable = nullptr,
        .createCapture = &CreateDummyCapture,
        .createSpeaker = &CreateDummySpeaker,
        .createLoopback = &CreateDummyLoopback,
        .listDevices = &ListDummyDevices,
    };
    return backend;
}

std::vector<VoiceBackendDescriptor>& Registry()
{
    static std::vector<VoiceBackendDescriptor> registry;
    return registry;
}
} // namespace

bool RegisterVoiceBackend(const VoiceBackendDescriptor& backend)
{
    if (!backend.createCapture || !backend.createSpeaker || !backend.createLoopback)
        return false;

    auto& registry = Registry();
    const auto existing =
        std::find_if(registry.begin(), registry.end(), [&](const VoiceBackendDescriptor& item)
                     { return std::string_view(item.codeName) == std::string_view(backend.codeName); });
    if (existing != registry.end())
        return false;

    registry.push_back(backend);
    std::sort(registry.begin(), registry.end(),
              [](const VoiceBackendDescriptor& lhs, const VoiceBackendDescriptor& rhs)
              {
                  if (lhs.priority != rhs.priority)
                      return lhs.priority < rhs.priority;
                  return std::string(lhs.codeName) < std::string(rhs.codeName);
              });
    return true;
}

const VoiceBackendDescriptor& GetSelectedVoiceBackend()
{
    auto& registry = Registry();
    for (auto it = registry.rbegin(); it != registry.rend(); ++it)
    {
        if (IsBackendAvailable(*it))
            return *it;
    }
    return DummyBackend();
}

} // namespace Poseidon
