/* streamfile.c (c) 2023, 2025 a dinosaur (zlib) */

#include "stream.h"
#include <stdio.h>
#include <errno.h>


static size_t streamFileRead(void* restrict user, void* restrict out, size_t size, size_t num)
{
	assert(user);
	return fread(out, size, num, (FILE*)user);
}

static size_t streamFileWrite(void* restrict user, const void* restrict src, size_t size, size_t num)
{
	assert(user);
	return fwrite(src, size, num, (FILE*)user);
}

static int streamFileGetC(void* restrict user)
{
	assert(user);
	return getc((FILE*)user);
}

static int streamFilePutC(void* restrict user, int c)
{
	assert(user);
	return putc(c, (FILE*)user);
}

static bool streamFileSeek(void* restrict user, long offset, StreamWhence whence)
{
	assert(user);
	int seek;
	switch (whence)
	{
	case STREAM_SEEK_SET: seek = SEEK_SET; break;
	case STREAM_SEEK_CUR: seek = SEEK_CUR; break;
	case STREAM_SEEK_END: seek = SEEK_END; break;
	default: return false;
	}
	return fseek((FILE*)user, offset, seek) ? false : true;
}

static bool streamFileTell(void* restrict user, size_t* restrict outPosition)
{
	assert(user);
	const long pos = ftell((FILE*)user);
	if (pos == -1L)
		return false;
	*outPosition = (size_t)pos;
	return true;
}

static bool streamFileEof(void* restrict user)
{
	assert(user);
	return feof((FILE*)user);
}

static bool streamFileError(void* restrict user)
{
	assert(user);
	return ferror((FILE*)user);
}

static void streamFileClose(void* restrict user)
{
	fclose((FILE*)user);
}

static const StreamIoCb streamFileCb =
{
	.read  = streamFileRead,
	.write = streamFileWrite,
	.getc  = streamFileGetC,
	.putc  = streamFilePutC,
	.seek  = streamFileSeek,
	.tell  = streamFileTell,
	.eof   = streamFileEof,
	.error = streamFileError,
	.close = streamFileClose
};


int streamFileOpen(StreamHandle* restrict outHnd, const char* restrict path, const char* restrict mode)
{
	assert(outHnd);
	FILE* file = fopen(path, mode);

	int err = 0;
	if (file)
	{
		(*outHnd) = (StreamHandle)
		{
			.user = (void*)file,
			.cb = (const StreamIoCb* restrict)&streamFileCb
		};
	}
	else
	{
		err = errno;
		errno = 0;
	}
	return err;
}
