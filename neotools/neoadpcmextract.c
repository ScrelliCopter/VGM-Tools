/* neoadpcmextract.c (C) 2017, 2019, 2020, 2023 a dinosaur (zlib) */

#include "neoadpcmextract.h"
#include "adpcm.h"
#include "adpcmb.h"
#include "wave.h"
#include "endian.h"
#include "util.h"
#include <stdlib.h>
#include <stdio.h>


bool bufferResize(Buffer* buf, size_t size)
{
	if (!buf)
		return false;
	buf->size = size;
	if (!buf->data || buf->reserved < size)
	{
		free(buf->data);
		buf->reserved = size;
		buf->data = malloc(size);
		if (!buf->data)
			return false;
	}
	return true;
}

int vgmReadSample(StreamHandle fin, Buffer* restrict buf)
{
	uint32_t sampLen;
	streamReadU32le(fin, &sampLen, 1);       // Get sample data length
	if (sampLen <= 8)
		return 1;
	sampLen -= 8;

	if (!bufferResize(buf, sampLen))         // Resize buffer if needed
		return false;
	streamSkip(fin, 8);                      // Ignore 8 bytes
	streamRead(fin, buf->data, 1, sampLen);  // Read adpcm data
	return 0;
}

int vgmScanSample(StreamHandle file)
{
	// Scan for pcm headers
	while (true)
	{
		if (file.cb->eof(file.user) || file.cb->error(file.user))
			return 0;
		if (streamGetC(file) != 0x67 || streamGetC(file) != 0x66)  // Match data block
			continue;
		switch (streamGetC(file))
		{
		case 0x82: return 'A';  // 67 66 82 - ADPCM-A
		case 0x83: return 'B';  // 67 66 83 - ADPCM-B
		default: return 0;
		}
	}
}


#define DECODE_BUFFER_SIZE 0x4000

int writeAdpcmA(int id, const Buffer* enc, Buffer* pcm)
{
	char name[32];
	snprintf(name, sizeof(name), "smpa_%02x.wav", id);
	StreamHandle fout;
	if (streamFileOpen(&fout, name, "wb"))
		return 1;

	// Write wave header
	const uint32_t decodedSize = enc->size * 2 * sizeof(short);
	waveWrite(&(const WaveSpec)
	{
		.format    = WAVE_FMT_PCM,
		.channels  = 1,
		.rate      = 18500,
		.bytedepth = 2
	},
	NULL, decodedSize, fout);

	bufferResize(pcm, DECODE_BUFFER_SIZE * 2 * sizeof(short));
	AdpcmADecoderState decoder;
	adpcmAInit(&decoder);
	size_t decoded = 0;
	do
	{
		const size_t blockSize = MIN(enc->size - decoded, DECODE_BUFFER_SIZE);
		adpcmADecode(&decoder, &((const char*)enc->data)[decoded], (short*)pcm->data, blockSize);
		streamWrite(fout, pcm->data, sizeof(short), blockSize * 2);
		decoded += DECODE_BUFFER_SIZE;
	}
	while (decoded < enc->size);

	streamClose(fout);
	fprintf(stderr, "Wrote \"%s\"\n", name);
	return 0;
}

int writeAdpcmB(int id, const Buffer* enc, Buffer* pcm)
{
	char name[32];
	snprintf(name, sizeof(name), "smpb_%02x.wav", id);
	StreamHandle fout;
	if (streamFileOpen(&fout, name, "wb"))
		return 1;

	// Write wave header
	const uint32_t decodedSize = enc->size * 2 * sizeof(short);
	waveWrite(&(const WaveSpec)
	{
		.format    = WAVE_FMT_PCM,
		.channels  = 1,
		.rate      = 22050,
		.bytedepth = 2
	},
	NULL, decodedSize, fout);

	bufferResize(pcm, DECODE_BUFFER_SIZE * 2 * sizeof(short));
	AdpcmBDecoderState decoder;
	adpcmBDecoderInit(&decoder);
	size_t decoded = 0;
	do
	{
		const size_t blockSize = MIN(enc->size - decoded, DECODE_BUFFER_SIZE);
		adpcmBDecode(&decoder, &((const uint8_t*)enc->data)[decoded], (int16_t*)pcm->data, blockSize);
		streamWrite(fout, pcm->data, sizeof(int16_t), blockSize * 2);
		decoded += DECODE_BUFFER_SIZE;
	}
	while (decoded < enc->size);

	streamClose(fout);
	fprintf(stderr, "Wrote \"%s\"\n", name);
	return 0;
}


int main(int argc, char** argv)
{
	if (argc != 2)
		return 1;

	StreamHandle file; // Open file
	if (streamGzFileOpen(&file, argv[1], "rb"))
		return 1;

#if !USE_ZLIB
	if (streamGetC(file) == 0x1F && streamGetC(file) == 0x8B)
	{
		fprintf(stderr, "I'm a little gzip short and stout\n");
		return 2;
	}
	streamSeek(file, 0, STREAM_SEEK_SET);
#endif

	Buffer rawbuf = BUFFER_CLEAR(), decbuf = BUFFER_CLEAR();
	int smpaCount = 0, smpbCount = 0;

	// Find ADCPM samples
	int scanType;
	while ((scanType = vgmScanSample(file)))
	{
		size_t offset;
		if (streamTell(file, &offset))
			fprintf(stderr, "ADPCM-%c data found at 0x%08zX\n", scanType, offset);

		if (vgmReadSample(file, &rawbuf) || rawbuf.size == 0)
			continue;

		if (scanType == 'A')
			writeAdpcmA(smpaCount++, &rawbuf, &decbuf);
		else if (scanType == 'B')
			writeAdpcmB(smpbCount++, &rawbuf, &decbuf);
	}

	free(rawbuf.data);
	streamClose(file);
	return 0;
}
