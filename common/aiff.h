#ifndef AIFF_H
#define AIFF_H

#include "stream.h"
#include "aiffdefs.h"

#define AIFF_BITDEPTH_TO_BYTES(BITS) (((BITS) + 7) >> 3U)

typedef enum AIFFChunkFlags
{
	AIFF_CHUNKFLAGS_FORM = 1 << 0,
	AIFF_CHUNKFLAGS_COMM = 1 << 1,
	AIFF_CHUNKFLAGS_SSND = 1 << 2,
	AIFF_CHUNKFLAGS_MARK = 1 << 3,
	AIFF_CHUNKFLAGS_INST = 1 << 4,
	AIFF_CHUNKFLAGS_MIDI = 1 << 5,
	AIFF_CHUNKFLAGS_AESD = 1 << 6,
	AIFF_CHUNKFLAGS_APPL = 1 << 7,
	AIFF_CHUNKFLAGS_COMT = 1 << 8

} AIFFChunkFlags;

typedef struct AIFFWriter
{
	StreamHandle hnd;
	AIFFChunkFlags chunks;
	AIFFChunkFlags lastChunk;
	size_t soundDataOffset;
	uint32_t markerChunkSize;
	uint32_t soundChunkSize;
	uint32_t formSize;
	size_t lastChunkSize;
	size_t writtenChunksSize;

} AIFFWriter;

void aiffWriterOpen(AIFFWriter* restrict writer, StreamHandle hnd);
int aiffWriterFileOpen(AIFFWriter* restrict writer, const char* restrict path);
void aiffWriterClose(AIFFWriter* restrict writer);

void aiffPrecalculateFormSize(AIFFWriter* restrict writer,
	AIFFChunkFlags chunks, AIFFChunkFlags lastChunk,
	const AIFFMarker markers[], uint16_t numMarkers,
	size_t frameSize, size_t frameCount);
void aiffWriteCommonChunk(AIFFWriter* writer,
	int16_t  channels,
	uint32_t frames,
	int16_t  bitDepth,
	uint32_t rate);
void aiffWriteSoundFrames(AIFFWriter* restrict writer, const uint8_t* restrict data, size_t depth, size_t count);
void aiffWriteMarkers(AIFFWriter* restrict writer, const AIFFMarker markers[], uint16_t num);
void aiffWriteInstrumentChunk(AIFFWriter* restrict writer, const AIFFInstrument* restrict instrument);

#endif//AIFF_H
