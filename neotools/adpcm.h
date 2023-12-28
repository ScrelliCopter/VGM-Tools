#ifndef ADPCM_H
#define ADPCM_H

typedef struct AdpcmADecoderState
{
	int jediTable[49 * 16];
	int cursignal;
	int delta;
} AdpcmADecoderState;

void adpcmAInit(AdpcmADecoderState* decoder);
void adpcmADecode(AdpcmADecoderState* decoder, const char* restrict in, short* restrict out, int len);

#endif//ADPCM_H
