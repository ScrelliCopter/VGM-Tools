/* aifsampconv.c (c) 2025 a dinosaur (zlib) */

#include "wave.h"
#include "aiffdefs.h"
#include "wavedefs.h"
#include "endian.h"
#include "util.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <float.h>


static inline void writeFourCC(StreamHandle hnd, char fourcc[4])
{
	streamWrite(hnd, (const void*)fourcc, 1, 4);
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

static AIFFMarkerPlayMode aiffPlayModeFromSmplLoopType(SamplerLoopType type)
{
	switch (type)
	{
	case LOOP_TYPE_BIDIRECTIONAL:  // Bidi is not supported in AIFF instrument chunks
	case LOOP_TYPE_FORWARD: return AIFF_LOOP_FORWARD;
	case LOOP_TYPE_REVERSE: return AIFF_LOOP_REVERSE;
	default:                return AIFF_LOOP_OFF;
	}
}

int main(int argc, char** argv)
{
	//FIXME: real CLI
	if (argc < 1)
		return 1;
	const char* inName = argv[1];
	StreamHandle in;
	if (streamFileOpen(&in, inName, "rb"))
		return 1;

	// Read & verify header
	RiffChunk riff;
	RiffFourCC filetype;
	streamRead(in, riff.fourcc.c, 1, 4);
	streamReadU32le(in, &riff.size, 1);
	streamRead(in, filetype.c,    1, 4);

	if (!WAVE_FOURCC_CMP(riff.fourcc, FOURCC_RIFF))
		return 1;
	if (riff.size < FORMAT_CHUNK_SIZE)
		return 1;
	if (!WAVE_FOURCC_CMP(filetype, FOURCC_WAVE))
		return 1;

	FormatChunk fmt;
	SamplerChunk smpl;
	SampleLoopChunk loops[2];
	size_t dataOffset = 0, dataBytes = 0;
	bool samplerPresent = false;

	size_t bytes = 4;
	do
	{
		RiffChunk chunk;
		memset(&chunk, 0, sizeof(RiffChunk));
		streamRead(in, &chunk.fourcc, 1, 4);
		streamReadU32le(in, &chunk.size, 1);

		if (WAVE_FOURCC_CMP(chunk.fourcc, FOURCC_FORM))
		{
			if (chunk.size != FORMAT_CHUNK_SIZE)
				return 1;

			streamReadU16le(in, &fmt.format, 1);
			streamReadU16le(in, &fmt.channels, 1);
			streamReadU32le(in, &fmt.samplerate, 1);
			streamReadU32le(in, &fmt.byterate, 1);
			streamReadU16le(in, &fmt.alignment, 1);
			streamReadU16le(in, &fmt.bitdepth, 1);

		}
		else if (WAVE_FOURCC_CMP(chunk.fourcc, FOURCC_SAMP))
		{
			if (chunk.size < SMPL_CHUNK_HEAD_SIZE)
				return 1;

			streamReadU32le(in, &smpl.manufacturer, 1);
			streamReadU32le(in, &smpl.product, 1);
			streamReadI32le(in, &smpl.samplePeriod, 1);
			streamReadI32le(in, &smpl.midiUnityNote, 1);
			streamReadI32le(in, &smpl.midiPitchFrac, 1);
			streamReadI32le(in, &smpl.smpteFormat, 1);
			streamReadU32le(in, &smpl.smpteOffset, 1);
			streamReadU32le(in, &smpl.sampleLoopCount, 1);
			streamReadU32le(in, &smpl.sampleData, 1);

			const size_t loopsinc = SMPL_CHUNK_HEAD_SIZE + smpl.sampleLoopCount * SAMPLER_LOOP_SIZE;
			if (chunk.size < loopsinc)
				return 1;

			// Read loops
			SampleLoopChunk loop;
			for (uint32_t i = 0; i < smpl.sampleLoopCount; ++i)
			{
				streamReadU32le(in, &loop.id, 1);
				streamReadU32le(in, &loop.type, 1);
				streamReadU32le(in, &loop.loopStart, 1);
				streamReadU32le(in, &loop.loopEnd, 1);
				streamReadU32le(in, &loop.fraction, 1);
				streamReadU32le(in, &loop.playCount, 1);
				if (i < 2)
					memcpy(&loops[i], &loop, sizeof(SampleLoopChunk));
			}

			// Some apps incorrectly set the vendor data field,
			//  so skip processing vendor data if there is none
			if (chunk.size != loopsinc && smpl.sampleData)
			{
				if (smpl.sampleData != loopsinc + chunk.size)
					return 1;

				streamSkip(in, smpl.sampleData);
			}

			samplerPresent = true;
		}
		else if (WAVE_FOURCC_CMP(chunk.fourcc, FOURCC_DATA))
		{
			if (streamTell(in, &dataOffset))
			{
				dataBytes = chunk.size;
				streamSkip(in, dataBytes);
			}
		}
		else
		{
			streamSkip(in, chunk.size);
		}

		bytes += sizeof(uint32_t) * 2 + chunk.size;
		if (chunk.size & 0x1)
		{
			streamSkip(in, 1);
			++bytes;
		}

		if (streamEOF(in) || streamError(in))
			break;
	}
	while (bytes < riff.size);

	if (fmt.channels > INT16_MAX)
		return 1;
	if (fmt.bitdepth == 0 || fmt.bitdepth > 32)
		return 1;

	const char* outName = argc > 2 ? argv[2] : "out.aif";
	StreamHandle out;
	if (streamFileOpen(&out, outName, "wb"))
	{
		streamClose(in);
		return 1;
	}

	const uint32_t byteDepth = ((uint32_t)fmt.bitdepth + 7u) / 8u;  //TODO: Prob not correct for wave
	const uint32_t bytesPerFrame = byteDepth * (uint32_t)fmt.channels;
	const size_t numFrames = dataBytes / bytesPerFrame;

	AIFFCommon common;
	common.numChannels     = (int16_t)fmt.channels;
	common.numSampleFrames = (uint32_t)numFrames;
	common.sampleSize      = (int16_t)fmt.bitdepth;
	common.sampleRate      = extendedPrecisionFromUInt32(fmt.samplerate);

	AIFFSoundDataHeader soundData;
	soundData.offset    = 0U;
	soundData.blockSize = 0U;

	AIFFInstrument instrument;
	int numMarkers = 0;
	AIFFMarker markers[4];

	if (samplerPresent)
	{
		memset(&instrument, 0, sizeof(AIFFInstrument));
		memset(&markers, 0, sizeof(AIFFMarker));

		instrument.baseNote     = MIN(smpl.midiUnityNote, 127);
		instrument.detune       =   0;
		instrument.lowNote      =   0;
		instrument.highNote     = 127;
		instrument.lowVelocity  =   1;
		instrument.highVelocity = 127;
		instrument.gain         =   0;

		// Follow OpenMPT rules for interpreting sampler chunk loops:
		// A single loop is a "release" loop, two or more loops are
		//  interpreted as a sustain followed by a release loop. A bogus
		//  second loop is interpreted as wanting only wanting sustain.
		// RIFF Sampler loops ends are inclusive, AIFF marker points fall
		//  between frames, so end points must be offset to be exclusive.
		memset(markers, 0, sizeof(markers));
		int loopIdx = 0, loopMarkID = numMarkers + 1;
		if (smpl.sampleLoopCount > 1)
		{
			uint32_t loopStart = loops[loopIdx].loopStart;
			uint32_t loopEnd = MIN(loops[loopIdx].loopEnd + 1, common.numSampleFrames);
			instrument.sustainLoop.playMode = aiffPlayModeFromSmplLoopType(loops[loopIdx++].type);
			instrument.sustainLoop.beginLoop = loopMarkID;
			markers[numMarkers++] = (AIFFMarker){ .id = loopMarkID++, .name = "beg sus", .position = loopStart };
			instrument.sustainLoop.endLoop = loopMarkID;
			markers[numMarkers++] = (AIFFMarker){ .id = loopMarkID++, .name = "end sus", .position = loopEnd };
		}
		if (smpl.sampleLoopCount == 1 || (loops[loopIdx].loopEnd && loops[loopIdx].loopStart < loops[loopIdx].loopEnd))
		{
			uint32_t loopStart = loops[loopIdx].loopStart;
			uint32_t loopEnd = MIN(loops[loopIdx].loopEnd + 1, common.numSampleFrames);
			instrument.releaseLoop.playMode = aiffPlayModeFromSmplLoopType(loops[loopIdx++].type);
			instrument.releaseLoop.beginLoop = loopMarkID;
			markers[numMarkers++] = (AIFFMarker){ .id = loopMarkID++, .name = "beg loop", .position = loopStart };
			instrument.releaseLoop.endLoop = loopMarkID;
			markers[numMarkers++] = (AIFFMarker){ .id = loopMarkID++, .name = "end loop", .position = loopEnd };
		}
	}

	// Calculate FORM chunk size
	size_t markerChunkSz;
	size_t formSize = 4 + 8 * 2 + AIFF_COMMON_SIZE + AIFF_SOUNDDATAHEADER_SIZE;
	formSize += (uint32_t)dataBytes + (dataBytes & 0x1);
	if (numMarkers)
	{
		markerChunkSz = 2;
		for (int i = 0; i < numMarkers; ++i)
			markerChunkSz += 6 + ((2 + strlen(markers[i].name)) & (SIZE_MAX - 1));
		formSize += 8 + markerChunkSz;
	}
	if (samplerPresent)
		formSize += 8 + AIFF_INSTRUMENT_SIZE;

	// Write FORM header
	writeFourCC(out, "FORM");
	streamWriteU32be(out, (uint32_t)formSize);
	writeFourCC(out, "AIFF");

	// Write Common chunk
	writeFourCC(out, "COMM");
	streamWriteU32be(out, AIFF_COMMON_SIZE);
	streamWriteI16be(out, common.numChannels);
	streamWriteU32be(out, common.numSampleFrames);
	streamWriteI16be(out, common.sampleSize);
	writeExtendedBe(out, common.sampleRate);

	// Write Sound Data chunk & audio data
	writeFourCC(out, "SSND");
	const uint32_t ssndChunkSize = AIFF_SOUNDDATAHEADER_SIZE + (uint32_t)dataBytes;
	streamWriteU32be(out, ssndChunkSize);
	streamWriteU32be(out, soundData.offset);
	streamWriteU32be(out, soundData.blockSize);
	streamSeek(in, dataOffset, STREAM_SEEK_SET);
#define BLOCK_SIZE (1024 * 16)
	uint8_t buffer[BLOCK_SIZE];
	do
	{
		size_t bufferSize = MIN(BLOCK_SIZE, dataBytes);
		size_t frames = bufferSize / byteDepth;
		size_t bytesRead = streamRead(in, buffer, byteDepth, frames);
		if (byteDepth == 1)
		{
			// 8-bit samples are stored unsigned in WAVE for some reason,
			//  so convert each sample to avoid ear sadness.
			for (size_t i = 0; i < frames; ++i)
				buffer[i] ^= 0x80;
		}
		// WAVE stores 2-byte & larger samples in little endian, so we must
		//  convert them to big-endian before writing them to AIFF.
		else if (byteDepth == 2)
		{
			uint16_t* samples = (uint16_t*)buffer;
			for (size_t i = 0; i < frames; ++i)
				samples[i] = swap16(samples[i]);
		}
		else if (byteDepth == 3)
		{
			for (size_t i = 0; i < frames; ++i)
			{
				uint8_t tmp = buffer[i * 3];
				buffer[i * 3 + 0] = buffer[i * 3 + 2];
				buffer[i * 3 + 2] = tmp;
			}
		}
		else if (byteDepth == 4)
		{
			uint32_t* samples = (uint32_t*)buffer;
			for (size_t i = 0; i < frames; ++i)
				samples[i] = swap32(samples[i]);
		}
		streamWrite(out, buffer, byteDepth, frames);
		dataBytes -= bufferSize;
	}
	while (dataBytes > 0);
	if (ssndChunkSize & 0x1)  // Pad uneven chunk as IFF requires
		streamPutC(out, '\0');

	// Write Marker chunk & markers
	if (numMarkers)
	{
		writeFourCC(out, "MARK");
		streamWriteU32be(out, (uint32_t)markerChunkSz);
		streamWriteU16be(out, (uint16_t)numMarkers);
		for (int i = 0; i < numMarkers; ++i)
		{
			const AIFFMarker* marker = &markers[i];
			streamWriteI16be(out, marker->id);
			streamWriteI32be(out, marker->position);
			writePascalString(out, marker->name);
		}
	}

	// Write Instrument chunk
	if (samplerPresent)
	{
		writeFourCC(out, "INST");
		streamWriteU32be(out, AIFF_INSTRUMENT_SIZE);
		streamWrite(out, &instrument.baseNote, 1, 1);
		streamWrite(out, &instrument.detune, 1, 1);
		streamWrite(out, &instrument.lowNote, 1, 1);
		streamWrite(out, &instrument.highNote, 1, 1);
		streamWrite(out, &instrument.lowVelocity, 1, 1);
		streamWrite(out, &instrument.highVelocity, 1, 1);
		streamWriteI16be(out, instrument.gain);
		streamWriteI16be(out, instrument.sustainLoop.playMode);
		streamWriteI16be(out, instrument.sustainLoop.beginLoop);
		streamWriteI16be(out, instrument.sustainLoop.endLoop);
		streamWriteI16be(out, instrument.releaseLoop.playMode);
		streamWriteI16be(out, instrument.releaseLoop.beginLoop);
		streamWriteI16be(out, instrument.releaseLoop.endLoop);
	}

	streamClose(in);

	return 0;
}
