#include "fuzz_init.hpp"

#include <Poseidon/Asset/Formats/Common/CsvReader.hpp>
#include <Poseidon/IO/Streams/QStream.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// libFuzzer harness for the CSV row reader behind the stringtable loader
// (LoadStringtable / LookupStringtableCsv / the mission-language detector all parse
// stringtable.csv through CsvReadRow). Every downloaded mission ships a
// stringtable, so the bytes are attacker-controlled. CsvReadRow takes a base
// QIStream, so the input drives it directly from memory -- no temp file. A thrown
// error is handled; only a memory-safety fault reaches ASan.

using namespace Poseidon;
using namespace Poseidon::Asset::Formats;

extern "C" int LLVMFuzzerInitialize(int*, char***)
{
    fuzz::InitEngine();
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    try
    {
        QIStream in(const_cast<char*>(reinterpret_cast<const char*>(data)), static_cast<int>(size));
        std::vector<std::string> row;
        int rows = 0;
        while (CsvReadRow(in, row) && ++rows < (1 << 20))
        {
            // row parsed; nothing to do -- exercising the reader is the point
        }
    }
    catch (...)
    {
    }
    return 0;
}
