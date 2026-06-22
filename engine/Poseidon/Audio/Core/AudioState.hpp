#pragma once

#include <Poseidon/Foundation/Math/Math3D.hpp>
#include <Poseidon/Audio/IAudioSystem.hpp>

// Pure-data audio playback state — platform-agnostic, no behavior.

namespace Poseidon
{
struct AudioState
{
	// Playback state
	bool playing = false;        // Currently playing?
	bool wantPlaying = false;    // Wants to play (will play on next Commit)?
	bool terminated = false;     // Has playback terminated?
	bool looping = true;         // Should loop when reaching end?
	
	// Position and timing
	int curPosition = 0;         // Current playback position (bytes)
	bool posValid = false;       // Has 3D position been explicitly set?
	
	// File format information
	unsigned int size = 0;       // Size of audio data in bytes
	long frequency = 0;          // Sampling rate (Hz)
	int nChannel = 0;            // Number of channels (1=mono, 2=stereo)
	int sSize = 0;               // Number of bytes per sample
	
	// Volume analysis (from file)
	float maxVolume = 0.0f;      // Maximum volume in file
	float avgVolume = 0.0f;      // Average volume in file
	
	// 3D audio positioning
	Vector3 position = VZero;    // 3D position in world space
	Vector3 velocity = VZero;    // 3D velocity for Doppler effect
	
	// Volume control
	float volume = 1.0f;         // Master volume (0.0 to 1.0+)
	float accomodation = 1.0f;   // Ear accommodation adjustment
	float volumeAdjust = 1.0f;   // Volume adjustment based on kind
	WaveKind kind = WaveEffect;  // Sound classification (effect/speech/music)
	bool enableAccommodation = true;  // Enable ear accommodation?
	
	// Audio type flags
	bool only2DEnabled = false;  // Is this a 2D sound (no 3D positioning)?
	
	// Timing and stop control
	float stopThreshold = FLT_MAX; // Stop playback when time exceeds this (default: never)
	
	// Loading state
	bool loaded = false;         // Has audio data been loaded?
	bool loadError = false;      // Did loading fail?
	
	AudioState() = default;
	
	// Disable copy (force explicit decision about copying state)
	AudioState(const AudioState&) = delete;
	AudioState& operator=(const AudioState&) = delete;
	
	// Allow move
	AudioState(AudioState&&) = default;
	AudioState& operator=(AudioState&&) = default;
};

} // namespace Poseidon
