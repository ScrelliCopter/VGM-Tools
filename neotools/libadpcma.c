/* libadpcma.c (C) 2023 a dinosaur (zlib)
   Original ADPCM to PCM converter v 1.01 By MARTINEZ Fabrice aka SNK of SUPREMACY */

#include "adpcm.h"
#include "util.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#define ADPCMA_VOLUME_RATE 1
#define ADPCMA_DECODE_RANGE 1024
#define ADPCMA_DECODE_MIN (-(ADPCMA_DECODE_RANGE * ADPCMA_VOLUME_RATE))
#define ADPCMA_DECODE_MAX ((ADPCMA_DECODE_RANGE * ADPCMA_VOLUME_RATE) - 1)


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

static const int decodeTableA1[16] =
{
	-1 * 16, -1 * 16, -1 * 16, -1 * 16, 2 * 16, 5 * 16, 7 * 16, 9 * 16,
	-1 * 16, -1 * 16, -1 * 16, -1 * 16, 2 * 16, 5 * 16, 7 * 16, 9 * 16
};

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
