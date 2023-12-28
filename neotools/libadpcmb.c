/* libadpcmb.c (C) 2023 a dinosaur (zlib)

   ** YM2610 ADPCM-B Codec **
   PCM to ADPCM-B & ADPCM-B to PCM converters for NEO-GEO System
   ADPCM-B - 1 channel 1.8-55.5 KHz, 16 MB Sample ROM size,
   256 B min size of sample, 16 MB max, compatable with YM2608

   http://www.raregame.ru/file/15/YM2610.pdf     YM2610 DATASHEET
*/

#include "adpcmb.h"
#include "util.h"
#include <stdlib.h>


static const long stepSizeTable[16] =
{
	 57,  57,  57,  57,  77, 102, 128, 153,
	 57,  57,  57,  57,  77, 102, 128, 153
};

void adpcmBEncoderInit(AdpcmBEncoderState* encoder)
{
	encoder->xn = 0;
	encoder->stepSize = 127;
	encoder->flag = false;
	encoder->adpcmPack = 0;
}

void adpcmBEncode(AdpcmBEncoderState* encoder, const int16_t* restrict in, uint8_t* restrict out, int len)
{
	for (int lpc = 0; lpc < len; ++lpc)
	{
		long dn = (*in++) - encoder->xn;

		long i = (labs(dn) << 16) / (encoder->stepSize << 14);
		i = MIN(i, 7);
		uint8_t adpcm = i;

		i = (adpcm * 2 + 1) * encoder->stepSize / 8;

		if (dn < 0)
		{
			adpcm |= 0x8;
			encoder->xn -= i;
		}
		else
		{
			encoder->xn += i;
		}

		encoder->stepSize = (stepSizeTable[adpcm] * encoder->stepSize) / 64;
		encoder->stepSize = CLAMP(encoder->stepSize, 127, 24576);

		if (!encoder->flag)
		{
			encoder->adpcmPack = adpcm << 4;
			encoder->flag = true;
		}
		else
		{
			(*out++) = encoder->adpcmPack |= adpcm;
			encoder->flag = false;
		}
	}
}

void adpcmBDecoderInit(AdpcmBDecoderState* decoder)
{
	decoder->xn = 0;
	decoder->stepSize = 127;
}

void adpcmBDecode(AdpcmBDecoderState* decoder, const uint8_t* restrict in, int16_t* restrict out, int len)
{
	for (long lpc = 0; lpc < len * 2; ++lpc)
	{
		uint8_t adpcm = (!(lpc & 0x1) ? (*in) >> 4 : (*in++)) & 0xF;

		long i = ((adpcm & 7) * 2 + 1) * decoder->stepSize / 8;
		if (adpcm & 8)
			decoder->xn -= i;
		else
			decoder->xn += i;
		decoder->xn = CLAMP(decoder->xn, -32768, 32767);

		decoder->stepSize = decoder->stepSize * stepSizeTable[adpcm] / 64;
		decoder->stepSize = CLAMP(decoder->stepSize, 127, 24576);

		(*out++) = (int16_t)decoder->xn;
	}
}
