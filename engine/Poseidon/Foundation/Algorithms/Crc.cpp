#include <Poseidon/Foundation/Algorithms/Crc.hpp>


namespace Poseidon::Foundation
{
CRCCalculator::CRCCalculator()
{
    InitTable();
}

void CRCCalculator::Reset()
{
    _crc = ~0; // preload shift register, per CRC-32 spec
}

void CRCCalculator::Add(const void* data, int len)
{
    const unsigned char* p = (const unsigned char*)data;
    uint32_t crc = _crc;
    for (; len > 0; p++, len--)
    {
        crc = (crc << 8) ^ _table[(crc >> 24) ^ *p];
    }
    _crc = crc;
}

void CRCCalculator::Add(char c)
{
    _crc = (_crc << 8) ^ _table[(_crc >> 24) ^ c];
}

uint32_t CRCCalculator::CRC(const void* data, int len)
{
    const unsigned char* p = (const unsigned char*)data;
    uint32_t crc = ~0; // preload shift register, per CRC-32 spec
    for (; len > 0; p++, len--)
    {
        crc = (crc << 8) ^ _table[(crc >> 24) ^ *p];
    }
    return ~crc; // transmit complement, per CRC-32 spec
}

void CRCCalculator::InitTable()
{
    const int CRCPoly = 0x04c11db7; // AUTODIN II, Ethernet, & FDDI
    for (int i = 0; i < 256; i++)
    {
        uint32_t c;
        int j;
        for (c = i << 24, j = 8; j > 0; --j)
        {
            c = c & 0x80000000 ? (c << 1) ^ CRCPoly : (c << 1);
        }
        _table[i] = c;
    }
}

} // namespace Poseidon::Foundation
