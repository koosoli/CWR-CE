#ifndef WAVELOADERS_HPP
#define WAVELOADERS_HPP

#include <Poseidon/Audio/Streaming/WaveStream.hpp>

namespace Poseidon
{
// WSS format: optional 'WSS0' (0x30535357) header, a delta-pack indicator
// (0/4/8), a standard WAVEFORMATEX, then raw (possibly delta-compressed) audio.
WaveStream* WSSLoadFile(const char* name);

} // namespace Poseidon

#endif // WAVELOADERS_HPP
