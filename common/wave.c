/* wave.c (c) 2023, 2025 a dinosaur (zlib) */

#include "wave.h"
#include "wavedefs.h"
#include "endian.h"


static void writeFourcc(StreamHandle hnd, RiffFourCC fourcc)
{
	streamWrite(hnd, fourcc.c, 1, 4);
}

void writeRiffChunk(StreamHandle hnd, RiffFourCC fourcc, uint32_t size)
{
	writeFourcc(hnd, fourcc);
	streamWriteU32le(hnd, size);
}

void writeFormatChunk(StreamHandle hnd, const FormatChunk* fmt)
{
	writeRiffChunk(hnd, FOURCC_FORM, FORMAT_CHUNK_SIZE);
	streamWriteU16le(hnd, fmt->format);
	streamWriteU16le(hnd, fmt->channels);
	streamWriteU32le(hnd, fmt->samplerate);
	streamWriteU32le(hnd, fmt->byterate);
	streamWriteU16le(hnd, fmt->alignment);
	streamWriteU16le(hnd, fmt->bitdepth);
}

static int waveWriteHeader(const WaveSpec* spec, size_t dataLen, StreamHandle hnd)
{
	if (!spec || !dataLen || dataLen >= UINT32_MAX || !hnd.cb || !hnd.cb->write)
		return 1;

	if (spec->format != WAVE_FMT_PCM)
		return 1;
	if (spec->channels <= 0 || spec->channels >= INT16_MAX)
		return 1;
	if (spec->format == WAVE_FMT_PCM && (spec->bytedepth <= 0 || spec->bytedepth > 4))
		return 1;

	// write riff container
	writeRiffChunk(hnd, FOURCC_RIFF, sizeof(uint32_t) * 5 + FORMAT_CHUNK_SIZE + (uint32_t)dataLen);
	writeFourcc(hnd, FOURCC_WAVE);

	writeFormatChunk(hnd, &(const FormatChunk)
	{
		.format     = spec->format,
		.channels   = (uint16_t)spec->channels,
		.samplerate = spec->rate,
		.byterate   = spec->rate * spec->channels * spec->bytedepth,
		.alignment  = (uint16_t)(spec->channels * spec->bytedepth),
		.bitdepth   = (uint16_t)spec->bytedepth * 8
	});

	// write data chunk
	writeFourcc(hnd, FOURCC_DATA);
	streamWriteU32le(hnd, (uint32_t)dataLen);

	return 0;
}

int waveWrite(const WaveSpec* spec, const void* data, size_t dataLen, StreamHandle hnd)
{
	// Write RIFF/Wave header and raw interleaved samples
	int res = waveWriteHeader(spec, dataLen, hnd);
	if (res)
		return res;
	//FIXME: not endian safe
	if (data)
		streamWrite(hnd, data, 1, dataLen);

	return 0;
}

int waveWriteBlock(const WaveSpec* spec, const void* blocks[], size_t blockLen, StreamHandle hnd)
{
	if (!blocks)
		return 1;

	// Write RIFF/Wave header to file
	int res = waveWriteHeader(spec, blockLen * spec->channels, hnd);
	if (res)
		return res;

	// Copy & interleave
	//FIXME: not endian safe
	for (size_t i = 0; i < blockLen / spec->bytedepth; ++i)
		for (int j = 0; j < spec->channels; ++j)
			streamWrite(hnd, &((const char**)blocks)[j][i * spec->bytedepth], spec->bytedepth, 1);

	return 0;
}

int waveWriteFile(const WaveSpec* spec, const void* data, size_t dataLen, const char* path)
{
	StreamHandle hnd;
	if (streamFileOpen(&hnd, path, "wb"))
		return 1;

	int res = waveWrite(spec, data, dataLen, hnd);
	streamClose(hnd);
	return res;
}

int waveWriteBlockFile(const WaveSpec* spec, const void* blocks[], size_t blockLen, const char* path)
{
	StreamHandle hnd;
	if (streamFileOpen(&hnd, path, "wb"))
		return 1;

	int res = waveWriteBlock(spec, blocks, blockLen, hnd);
	streamClose(hnd);
	return res;
}
