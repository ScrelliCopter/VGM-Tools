#ifndef WAVE_H
#define WAVE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

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

typedef enum
{
	WAVE_SEEK_SET = 0, // From start of file
	WAVE_SEEK_CUR = 1, // From current position in file
	WAVE_SEEK_END = 2  // From EOF
} WaveStreamWhence;

typedef struct
{
	size_t (*read)(void* restrict user, void* restrict out, size_t size, size_t num);
	size_t (*write)(void* restrict user, const void* restrict src, size_t size, size_t num);
	void   (*seek)(void* restrict user, long offset, WaveStreamWhence whence);
	bool   (*tell)(void* restrict user, size_t* restrict result);
	bool   (*eof)(void* restrict user);
	bool   (*error)(void* restrict user);
} WaveStreamCb;

extern const WaveStreamCb waveStreamDefaultCb;

int waveWrite(const WaveSpec* spec, const void* data, size_t dataLen, const WaveStreamCb* cb, void* user);
int waveWriteFile(const WaveSpec* spec, const void* data, size_t dataLen, const char* path);
int waveWriteBlock(const WaveSpec* spec, const void* blocks[], size_t blockLen, const WaveStreamCb* cb, void* user);
int waveWriteBlockFile(const WaveSpec* spec, const void* blocks[], size_t blockLen, const char* path);

#ifdef __cplusplus
}
#endif

#endif//WAVE_H
