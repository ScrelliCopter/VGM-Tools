/* gzstreamfile.c (C) 2023, 2025 a dinosaur (zlib) */

#include "neoadpcmextract.h"
#include <errno.h>

#if USE_ZLIB
# include <zlib.h>


static size_t streamGzFileRead(void* restrict user, void* restrict out, size_t size, size_t num)
{
	assert(user);
	return gzfread(out, size, num, (gzFile)user);
}

static int streamGzGetC(void* restrict user)
{
	assert(user);
	return gzgetc((gzFile)user);
}

static bool streamGzFileSeek(void* restrict user, long offset, StreamWhence whence)
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
	return gzseek((gzFile)user, offset, seek) ? false : true;
}

static bool streamGzFileTell(void* restrict user, size_t* restrict outPosition)
{
	assert(user);
	const long pos = gztell((gzFile)user);
	if (pos == -1L)
		return false;
	*outPosition = (size_t)pos;
	return true;
}

static bool streamGzFileEof(void* restrict user)
{
	assert(user);
	return gzeof((gzFile)user);
}

static bool streamGzFileError(void* restrict user)
{
	assert(user);
	int err;
	gzerror((gzFile)user, &err);
	return err;
}

static void streamGzFileClose(void* restrict user)
{
	gzclose(user);
}

static const StreamIoCb streamFileCb =
{
	.read  = streamGzFileRead,
	.write = NULL,
	.getc  = streamGzGetC,
	.putc  = NULL,
	.seek  = streamGzFileSeek,
	.tell  = streamGzFileTell,
	.eof   = streamGzFileEof,
	.error = streamGzFileError,
	.close = streamGzFileClose
};


int streamGzFileOpen(StreamHandle* restrict outHnd,
	const char* restrict path,
	const char* restrict mode)
{
	assert(outHnd);
	errno = 0;
	gzFile file = gzopen(path, mode);
	if (file == Z_NULL)
	{
		int err = errno;
		if (err == 0)
			err = -1;
		return err;
	}

	(*outHnd) = (StreamHandle)
	{
		.user = (void*)file,
		.cb = (const StreamIoCb* restrict)&streamFileCb
	};
	return 0;
}

#endif
