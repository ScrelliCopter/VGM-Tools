#ifndef NEOADPCMEXTRACT_H
#define NEOADPCMEXTRACT_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "stream.h"

typedef struct { void* data; size_t size, reserved; } Buffer;
#define BUFFER_CLEAR() { NULL, 0, 0 }

bool bufferResize(Buffer* buf, size_t size);

int vgmReadSample(StreamHandle fin, Buffer* restrict buf);
int vgmScanSample(StreamHandle file);

int streamGzFileOpen(StreamHandle* restrict outHnd,
	const char* restrict path,
	const char* restrict mode);

#endif//NEOADPCMEXTRACT_H
