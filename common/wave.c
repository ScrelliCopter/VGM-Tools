/* wave.c (c) 2023, 2025 a dinosaur (zlib) */

#include "wave.h"
#include "wavedefs.h"


static void writeRiffChunk(StreamHandle hnd, IffFourCC fourcc, uint32_t size)
{
	streamWrite(hnd, fourcc.c, 1, 4);
	streamWriteU32le(hnd, size);
}

static void writeFormatChunk(StreamHandle hnd, const FormatChunk* fmt)
{
	writeRiffChunk(hnd, WAVE_FOURCC_FMT, FORMAT_CHUNK_SIZE);
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

	if (spec->format != WAVESPEC_FORMAT_PCM)
		return 1;
	if (spec->channels <= 0 || spec->channels >= INT16_MAX)
		return 1;
	if (spec->format == WAVESPEC_FORMAT_PCM && (spec->bytedepth <= 0 || spec->bytedepth > 4))
		return 1;

	// write riff container
	writeRiffChunk(hnd, FOURCC_RIFF, sizeof(uint32_t) * 5 + FORMAT_CHUNK_SIZE + (uint32_t)dataLen);
	streamWrite(hnd, FOURCC_WAVE.c, 1, 4);

	WAVEFmt sampleFmt;
	switch (spec->format)
	{
		case WAVESPEC_FORMAT_PCM:
			sampleFmt = WAVE_FMT_PCM;
			break;
		case WAVESPEC_FORMAT_FLOAT:
			sampleFmt = WAVE_FMT_IEEE_FLOAT;
			break;
	}

	writeFormatChunk(hnd, &(const FormatChunk)
	{
		.format     = sampleFmt,
		.channels   = (uint16_t)spec->channels,
		.samplerate = spec->rate,
		.byterate   = spec->rate * spec->channels * spec->bytedepth,
		.alignment  = (uint16_t)(spec->channels * spec->bytedepth),
		.bitdepth   = (uint16_t)spec->bytedepth * 8
	});

	// write data chunk
	streamWrite(hnd, WAVE_FOURCC_DATA.c, 1, 4);
	streamWriteU32le(hnd, (uint32_t)dataLen);

	return 0;
}

int waveWrite(const WaveSpec* spec, const void* data, size_t dataLen, StreamHandle hnd)
{
	assert(spec);

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
	assert(spec && blocks);

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
