#ifndef _FILEINFO_H
#define _FILEINFO_H

#include <cstdint>

#define NAME_LEN 128-20

#ifdef __GNUC__

#define EncrMagic    StrToInt("rcnE")
#define CompMagic    StrToInt("srpC")
#define VersionMagic StrToInt("sreV")

#else

#define EncrMagic 'Encr'
#define CompMagic 'Cprs'
#define VersionMagic 'Vers'

#endif


namespace Poseidon
{
struct FileInfo
{
	char name[NAME_LEN];
	int compressedMagic;
	int uncompressedSize;
	int32_t startOffset;
	int32_t time;
	int32_t length;

	const char *GetKey() const {return name;}
};

} // namespace Poseidon
#endif
