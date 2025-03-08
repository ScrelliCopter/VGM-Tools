#ifndef WAVE_H
#define WAVE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stream.h"

typedef enum
{
	WAVESPEC_FORMAT_PCM   = 0x0001,
	WAVESPEC_FORMAT_FLOAT = 0x0003
} WaveSpecFormat;

typedef struct
{
	WaveSpecFormat format;
	int            channels;
	unsigned       rate;
	int            bytedepth;
} WaveSpec;

int waveWrite(const WaveSpec* spec, const void* data, size_t dataLen, StreamHandle hnd);
int waveWriteFile(const WaveSpec* spec, const void* data, size_t dataLen, const char* path);
int waveWriteBlock(const WaveSpec* spec, const void* blocks[], size_t blockLen, StreamHandle hnd);
int waveWriteBlockFile(const WaveSpec* spec, const void* blocks[], size_t blockLen, const char* path);

#ifdef __cplusplus
}
#endif

#endif//WAVE_H
