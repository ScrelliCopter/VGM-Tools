/* aifsampconv.c (c) 2025 a dinosaur (zlib) */

#include "wave.h"
#include "aiff.h"
#include "wavedefs.h"
#include "endian.h"
#include "util.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <float.h>


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
	IffChunk riff;
	IffFourCC filetype;
	streamRead(in, riff.fourcc.c, 1, 4);
	streamReadU32le(in, &riff.size, 1);
	streamRead(in, filetype.c,    1, 4);

	if (!IFF_FOURCC_CMP(riff.fourcc, FOURCC_RIFF))
		return 1;
	if (riff.size < FORMAT_CHUNK_SIZE)
		return 1;
	if (!IFF_FOURCC_CMP(filetype, FOURCC_WAVE))
		return 1;

	FormatChunk fmt;
	SamplerChunk smpl;
	SampleLoopChunk loops[2];
	size_t dataOffset = 0, dataBytes = 0;
	bool samplerPresent = false;

	size_t bytes = 4;
	do
	{
		IffChunk chunk;
		memset(&chunk, 0, sizeof(IffChunk));
		streamRead(in, &chunk.fourcc, 1, 4);
		streamReadU32le(in, &chunk.size, 1);

		if (IFF_FOURCC_CMP(chunk.fourcc, WAVE_FOURCC_FMT))
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
		else if (IFF_FOURCC_CMP(chunk.fourcc, WAVE_FOURCC_SMPL))
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
		else if (IFF_FOURCC_CMP(chunk.fourcc, WAVE_FOURCC_DATA))
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

	AIFFWriter writer;
	const char* outName = argc > 2 ? argv[2] : "out.aif";
	if (aiffWriterFileOpen(&writer, outName))
	{
		streamClose(in);
		return 1;
	}

	const uint32_t byteDepth = AIFF_BITDEPTH_TO_BYTES((uint32_t)fmt.bitdepth);  //TODO: Prob not correct for wave
	const uint32_t bytesPerFrame = byteDepth * (uint32_t)fmt.channels;
	const size_t numFrames = dataBytes / bytesPerFrame;

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
			uint32_t loopEnd = MIN(loops[loopIdx].loopEnd + 1U, (uint32_t)numFrames);
			instrument.sustainLoop.playMode = aiffPlayModeFromSmplLoopType(loops[loopIdx++].type);
			instrument.sustainLoop.beginLoop = loopMarkID;
			markers[numMarkers++] = (AIFFMarker){ .id = loopMarkID++, .name = "beg sus", .position = loopStart };
			instrument.sustainLoop.endLoop = loopMarkID;
			markers[numMarkers++] = (AIFFMarker){ .id = loopMarkID++, .name = "end sus", .position = loopEnd };
		}
		if (smpl.sampleLoopCount == 1 || (loops[loopIdx].loopEnd && loops[loopIdx].loopStart < loops[loopIdx].loopEnd))
		{
			uint32_t loopStart = loops[loopIdx].loopStart;
			uint32_t loopEnd = MIN(loops[loopIdx].loopEnd + 1U, (uint32_t)numFrames);
			instrument.releaseLoop.playMode = aiffPlayModeFromSmplLoopType(loops[loopIdx++].type);
			instrument.releaseLoop.beginLoop = loopMarkID;
			markers[numMarkers++] = (AIFFMarker){ .id = loopMarkID++, .name = "beg loop", .position = loopStart };
			instrument.releaseLoop.endLoop = loopMarkID;
			markers[numMarkers++] = (AIFFMarker){ .id = loopMarkID++, .name = "end loop", .position = loopEnd };
		}
	}

	AIFFChunkFlags chunks = AIFF_CHUNKFLAGS_COMM | AIFF_CHUNKFLAGS_SSND;
	AIFFChunkFlags last = AIFF_CHUNKFLAGS_SSND;
	if (numMarkers)
	{
		chunks |= AIFF_CHUNKFLAGS_MARK;
		last = AIFF_CHUNKFLAGS_MARK;
	}
	if (samplerPresent)
	{
		chunks |= AIFF_CHUNKFLAGS_INST;
		last = AIFF_CHUNKFLAGS_INST;
	}
	aiffPrecalculateFormSize(&writer, chunks, last, markers, numMarkers, byteDepth, numFrames);

	// Write Common chunk
	aiffWriteCommonChunk(&writer,
		(int16_t)fmt.channels,
		(uint32_t)numFrames,
		(int16_t)fmt.bitdepth,
		fmt.samplerate);

	// Write Sound Data chunk & audio data
	streamSeek(in, dataOffset, STREAM_SEEK_SET);
#define BLOCK_SIZE (1024 * 16)
	uint8_t buffer[BLOCK_SIZE];
	do
	{
		const size_t bufferSize = MIN(BLOCK_SIZE, dataBytes);
		const size_t frames = bufferSize / byteDepth;
		const size_t bytesRead = streamRead(in, buffer, byteDepth, frames);
		if (byteDepth == 1)
		{
			// 8-bit samples are stored unsigned in WAVE for some reason,
			//  so convert each sample to avoid ear sadness.
			for (size_t i = 0; i < frames; ++i)
				buffer[i] ^= 0x80;
		}
		// WAVE stores >=2 byte samples in little endian, so we must
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
		aiffWriteSoundFrames(&writer, buffer, byteDepth, frames);
		dataBytes -= bufferSize;
	}
	while (dataBytes > 0);

	// Write Marker chunk & markers
	if (numMarkers)
		aiffWriteMarkers(&writer, markers, numMarkers);

	// Write Instrument chunk
	if (samplerPresent)
		aiffWriteInstrumentChunk(&writer, &instrument);

	aiffWriterClose(&writer);

	streamClose(in);

	return 0;
}
