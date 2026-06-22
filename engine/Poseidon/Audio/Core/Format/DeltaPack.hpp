#ifndef DELTAPACK_HPP
#define DELTAPACK_HPP

namespace Poseidon
{
// Exponential delta encoding, 7-bit signed deltas.
class UnpackDelta8
{
	enum { MaxDelta = 32768 };
	enum { MaxDeltaInv = 127 };

	int _delta[MaxDeltaInv * 2 + 1];

public:
	UnpackDelta8();

		int Unpack(short* dest, const char* src, int destSize, int startValue);

		int Skip(const char* src, int destSize, int startValue);
};

// Fixed delta table, 4-bit signed deltas (2 samples per byte).
class UnpackDelta4
{
	enum { MaxDelta = 8192 };
	enum { MaxDeltaInv = 7 };

	static const int _delta[MaxDeltaInv * 2 + 1];

public:
	UnpackDelta4();

		int Unpack(short* dest, const char* src, int destSize, int startValue);

		int Skip(const char* src, int destSize, int startValue);
};

// Global decompressor instances (initialized in DeltaPack.cpp)
extern UnpackDelta8 UnpackD8;
extern UnpackDelta4 UnpackD4;

} // namespace Poseidon

#endif // DELTAPACK_HPP
