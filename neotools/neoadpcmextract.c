/* neoadpcmextract.c (C) 2017, 2019, 2020, 2023 a dinosaur (zlib) */

#include "neoadpcmextract.h"
#include "endian.h"
#include <stdlib.h>


static uint32_t read32le(nfile* fin)
{
	uint32_t tmp = 0;
	nread(&tmp, sizeof(uint32_t), 1, fin);
	return SWAP_LE32(tmp);
}

bool bufferResize(Buffer* buf, size_t size)
{
	if (!buf)
		return false;
	buf->size = size;
	if (!buf->data || buf->reserved < size)
	{
		free(buf->data);
		buf->reserved = size;
		buf->data = malloc(size);
		if (!buf->data)
			return false;
	}
	return true;
}

int vgmReadSample(nfile* restrict fin, Buffer* restrict buf)
{
	// Get sample data length
	uint32_t sampLen = read32le(fin);
	if (sampLen <= 8) return 1;
	sampLen -= 8;

	if (!bufferResize(buf, sampLen)) return false;    // Resize buffer if needed
	nseek(fin, 8, SEEK_CUR);                          // Ignore 8 bytes
	nread(buf->data, sizeof(uint8_t), sampLen, fin);  // Read adpcm data
	return 0;
}

int vgmScanSample(nfile* file)
{
	// Scan for pcm headers
	while (1)
	{
		if (neof(file) || nerror(file)) return 0;
		if (ngetc(file) != 0x67 || ngetc(file) != 0x66) continue;  // Match data block
		switch (ngetc(file))
		{
		case 0x82: return 'A';  // 67 66 82 - ADPCM-A
		case 0x83: return 'B';  // 67 66 83 - ADPCM-B
		default: return 0;
		}
	}
}
