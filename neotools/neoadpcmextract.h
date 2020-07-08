#ifndef __NEOADPCMEXTRACT_H__
#define __NEOADPCMEXTRACT_H__

#include <stdio.h>
#include <stdint.h>

typedef struct { uint8_t* data; size_t size, reserved; } Buffer;

int vgmReadSample(FILE* fin, Buffer* buf);
int vgmScanSample(FILE* file);

#endif//__NEOADPCMEXTRACT_H__
