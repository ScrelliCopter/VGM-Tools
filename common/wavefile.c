/* wavefile.c (c) 2023, 2025 a dinosaur (zlib) */

#include "wave.h"
#include <stdio.h>

static size_t waveFileRead(void* restrict user, void* restrict out, size_t size, size_t num)
{
	return fread(out, size, num, (FILE*)user);
}

static size_t waveFileWrite(void* restrict user, const void* restrict src, size_t size, size_t num)
{
	return fwrite(src, size, num, (FILE*)user);
}

static void waveFileSeek(void* restrict user, long offset, WaveStreamWhence whence)
{
	int seek;
	switch (whence)
	{
	case WAVE_SEEK_SET: seek = SEEK_SET; break;
	case WAVE_SEEK_CUR: seek = SEEK_CUR; break;
	case WAVE_SEEK_END: seek = SEEK_END; break;
	default: seek = -1;
	}
	fseek((FILE*)user, offset, seek);
}

static bool waveFileTell(void* restrict user, size_t* restrict result)
{
	const long pos = ftell((FILE*)user);
	if (pos == -1L)
		return false;
	*result = (size_t)pos;
	return true;
}

static bool waveFileEof(void* restrict user)
{
	return feof((FILE*)user);
}

static bool waveFileError(void* restrict user)
{
	return ferror((FILE*)user);
}

const WaveStreamCb waveStreamDefaultCb =
{
	.read  = waveFileRead,
	.write = waveFileWrite,
	.seek  = waveFileSeek,
	.tell  = waveFileTell,
	.eof   = waveFileEof,
	.error = waveFileError
};

int waveWriteFile(const WaveSpec* spec, const void* data, size_t dataLen, const char* path)
{
	FILE* file = fopen(path, "wb");
	if (!file)
		return 1;

	int res = waveWrite(spec, data, dataLen, &waveStreamDefaultCb, (void*)file);
	fclose(file);
	return res;
}

int waveWriteBlockFile(const WaveSpec* spec, const void* blocks[], size_t blockLen, const char* path)
{
	FILE* file = fopen(path, "wb");
	if (!file)
		return 1;

	int res = waveWriteBlock(spec, blocks, blockLen, &waveStreamDefaultCb, (void*)file);
	fclose(file);
	return res;
}
