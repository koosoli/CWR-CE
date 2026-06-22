#ifndef AUDIO_WAVEFILE_HPP
#define AUDIO_WAVEFILE_HPP

#include <Poseidon/Foundation/Common/Win.h>
#include <Poseidon/Audio/Core/Format/RiffFile.hpp>

#include <Poseidon/IO/Streams/QBStream.hpp>

namespace Poseidon
{
// Parses a RIFF/WAV header and exposes the data chunk for reading.
class WaveFile
{
private:
	QIFStream _file;

	// note: sometimes we need to allocate more than WAVEFORMATEX
	WAVEFORMATEX* _format;

	DWORD _dataOffset;
	DWORD _dataSize;
	DWORD _dataRest;
	bool error;
	DWORD _samples; // actual number of samples

protected:
	void New(const QIFStream& file);
	void Delete();

	void FreeFormat();
	void CreateFormat(int size);

public:
	WaveFile();
	~WaveFile() { Delete(); }

	void Open(const QIFStream& file) { New(file); }
	void StartDataRead();
	UINT Read(UINT cbRead, char* Dest);

	DWORD DataSize() const { return _dataSize; }
	DWORD DataOffset() const { return _dataOffset; }

	const WAVEFORMATEX& Format() const { return *_format; }
	DWORD GetSamples() const { return _samples; }

	bool GetError() const { return error; }

	IFileBuffer* GetBuffer() const { return _file.GetBuffer(); }
};

class WaveStream;

// Loads a standard PCM WAV file; returns nullptr on error.
WaveStream* WaveLoadFile(const char* filename);

} // namespace Poseidon

#endif // AUDIO_WAVEFILE_HPP
