/* adpcm.c
* original ADPCM to PCM converter v 1.01 By MARTINEZ Fabrice aka SNK of SUPREMACY
*/

#include "adpcm.h"
#include "util.h"
#include "wave.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define BUFFER_SIZE (1024 * 256)
#define ADPCMA_VOLUME_RATE 1
#define ADPCMA_DECODE_RANGE 1024
#define ADPCMA_DECODE_MIN (-(ADPCMA_DECODE_RANGE * ADPCMA_VOLUME_RATE))
#define ADPCMA_DECODE_MAX ((ADPCMA_DECODE_RANGE * ADPCMA_VOLUME_RATE) - 1)


static const int decodeTableA1[16] =
{
	-1 * 16, -1 * 16, -1 * 16, -1 * 16, 2 * 16, 5 * 16, 7 * 16, 9 * 16,
	-1 * 16, -1 * 16, -1 * 16, -1 * 16, 2 * 16, 5 * 16, 7 * 16, 9 * 16
};

int	main(int argc, char* argv[])
{
	fprintf(stderr, "**** ADPCM to PCM converter v 1.01\n\n");
	if (argc != 3)
	{
		fprintf(stderr, "USAGE: adpcm <InputFile.pcm> <OutputFile.wav>\n");
		return -1;
	}

	FILE* inFile = fopen(argv[1], "rb");
	if (!inFile)
	{
		fprintf(stderr, "Could not open inputfile %s\n", argv[1]);
		return -2;
	}

	FILE* outFile = fopen(argv[2], "wb");
	if (!outFile)
	{
		fprintf(stderr, "Could not open outputfile %s\n", argv[2]);
		return -3;
	}

	char* InputBuffer = malloc(BUFFER_SIZE);
	if (InputBuffer == NULL)
	{
		fprintf(stderr, "Could not allocate input buffer. (%d bytes)\n", BUFFER_SIZE);
		return -4;
	}

	short* OutputBuffer = malloc(BUFFER_SIZE * 4);
	if (OutputBuffer == NULL)
	{
		fprintf(stderr, "Could not allocate output buffer. (%d bytes)\n", BUFFER_SIZE * 4);
		return -5;
	}

	AdpcmADecoderState decoder;
	adpcmAInit(&decoder);

	fseek(inFile, 0, SEEK_END);
	unsigned int Filelen = ftell(inFile);
	fseek(inFile, 0, SEEK_SET);

	// Write wave header
	waveWrite(&(const WaveSpec)
	{
		.format    = WAVE_FMT_PCM,
		.channels  = 1,
		.rate      = 18500,
		.bytedepth = 2
	},
	NULL, Filelen * 4, &waveStreamDefaultCb, outFile);

	// Convert ADPCM to PCM and write to wave
	int bytesRead;
	do
	{
		bytesRead = fread(InputBuffer, 1, BUFFER_SIZE, inFile);
		if (bytesRead > 0)
		{
			adpcmADecode(&decoder, InputBuffer, OutputBuffer, bytesRead);
			fwrite(OutputBuffer, bytesRead * 4, 1, outFile);
		}
	}
	while (bytesRead == BUFFER_SIZE);

	free(OutputBuffer);
	free(InputBuffer);
	fclose(outFile);
	fclose(inFile);

	fprintf(stderr, "Done...\n");

	return 0;
}

void adpcmAInit(AdpcmADecoderState* decoder)
{
	for (int step = 0; step <= 48; step++)
	{
		int stepval = floor(16.0 * pow(11.0 / 10.0, step) * ADPCMA_VOLUME_RATE);
		// Loop over all nibbles and compute the difference
		for (int nib = 0; nib < 16; nib++)
		{
			int value = stepval * ((nib & 0x07) * 2 + 1) / 8;
			decoder->jediTable[step * 16 + nib] = (nib & 0x08) ? -value : value;
		}
	}

	decoder->delta = 0;
	decoder->cursignal = 0;
}

void adpcmADecode(AdpcmADecoderState* restrict decoder, char* restrict in, short* restrict out, int len)
{
	for (int i = 0; i < len * 2; ++i)
	{
		int data = (!(i & 0x1) ? ((*in) >> 4) : (*in++)) & 0x0F;
		int oldsignal = decoder->cursignal;
		decoder->cursignal = CLAMP(decoder->cursignal + decoder->jediTable[data + decoder->delta],
			ADPCMA_DECODE_MIN, ADPCMA_DECODE_MAX);
		decoder->delta = CLAMP(decoder->delta + decodeTableA1[data], 0 * 16, 48 * 16);
		if (abs(oldsignal - decoder->cursignal) > 2500)
		{
			fprintf(stderr, "WARNING: Suspicious signal evolution %06x,%06x pos:%06x delta:%06x\n",
				oldsignal, decoder->cursignal, i % len, decoder->delta);
			fprintf(stderr, "data:%02x dx:%08x\n",
				data, decoder->jediTable[data + decoder->delta]);
		}
		*(out++) = (decoder->cursignal & 0xffff) * 32;
	}
}
