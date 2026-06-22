#pragma once

namespace Poseidon
{

class IAudioSystem;

// Create an audio system for tool use (not game). Uses OpenAL, falls back to Dummy.
// Returns nullptr on total failure. Caller owns the returned pointer.
IAudioSystem* CreateToolAudio();

} // namespace Poseidon
