/* dspdecode.c (c) 2023 a dinosaur (zlib) */

#include "dsptool.h"
#include "wave.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct
{
	uint32_t numSamples;
	uint32_t numNibbles;
	uint32_t sampleRate;
	uint16_t loopFlag;
	uint16_t format; // Reserved, always 0
	uint32_t loopBeg;
	uint32_t loopEnd;
	uint32_t curAddress; // Reserved, always 0
	int16_t coefs[16];
	uint16_t gain; // Always 0
	uint16_t predScale;
	int16_t history[2];
	uint16_t loopPredScale;
	int16_t loopHistory[2];
	int16_t channels;
	uint16_t blockSize;
	uint16_t reserved1[9];

} DspHeader;

static void readBeU32(uint32_t* restrict dst, size_t count, FILE* restrict file)
{
	fread(dst, sizeof(uint32_t), count, file);
#if BYTE_ORDER == LITTLE_ENDIAN
	for (size_t i = 0; i < count; ++i)
		dst[i] = swap32(dst[i]);
#endif
}

static void readBeU16(uint16_t* restrict dst, size_t count, FILE* restrict file)
{
	fread(dst, sizeof(uint16_t), count, file);
#if BYTE_ORDER == LITTLE_ENDIAN
	for (size_t i = 0; i < count; ++i)
		dst[i] = swap16(dst[i]);
#endif
}

static void readBeS16(int16_t* restrict dst, size_t count, FILE* restrict file)
{
	fread(dst, sizeof(int16_t), count, file);
#if BYTE_ORDER == LITTLE_ENDIAN
	for (size_t i = 0; i < count; ++i)
		dst[i] = (int16_t)swap16((uint16_t)dst[i]);
#endif
}

typedef struct
{
	int16_t* pcm;
	size_t pcmSize;
	uint32_t rate;
} PcmFile;

#define PCMFILE_CLEAR() (PcmFile){ NULL, 0, 0 }

static int loadDsp(const char* path, PcmFile* out)
{
	FILE* file = fopen(path, "rb");
	if (!file)
	{
		fprintf(stderr, "File not found\n");
		return 1;
	}

	uint8_t* adpcm = NULL;
	out->pcm = NULL;

	DspHeader dsp;
	readBeU32(&dsp.numSamples,     1, file);
	readBeU32(&dsp.numNibbles,     1, file);
	readBeU32(&dsp.sampleRate,     1, file);
	readBeU16(&dsp.loopFlag,       1, file);
	readBeU16(&dsp.format,         1, file);
	readBeU32(&dsp.loopBeg,        1, file);
	readBeU32(&dsp.loopEnd,        1, file);
	readBeU32(&dsp.curAddress,     1, file);
	readBeS16(dsp.coefs,          16, file);
	readBeU16(&dsp.gain,           1, file);
	readBeU16(&dsp.predScale,      1, file);
	readBeS16(dsp.history,         2, file);
	readBeU16(&dsp.loopPredScale,  1, file);
	readBeS16(dsp.loopHistory,     2, file);
	readBeS16(&dsp.channels,       1, file);
	readBeU16(&dsp.blockSize,      1, file);
	readBeU16(dsp.reserved1,       9, file);

	if (dsp.loopFlag > 1 || dsp.format)
		goto Fail;

	size_t adpcmSize = getBytesForAdpcmBuffer(dsp.numSamples);
	adpcm = malloc(adpcmSize);
	if (!adpcm)
		goto Fail;
	out->pcmSize = getBytesForPcmBuffer(dsp.numSamples);
	out->pcm = malloc(out->pcmSize * sizeof(int16_t));
	if (!out->pcm)
		goto Fail;

	fread(adpcm, 1, adpcmSize, file);
	fclose(file);
	file = NULL;

	ADPCMINFO adpcmInfo =
	{
		.gain = dsp.gain,
		.pred_scale = dsp.predScale,
		.yn1 = dsp.history[0],
		.yn2 = dsp.history[1],
		.loop_pred_scale = dsp.loopPredScale,
		.loop_yn1 = dsp.loopHistory[0],
		.loop_yn2 = dsp.loopHistory[1]
	};
	memcpy(adpcmInfo.coef, dsp.coefs, sizeof(int16_t) * 16);
	decode(adpcm, out->pcm, &adpcmInfo, dsp.numSamples);

	free(adpcm);

	out->rate = dsp.sampleRate;
	return 0;

Fail:
	free(out->pcm);
	free(adpcm);
	if (file)
		fclose(file);
	return 1;
}

static void usage(const char* argv0)
{
	fprintf(stderr, "Usage: %s <in.dsp> [inR.dsp] [-o out.wav]\n", argv0);
	exit(1);
}

int main(int argc, char* argv[])
{
	// Parse cli arguments
	char* inPathL = NULL, * inPathR = NULL, * outPath = NULL;
	bool opt = false, outPathAlloc = false;
	for (int i = 1; i < argc; ++i)
	{
		if (opt)
		{
			if (outPath)
				usage(argv[0]);
			outPath = argv[i];
			opt = false;
		}
		else if (argv[i][0] == '-')
		{
			if (argv[i][1] != 'o')
				usage(argv[0]);
			opt = true;
		}
		else if (!inPathL)
		{
			inPathL = argv[i];
		}
		else if (!inPathR)
		{
			inPathR = argv[i];
		}
		else
		{
			usage(argv[0]);
		}
	}

	if (opt || !inPathL)
		usage(argv[0]);

	// Compute output path if one wasn't specified
	if (!outPath)
	{
		const char* base = strrchr(inPathL, '/');
#ifdef _WIN32
		const char* base2 = strrchr(inPathL, '\\');
		if (base2 && base2 > base)
			base = base2;
#endif
		if (!base)
			base = inPathL;
		char* ext = strrchr(base, '.');
		size_t nameLen = strlen(inPathL);
		if (ext)
			nameLen -= strlen(ext);
		outPath = malloc(nameLen + 5);
		if (!outPath)
			return 2;
		memcpy(outPath, inPathL, nameLen);
		memcpy(outPath + nameLen, ".wav", 5);
		outPathAlloc = true;
	}

	// Convert left (and optionally right) channels to PCM, save as wave
	int ret;
	PcmFile left = PCMFILE_CLEAR(), right = PCMFILE_CLEAR();
	if ((ret = loadDsp(inPathL, &left)))
		goto Cleanup;
	if (inPathR)
	{
		if ((ret = loadDsp(inPathR, &right)))
			goto Cleanup;
		if (left.pcmSize != right.pcmSize || left.rate != right.rate)
		{
			fprintf(stderr, "Rate or length mismatch");
			ret = 1;
			goto Cleanup;
		}

		WaveSpec wav =
		{
			.format    = WAVE_FMT_PCM,
			.channels  = 2,
			.rate      = left.rate,
			.bytedepth = sizeof(int16_t)
		};
		if ((ret = waveWriteBlockFile(&wav, (const void*[2]){left.pcm, right.pcm}, left.pcmSize, outPath)))
			goto Cleanup;

		free(right.pcm);
		right = PCMFILE_CLEAR();
	}
	else
	{
		WaveSpec wav =
		{
			.format    = WAVE_FMT_PCM,
			.channels  = 1,
			.rate      = left.rate,
			.bytedepth = sizeof(int16_t)
		};
		if ((ret = waveWriteFile(&wav, left.pcm, left.pcmSize, outPath)))
			goto Cleanup;
	}

	free(left.pcm);
	left = PCMFILE_CLEAR();

	ret = 0;
Cleanup:
	free(right.pcm);
	free(left.pcm);
	if (outPathAlloc)
		free(outPath);

	return ret;
}
