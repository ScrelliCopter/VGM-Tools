/* wave.c (c) 2023 a dinosaur (zlib) */

#include "wave.h"
#include "common.h"

#define FOURCC_RIFF "RIFF"
#define FOURCC_WAVE "WAVE"
#define FOURCC_FORM "fmt "
#define FOURCC_SAMP "smpl"
#define FOURCC_DATA "data"

#define FORMAT_CHUNK_SIZE 16
typedef struct
{
    uint16_t format;
    uint16_t channels;
    uint32_t samplerate;
    uint32_t byterate;
    uint16_t alignment;
    uint16_t bitdepth;

} FormatChunk;

typedef struct
{
    uint32_t id;
	uint32_t type;
	uint32_t loopstart;
	uint32_t loopend;
	uint32_t fraction;
	uint32_t playcount;

} SampleLoopChunk;

#define SMPL_CHUNK_HEAD_SIZE 24
typedef struct
{
	uint32_t manufacturer;
	uint32_t product;
	uint32_t samplePeriod;
	uint32_t midiUniNote;
	uint32_t midiPitchFrac;
	uint32_t smpteFormat;
	uint32_t smpteOffset;
	uint32_t sampleLoopCount;
	uint32_t sampleData;

} SamplerChunk;

static void writeFourcc(const WaveStreamCb* restrict cb, void* restrict user, const char fourcc[4])
{
	cb->write(user, fourcc, 1, 4);
}

static void writeU32le(const WaveStreamCb* restrict cb, void* restrict user, uint32_t v)
{
	uint32_t tmp = SWAP_LE32(v);
	cb->write(user, &tmp, sizeof(uint32_t), 1);
}

static void writeU16le(const WaveStreamCb* restrict cb, void* restrict user, uint16_t v)
{
	uint16_t tmp = SWAP_LE16(v);
	cb->write(user, &tmp, sizeof(uint16_t), 1);
}

void writeRiffChunk(const WaveStreamCb* restrict cb, void* restrict user, char fourcc[4], uint32_t size)
{
	writeFourcc(cb, user, fourcc);
	writeU32le(cb, user, size);
}

void writeFormatChunk(const WaveStreamCb* restrict cb, void* restrict user, const FormatChunk* fmt)
{
	writeRiffChunk(cb, user, FOURCC_FORM, FORMAT_CHUNK_SIZE);
	writeU16le(cb, user, fmt->format);
	writeU16le(cb, user, fmt->channels);
	writeU32le(cb, user, fmt->samplerate);
	writeU32le(cb, user, fmt->byterate);
	writeU16le(cb, user, fmt->alignment);
	writeU16le(cb, user, fmt->bitdepth);
}

static int waveWriteHeader(const WaveSpec* spec, size_t dataLen, const WaveStreamCb* cb, void* user)
{
	if (!spec || !dataLen || dataLen >= UINT32_MAX || !cb || !cb->write)
		return 1;

	if (spec->format != WAVE_FMT_PCM)
		return 1;
	if (spec->channels <= 0 || spec->channels >= INT16_MAX)
		return 1;
	if (spec->format == WAVE_FMT_PCM && (spec->bytedepth <= 0 || spec->bytedepth > 4))
		return 1;

	// write riff container
	writeRiffChunk(cb, user, FOURCC_RIFF, sizeof(uint32_t) * 5 + FORMAT_CHUNK_SIZE + (uint32_t)dataLen);
	writeFourcc(cb, user, FOURCC_WAVE);

	writeFormatChunk(cb, user, &(const FormatChunk)
	{
		.format     = spec->format,
		.channels   = (uint16_t)spec->channels,
		.samplerate = spec->rate,
		.byterate   = spec->rate * spec->channels * spec->bytedepth,
		.alignment  = (uint16_t)(spec->channels * spec->bytedepth),
		.bitdepth   = (uint16_t)spec->bytedepth * 8
	});

	// write data chunk
	writeFourcc(cb, user, FOURCC_DATA);
	writeU32le(cb, user, (uint32_t)dataLen);

	return 0;
}

int waveWrite(const WaveSpec* spec, const void* data, size_t dataLen, const WaveStreamCb* cb, void* user)
{
	if (!data)
		return 1;

	// Write RIFF/Wave header and raw interleaved samples
	int res = waveWriteHeader(spec, dataLen, cb, user);
	if (res)
		return res;
	//FIXME: not endian safe
	cb->write(user, data, 1, dataLen);

	return 0;
}

int waveWriteBlock(const WaveSpec* spec, const void* blocks[], size_t blockLen, const WaveStreamCb* cb, void* user)
{
	if (!blocks)
		return 1;

	// Write RIFF/Wave header to file
	int res = waveWriteHeader(spec, blockLen * spec->channels, cb, user);
	if (res)
		return res;

	// Copy & interleave
	//FIXME: not endian safe
	for (size_t i = 0; i < blockLen / spec->bytedepth; ++i)
		for (int j = 0; j < spec->channels; ++j)
			cb->write(user, &((const char**)blocks)[j][i * spec->bytedepth], spec->bytedepth, 1);

	return 0;
}
