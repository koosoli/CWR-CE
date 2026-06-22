#include <Poseidon/Foundation/Common/FltOpts.hpp>
#ifdef _WIN32
#include <windows.h>
#endif
#include <Poseidon/Network/IntegrityCheck.hpp>
#include <Poseidon/Foundation/Algorithms/Crc.hpp>
#include <Poseidon/IO/Streams/QBStream.hpp>
#include <Poseidon/Foundation/Common/Filenames.hpp>

unsigned int CalculateExeCRC(int offset, int size)
{
#ifdef _WIN32
    HMODULE hMod = GetModuleHandle(nullptr);

    // get exe header from PE
    PIMAGE_DOS_HEADER pDosHdr = (PIMAGE_DOS_HEADER)hMod;

    // From the DOS header, find the NT (PE) header
    PIMAGE_NT_HEADERS pNtHdr = (PIMAGE_NT_HEADERS)((char*)hMod + pDosHdr->e_lfanew);

    PIMAGE_SECTION_HEADER pSection = IMAGE_FIRST_SECTION(pNtHdr);

    uintptr_t codeStart = (uintptr_t)hMod + pSection->PointerToRawData;
    uintptr_t codeSize = pSection->SizeOfRawData;
    uintptr_t codeEnd = codeStart + codeSize;

    uintptr_t checkStart = offset;
    uintptr_t checkEnd = offset + size;
    if (checkStart < codeStart)
        checkStart = codeStart;
    if (checkStart > codeEnd)
        checkStart = codeEnd;
    if (checkEnd < codeStart)
        checkEnd = codeStart;
    if (checkEnd > codeEnd)
        checkEnd = codeEnd;

    Poseidon::Foundation::CRCCalculator crc;
    crc.Reset();
    crc.Add((void*)checkStart, (int)(checkEnd - checkStart));
    return crc.GetResult();
#else
    return 0;
#endif
}

unsigned int CalculateDataCRC(const char* path, int offset, int size)
{
    // reject files not inside current directory
    if (!IsRelativePath(path))
    {
        return 0;
    }
    QIFStreamB in;
    in.AutoOpen(path);
    if (in.fail())
    {
        return 0;
    }
    int endCheck = offset + size;
    saturate(offset, 0, in.rest());
    saturate(endCheck, offset, in.rest());

    Poseidon::Foundation::CRCCalculator crc;
    crc.Reset();
    crc.Add(in.act() + offset, endCheck - offset);
    return crc.GetResult();
}
