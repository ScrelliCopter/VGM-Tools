#ifndef ADPCMB_H
#define ADPCMB_H

#include <stdint.h>
#include <stdbool.h>

typedef struct AdpcmBEncoderState
{
	bool flag;
	long xn, stepSize;
	uint8_t adpcmPack;
} AdpcmBEncoderState;

void adpcmBEncoderInit(AdpcmBEncoderState* encoder);
void adpcmBEncode(AdpcmBEncoderState* encoder, const int16_t* restrict in, uint8_t* restrict out, int len);

typedef struct AdpcmBDecoderState
{
	long xn, stepSize;
} AdpcmBDecoderState;

void adpcmBDecoderInit(AdpcmBDecoderState* decoder);
void adpcmBDecode(AdpcmBDecoderState* decoder, const uint8_t* restrict in, int16_t* restrict out, int len);

#endif//ADPCMB_H
