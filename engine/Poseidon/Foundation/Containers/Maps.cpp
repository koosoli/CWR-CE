
#define MaxPrime 24

namespace Poseidon::Foundation
{
const unsigned
    primes[MaxPrime] =
        {
            5,       13,      31,      61,       127,      251,     509,    1021,   2039,
            4051,    8111,    16223,   32467,    64937,    129887,  259781, 519577, 1039169,
            2078339, 4156709, 8313433, 16626941, 33253889, 66507787}; // that seems to be enough for 32-bit computers
                                                                      // (maximum hash table size = 256MB !)

unsigned hashSize(unsigned s)
{
    int i = 0;
    while (i < MaxPrime && primes[i] < s)
    {
        i++;
    }
    return primes[i];
}

} // namespace Poseidon::Foundation
