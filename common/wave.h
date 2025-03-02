#ifndef WAVE_H
#define WAVE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stream.h"

typedef enum
{
	WAVE_FMT_PCM        = 0x0001,
	WAVE_FMT_IEEE_FLOAT = 0x0003,
	WAVE_FMT_ALAW       = 0x0006,
	WAVE_FMT_MULAW      = 0x0007,
	WAVE_FMT_EXTENSIBLE = 0xFFFE
} WaveFormat;

typedef struct
{
	WaveFormat format;
	int        channels;
	unsigned   rate;
	int        bytedepth;
} WaveSpec;

int waveWrite(const WaveSpec* spec, const void* data, size_t dataLen, StreamHandle hnd);
int waveWriteFile(const WaveSpec* spec, const void* data, size_t dataLen, const char* path);
int waveWriteBlock(const WaveSpec* spec, const void* blocks[], size_t blockLen, StreamHandle hnd);
int waveWriteBlockFile(const WaveSpec* spec, const void* blocks[], size_t blockLen, const char* path);

#ifdef __cplusplus
}
#endif

#endif//WAVE_H
