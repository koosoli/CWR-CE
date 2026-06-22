#include <Poseidon/Foundation/Common/Win.h>
#include <Poseidon/Audio/IAudioSystem.hpp>
#include <Poseidon/Foundation/Strings/RString.hpp>
#include <Poseidon/Foundation/Types/Pointers.hpp>

namespace Poseidon
{

IWave::IWave(RString name) : _name(name), _sticky(false)
{
    _onTerminate = nullptr;
    _onPlay = nullptr;
}

IWave::~IWave() = default;

void IWave::OnTerminateOnce()
{
    if (!_onTerminate)
    {
        return;
    }
    _onTerminate(this, _onTerminateContext);
    _onTerminate = nullptr;
}

void IWave::OnPlayOnce()
{
    if (!_onPlay)
    {
        return;
    }
    _onPlay(this, _onPlayContext);
    _onPlay = nullptr;
}

} // namespace Poseidon
