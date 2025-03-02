/* gzstreamfile.c (C) 2023, 2025 a dinosaur (zlib) */

#include "neoadpcmextract.h"
#include <errno.h>

#if USE_ZLIB
# include <zlib.h>
typedef struct gzFile_s nfile;
# define nopen gzopen
# define nclose gzclose
# define nread gzfread
# define ngetc gzgetc
# define nseek gzseek
# define ntell gztell
# define neof gzeof
#else
typedef FILE nfile;
# define nopen fopen
# define nclose fclose
# define nread fread
# define ngetc fgetc
# define nseek fseek
# define ntell ftell
# define neof feof
#endif


static size_t streamGzFileRead(void* restrict user, void* restrict out, size_t size, size_t num)
{
	assert(user);
	return nread(out, size, num, (nfile*)user);
}

static int streamGzGetC(void* restrict user)
{
	assert(user);
	return ngetc((nfile*)user);
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
	return nseek((nfile*)user, offset, seek) ? false : true;
}

static bool streamGzFileTell(void* restrict user, size_t* restrict outPosition)
{
	assert(user);
	const long pos = ntell((nfile*)user);
	if (pos == -1L)
		return false;
	*outPosition = (size_t)pos;
	return true;
}

static bool streamGzFileEof(void* restrict user)
{
	assert(user);
	return neof((nfile*)user);
}

static bool streamGzFileError(void* restrict user)
{
	assert(user);
#ifdef USE_ZLIB
	int err;
	gzerror((gzFile)user, &err);
	return err;
#else
	return ferror((FILE*)user);
#endif
}

static void streamGzFileClose(void* restrict user)
{
	nclose(user);
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
	nfile* file = nopen(path, mode);

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
