#include <Poseidon/Asset/Probes/AudioInit.hpp>
#include <Poseidon/Audio/AudioFactory.hpp>
#include <Poseidon/Audio/IAudioSystem.hpp>
#include <string>

namespace Poseidon
{

::IAudioSystem* CreateToolAudio()
{
    AudioCreateResult created = AudioFactory::Create("auto", false);
    return created.system;
}

} // namespace Poseidon
