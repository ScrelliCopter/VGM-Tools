#ifndef NEOADPCMEXTRACT_H
#define NEOADPCMEXTRACT_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#if USE_ZLIB
#include <zlib.h>
typedef struct gzFile_s nfile;
#  define nopen gzopen
#  define nclose gzclose
#  define nread gzfread
#  define ngetc gzgetc
#  define nseek gzseek
#  define ntell gztell
#  define neof gzeof
static inline int nerror(gzFile file) { int err; gzerror(file, &err); return err; }
#else
typedef FILE nfile;
#  define nopen fopen
#  define nclose fclose
#  define nread fread
#  define ngetc fgetc
#  define nseek fseek
#  define ntell ftell
#  define neof feof
#  define nerror ferror
#endif

typedef struct { void* data; size_t size, reserved; } Buffer;
#define BUFFER_CLEAR() { NULL, 0, 0 }

bool bufferResize(Buffer* buf, size_t size);

int vgmReadSample(nfile* restrict fin, Buffer* restrict buf);
int vgmScanSample(nfile* file);

#endif//NEOADPCMEXTRACT_H
