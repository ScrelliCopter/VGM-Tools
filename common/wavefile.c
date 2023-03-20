/* wavefile.c (c) 2023 a dinosaur (zlib) */

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

static void waveFileSeek(void* restrict user, int offset)
{
#ifndef _MSC_VER
	fseek((FILE*)user, (off_t)offset, SEEK_CUR);
#else
	fseek((FILE*)user, (long)offset, SEEK_CUR);
#endif
}

static size_t waveFileTell(void* user)
{
	return ftell((FILE*)user);
}

static int waveFileEof(void* user)
{
	return feof((FILE*)user) || ferror((FILE*)user);
}

const WaveStreamCb waveStreamDefaultCb =
{
	.read  = waveFileRead,
	.write = waveFileWrite,
	.seek  = waveFileSeek,
	.tell  = waveFileTell,
	.eof   = waveFileEof
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
