#include <PoseidonOpenAL/Voice/VoNCaptureOpenAL.hpp>
#include <PoseidonOpenAL/Voice/VoNSpeakerOpenAL.hpp>
#include <PoseidonOpenAL/Voice/MicLoopbackOpenAL.hpp>

#include <PoseidonOpenAL/OpenALRuntime.hpp>
#include <Poseidon/Audio/Voice/VoiceBackend.hpp>


namespace Poseidon
{
namespace
{
bool IsOpenALVoiceAvailable()
{
    return OpenALRuntime::IsAvailable();
}

std::unique_ptr<IVoiceCaptureBackend> CreateOpenALVoiceCapture()
{
    return std::make_unique<VoNCaptureOpenAL>();
}

std::unique_ptr<IVoiceSpeakerBackend> CreateOpenALVoiceSpeaker()
{
    return std::make_unique<VoNSpeakerOpenAL>();
}

std::unique_ptr<IVoiceLoopbackBackend> CreateOpenALVoiceLoopback()
{
    return std::make_unique<MicLoopbackOpenAL>();
}

std::vector<std::string> ListOpenALVoiceDevices()
{
    return VoNCaptureOpenAL::ListDevices();
}
} // namespace

void RegisterOpenALVoiceBackend()
{
    static const bool registered = RegisterVoiceBackend({
        .codeName       = "openal",
        .priority       = 1,
        .isAvailable    = &IsOpenALVoiceAvailable,
        .createCapture  = &CreateOpenALVoiceCapture,
        .createSpeaker  = &CreateOpenALVoiceSpeaker,
        .createLoopback = &CreateOpenALVoiceLoopback,
        .listDevices    = &ListOpenALVoiceDevices,
    });
    (void)registered;
}

} // namespace Poseidon
