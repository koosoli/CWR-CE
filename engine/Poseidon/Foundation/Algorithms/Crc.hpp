#pragma once
#include <cstdint>

// CRC-32 (FDDI / Ethernet polynomial).

namespace Poseidon::Foundation
{
class CRCCalculator
{
	uint32_t _table[256];
	uint32_t _crc;

	void InitTable();

	public:
	CRCCalculator();
	// one-shot CRC of a memory block
	uint32_t CRC( const void *data, int len );
	// Incremental interface: Reset, then Add..., then GetResult.
	void Reset();
	void Add(const void *data, int len);
	void Add(char c);
	// CRC of everything added since Reset()
	uint32_t GetResult() const {return ~_crc;}

};

} // namespace Poseidon::Foundation
