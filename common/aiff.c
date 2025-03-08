#include "aiff.h"
#include "util.h"
#include <string.h>


static uint32_t calculateMarkerChunkSize(const AIFFMarker markers[], int count)
{
	uint32_t chunkSize = 2;
	for (int i = 0; i < count; ++i)
		chunkSize += 6 + ((2 + strlen(markers[i].name)) & (SIZE_MAX - 1));
	return chunkSize;
}

static extended extendedPrecisionFromUInt64(uint64_t value)
{
	extended result = { .signExponent = 0, .mantissa = 0 };
	if (fastPath(value))
	{
		unsigned leadingZeroes;
#if defined(__has_builtin) && __has_builtin(__builtin_clz)
		leadingZeroes = __builtin_clzl(value);
#else
		for (leadingZeroes = 0; ((value << leadingZeroes) & 0x1UL << 63) == 0; ++leadingZeroes);
#endif
		result.signExponent = 63 - (uint16_t)leadingZeroes + 0x3FFF;
		result.mantissa = value << leadingZeroes;
	}
	return result;
}

static extended extendedPrecisionFromUInt32(uint32_t value)
{
	extended result = { .signExponent = 0, .mantissa = 0 };
	if (fastPath(value))
	{
		unsigned leadingZeroes;
#if defined(__has_builtin) && __has_builtin(__builtin_clz)
		leadingZeroes = __builtin_clz(value);
#else
		for (leadingZeroes = 0; ((value << leadingZeroes) & 0x1 << 31) == 0; ++leadingZeroes);
#endif
		result.signExponent = 31 - (uint16_t)leadingZeroes + 0x3FFF;
		result.mantissa = ((uint64_t)value << 32) << leadingZeroes;
	}
	return result;
}

static void packExtended(uint8_t out[10], extended ext)
{
	uint16_t signExp  = SWAP_BE16(ext.signExponent);
	uint64_t mantissa = SWAP_BE64(ext.mantissa);
	memcpy(&out[0], (const void*)&signExp,  2);
	memcpy(&out[2], (const void*)&mantissa, 8);
}

static inline void writeExtendedBe(StreamHandle out, extended v)
{
	uint8_t packed[10];
	packExtended(packed, v);
	streamWrite(out, packed, 10, 1);
}

static void writePascalString(StreamHandle out, const char* const restrict string)
{
	size_t len = MIN(0xFF, strlen(string));
	uint8_t byte = (uint8_t)len;
	streamWrite(out, &byte, 1, 1);     // Length byte
	streamWrite(out, string, 1, len);  // String data
	if ((len + 1) & 0x1)               // AIFF Pascal-style strings are even byte padded
		streamPutC(out, '\0');
}

static void writeIffFormHeader(AIFFWriter* restrict writer)
{
	// Write FORM header
	streamWrite(writer->hnd, FOURCC_FORM.c, 1, 4);
	streamWriteU32be(writer->hnd, writer->formSize);
	streamWrite(writer->hnd, FOURCC_AIFF.c, 1, 4);

	writer->lastChunkSize = 4;
	writer->lastChunk = AIFF_CHUNKFLAGS_FORM;
}

static void aiffWriterFlush(AIFFWriter* restrict writer, bool atEnd)
{
	// Write FORM at the start of the file
	if (!(writer->chunks & AIFF_CHUNKFLAGS_FORM))
		writeIffFormHeader(writer);

	writer->chunks |= writer->lastChunk;

	if (!writer->formSize)
	{
		if (writer->lastChunk != AIFF_CHUNKFLAGS_FORM)
			writer->writtenChunksSize += IFF_CHUNK_HEAD_SIZE;
		writer->writtenChunksSize += writer->lastChunkSize;

		// The Sound Data chunk size needs to be written if it's not precomputed
		if (writer->lastChunk == AIFF_CHUNKFLAGS_SSND)
		{
			assert(writer->lastChunkSize <= UINT32_MAX);
			uint32_t soundChunkSz = (uint32_t)writer->lastChunkSize;
			streamSeek(writer->hnd, writer->soundDataOffset + 4, STREAM_SEEK_SET);
			streamWriteU32be(writer->hnd, soundChunkSz);
			streamSeek(writer->hnd, 0, STREAM_SEEK_END);
			writer->soundChunkSize = soundChunkSz;
		}
	}

	// Handle chunk padding
	if (slowPath(IFF_NEEDS_PAD(writer->lastChunkSize)))
	{
		streamPutC(writer->hnd, '\0');
		if (!writer->formSize && !atEnd)  // Final pad is not included in FORM size
			++writer->writtenChunksSize;
	}

	writer->lastChunkSize = writer->lastChunk = 0;
}


void aiffWriterOpen(AIFFWriter* restrict writer, StreamHandle hnd)
{
	assert(writer && hnd.cb);
	(*writer) = (AIFFWriter)
	{
		.hnd = hnd,

		.chunks    = 0,
		.lastChunk = 0,

		.markerChunkSize   = 0U,
		.soundChunkSize    = 0U,
		.lastChunkSize     = 0U,
		.writtenChunksSize = 0U,
		.formSize = 0U
	};
}

int aiffWriterFileOpen(AIFFWriter* restrict writer, const char* restrict path)
{
	StreamHandle hnd;
	int err = streamFileOpen(&hnd, path, "wb");
	if (err)
		return err;
	aiffWriterOpen(writer, hnd);
	return 0;
}

void aiffWriterClose(AIFFWriter* restrict writer)
{
	assert(writer);
	aiffWriterFlush(writer, true);

	// Overwrite FORM chunk size if not precomputed
	if (!writer->formSize)
	{
		const size_t formSize = writer->writtenChunksSize;
		assert(formSize <= UINT32_MAX);
		streamSeek(writer->hnd, 4, STREAM_SEEK_SET);
		streamWriteU32be(writer->hnd, (uint32_t)formSize);
		writer->formSize = (uint32_t)formSize;
	}

	streamClose(writer->hnd);
}


void aiffPrecalculateFormSize(AIFFWriter* restrict writer,
	AIFFChunkFlags chunks, AIFFChunkFlags lastChunk,
	const AIFFMarker markers[], uint16_t numMarkers,
	size_t frameSize, size_t frameCount)
{
	assert(writer && !writer->chunks);

	// Calculate FORM chunk size
	size_t formSize = 4;
	if (chunks & AIFF_CHUNKFLAGS_COMM)
		formSize += IFF_CHUNK_HEAD_SIZE + AIFF_COMMON_SIZE;
	if (chunks & AIFF_CHUNKFLAGS_SSND)
	{
		size_t dataLength = frameSize * frameCount;
		assert(dataLength + AIFF_SOUNDDATAHEADER_SIZE <= UINT32_MAX);
		const uint32_t soundChunkSz = AIFF_SOUNDDATAHEADER_SIZE + (uint32_t)dataLength;
		formSize += IFF_CHUNK_HEAD_SIZE + (lastChunk == AIFF_CHUNKFLAGS_SSND
			? soundChunkSz : iffRealSize(soundChunkSz));
		writer->soundChunkSize = soundChunkSz;
	}
	if (chunks & AIFF_CHUNKFLAGS_MARK)
	{
		const uint32_t markerChunkSz = calculateMarkerChunkSize(markers, numMarkers);
		formSize += IFF_CHUNK_HEAD_SIZE + markerChunkSz;
		writer->markerChunkSize = markerChunkSz;
	}
	if (chunks & AIFF_CHUNKFLAGS_INST)
		formSize += IFF_CHUNK_HEAD_SIZE + AIFF_INSTRUMENT_SIZE;

	assert(formSize <= UINT32_MAX);
	writer->formSize = (uint32_t)formSize;
}

void aiffWriteCommonChunk(AIFFWriter* writer,
	int16_t  channels,
	uint32_t frames,
	int16_t  bitDepth,
	uint32_t rate)
{
	assert(writer && !(writer->chunks & AIFF_CHUNKFLAGS_COMM));

	aiffWriterFlush(writer, false);

	AIFFCommon common;
	common.numChannels     = channels;
	common.numSampleFrames = frames;
	common.sampleSize      = bitDepth;
	common.sampleRate      = extendedPrecisionFromUInt32(rate);

	// Write Common chunk
	streamWrite(writer->hnd, AIFF_FOURCC_COMM.c, 1, 4);
	streamWriteU32be(writer->hnd, AIFF_COMMON_SIZE);
	streamWriteI16be(writer->hnd, common.numChannels);
	streamWriteU32be(writer->hnd, common.numSampleFrames);
	streamWriteI16be(writer->hnd, common.sampleSize);
	writeExtendedBe(writer->hnd, common.sampleRate);

	writer->lastChunk = AIFF_CHUNKFLAGS_COMM;
	writer->lastChunkSize = AIFF_COMMON_SIZE;
}

void aiffWriteSoundFrames(AIFFWriter* restrict writer, const uint8_t* restrict frames, size_t depth, size_t count)
{
	assert(writer && frames && !(writer->chunks & AIFF_CHUNKFLAGS_SSND));

	if (writer->lastChunk != AIFF_CHUNKFLAGS_SSND)
	{
		aiffWriterFlush(writer, false);

		// Write Sound Data chunk header
		AIFFSoundDataHeader soundData;
		soundData.offset    = 0U;
		soundData.blockSize = 0U;
		if (!writer->soundChunkSize)
			streamTell(writer->hnd, &writer->soundDataOffset);
		streamWrite(writer->hnd, AIFF_FOURCC_SSND.c, 1, 4);
		streamWriteU32be(writer->hnd, writer->soundChunkSize);
		streamWriteU32be(writer->hnd, soundData.offset);
		streamWriteU32be(writer->hnd, soundData.blockSize);
		writer->lastChunkSize = AIFF_SOUNDDATAHEADER_SIZE;
	}

	// Write Sound Data audio frames
	streamWrite(writer->hnd, frames, depth, count);

	writer->lastChunk = AIFF_CHUNKFLAGS_SSND;
	writer->lastChunkSize += depth * count;  // Appending to existing SSND chunk
}

void aiffWriteMarkers(AIFFWriter* restrict writer, const AIFFMarker markers[], uint16_t num)
{
	assert(writer && markers && !(writer->chunks & AIFF_CHUNKFLAGS_MARK));

	aiffWriterFlush(writer, false);

	if (!writer->markerChunkSize)
		writer->markerChunkSize = calculateMarkerChunkSize(markers, num);

	// Write Marker chunk & markers
	streamWrite(writer->hnd, AIFF_FOURCC_MARK.c, 1, 4);
	streamWriteU32be(writer->hnd, (uint32_t)writer->markerChunkSize);
	streamWriteU16be(writer->hnd, num);
	for (int i = 0; i < num; ++i)
	{
		const AIFFMarker* marker = &markers[i];
		streamWriteI16be(writer->hnd, marker->id);
		streamWriteI32be(writer->hnd, marker->position);
		writePascalString(writer->hnd, marker->name);
	}

	writer->lastChunk = AIFF_CHUNKFLAGS_MARK;
	writer->lastChunkSize = writer->markerChunkSize;
}

void aiffWriteInstrumentChunk(AIFFWriter* restrict writer, const AIFFInstrument* restrict instrument)
{
	assert(writer && instrument && !(writer->chunks & AIFF_CHUNKFLAGS_INST));

	aiffWriterFlush(writer, false);

	// Write Instrument chunk
	streamWrite(writer->hnd, AIFF_FOURCC_INST.c, 1, 4);
	streamWriteU32be(writer->hnd, AIFF_INSTRUMENT_SIZE);
	streamWrite(writer->hnd, &instrument->baseNote, 1, 1);
	streamWrite(writer->hnd, &instrument->detune, 1, 1);
	streamWrite(writer->hnd, &instrument->lowNote, 1, 1);
	streamWrite(writer->hnd, &instrument->highNote, 1, 1);
	streamWrite(writer->hnd, &instrument->lowVelocity, 1, 1);
	streamWrite(writer->hnd, &instrument->highVelocity, 1, 1);
	streamWriteI16be(writer->hnd, instrument->gain);
	streamWriteI16be(writer->hnd, instrument->sustainLoop.playMode);
	streamWriteI16be(writer->hnd, instrument->sustainLoop.beginLoop);
	streamWriteI16be(writer->hnd, instrument->sustainLoop.endLoop);
	streamWriteI16be(writer->hnd, instrument->releaseLoop.playMode);
	streamWriteI16be(writer->hnd, instrument->releaseLoop.beginLoop);
	streamWriteI16be(writer->hnd, instrument->releaseLoop.endLoop);

	writer->lastChunk = AIFF_CHUNKFLAGS_INST;
	writer->lastChunkSize = AIFF_INSTRUMENT_SIZE;
}
