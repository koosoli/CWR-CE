#pragma once

#include <Poseidon/Audio/Core/AudioState.hpp>

// Pure timing/position logic for audio waves — no platform or DirectSound code.

namespace Poseidon
{
class WaveLogic
{
public:
	WaveLogic() = default;
	~WaveLogic() = default;

	// Disable copy, allow move
	WaveLogic(const WaveLogic&) = delete;
	WaveLogic& operator=(const WaveLogic&) = delete;
	WaveLogic(WaveLogic&&) = default;
	WaveLogic& operator=(WaveLogic&&) = default;

	static int WrapPosition(int curPosition, unsigned int size, bool looping)
	{
		if (curPosition < 0 || size == 0) return curPosition;
		
		if (looping && size > 0)
		{
			while (curPosition >= (int)size)
			{
				curPosition -= size;
			}
		}
		return curPosition;
	}

	static bool ShouldTerminate(int curPosition, unsigned int size, bool looping)
	{
		if (size == 0) return false;
		if (curPosition < 0) return false; // Waiting to start
		
		return (curPosition >= (int)size && !looping);
	}

	static float CalculateLength(unsigned int size, int sSize, long frequency)
	{
		if (sSize == 0 || frequency == 0) return 0.0f;
		return float(size / sSize) / frequency;
	}

	// A negative position means the sound is waiting on a delayed start.
	static bool IsWaiting(int curPosition)
	{
		return curPosition < 0;
	}

	static bool IsStopped(bool playing, bool wantPlaying)
	{
		return !playing && !wantPlaying;
	}

	// Advance a non-playing sound's position by deltaT seconds (skip ahead).
	static int SkipAhead(int curPosition, float deltaT, long frequency, int sSize)
	{
		int skip = int(deltaT * frequency * sSize);
		return curPosition + skip;
	}

	// Delay (seconds) implied by a negative position — the speed-of-sound offset.
	static float CalculateDelay(int curPosition, long frequency, int sSize)
	{
		if (curPosition >= 0) return 0.0f;
		if (frequency == 0 || sSize == 0) return 0.0f;
		return float(-curPosition) / (sSize * frequency);
	}

	static int TimeToPosition(float timeOffset, long frequency, int sSize)
	{
		return int(timeOffset * frequency * sSize);
	}
};

} // namespace Poseidon
