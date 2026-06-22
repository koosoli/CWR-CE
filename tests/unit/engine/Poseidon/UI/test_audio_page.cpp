#include <Poseidon/Audio/Dummy/SoundSystemDummy.hpp>
#include <Poseidon/UI/Options/AudioPage.hpp>
#include <Poseidon/UI/Options/OptionsScrollList.hpp>
#include <catch2/catch_test_macros.hpp>

#include <unordered_set>
#include <vector>
#include <stddef.h>
#include <string>

using namespace Poseidon;
namespace
{
class TestableAudioPage : public AudioPage
{
  public:
    OptionsScrollList::Provider& Provider() { return ProviderRef(); }
};

struct SoundSystemGuard
{
    IAudioSystem* previous = nullptr;

    explicit SoundSystemGuard(IAudioSystem* replacement) : previous(Poseidon::GSoundsys)
    {
        Poseidon::GSoundsys = replacement;
    }

    ~SoundSystemGuard() { Poseidon::GSoundsys = previous; }
};

class FakeSwitchingAudioSystem : public SoundSystemDummy
{
  public:
    std::vector<std::string> devices;
    std::unordered_set<std::string> failingDevices;
    std::vector<std::string> switchAttempts;
    std::string currentDevice;

    std::vector<std::string> ListOutputDevices() const override { return devices; }
    std::string GetCurrentOutputDevice() const override { return currentDevice; }

    bool SwitchOutputDevice(const char* name) override
    {
        const std::string requested = (name && *name) ? std::string(name) : std::string();
        switchAttempts.push_back(requested);
        if (failingDevices.count(requested))
            return false;
        currentDevice = requested;
        return true;
    }
};
} // namespace

TEST_CASE("AudioPage skips output devices that fail to open", "[UI][AudioPage]")
{
    FakeSwitchingAudioSystem audio;
    audio.devices = {"Broken", "Working"};
    audio.failingDevices.insert("Broken");

    SoundSystemGuard guard(&audio);
    TestableAudioPage page;
    auto& provider = page.Provider();

    CHECK(provider.RowFor(0).count == 3);
    CHECK(provider.RowValue(0) == 0);

    provider.SetRowValue(0, 1);

    REQUIRE(audio.switchAttempts.size() == 2);
    CHECK(audio.switchAttempts[0] == "Broken");
    CHECK(audio.switchAttempts[1] == "Working");
    CHECK(audio.GetCurrentOutputDevice() == "Working");
    CHECK(provider.RowValue(0) == 1);
    CHECK(provider.RowFor(0).count == 2);

    provider.SetRowValue(0, 0);
    CHECK(audio.GetCurrentOutputDevice().empty());

    const size_t attemptsBefore = audio.switchAttempts.size();
    provider.SetRowValue(0, 1);
    REQUIRE(audio.switchAttempts.size() == attemptsBefore + 1);
    CHECK(audio.switchAttempts.back() == "Working");
    CHECK(audio.GetCurrentOutputDevice() == "Working");
}

TEST_CASE("AudioPage volume helpers clamp the backend scale into the UI percent range", "[UI][AudioPage]")
{
    CHECK(AudioPage::VolumeToPercent(-1.0f) == 0);
    CHECK(AudioPage::VolumeToPercent(0.0f) == 0);
    CHECK(AudioPage::VolumeToPercent(5.0f) == 50);
    CHECK(AudioPage::VolumeToPercent(10.0f) == 100);
    CHECK(AudioPage::VolumeToPercent(12.0f) == 100);

    CHECK(AudioPage::PercentToVolume(-10) == 0.0f);
    CHECK(AudioPage::PercentToVolume(0) == 0.0f);
    CHECK(AudioPage::PercentToVolume(50) == 5.0f);
    CHECK(AudioPage::PercentToVolume(100) == 10.0f);
    CHECK(AudioPage::PercentToVolume(120) == 10.0f);
}
