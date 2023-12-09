/* neoadpcmextract.c (C) 2017, 2019, 2020, 2023 a dinosaur (zlib) */

#include "neoadpcmextract.h"
#include "endian.h"
#include <stdlib.h>
#include <stdio.h>


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


int main(int argc, char** argv)
{
	if (argc != 2) return 1;

	nfile* file = nopen(argv[1], "rb");  // Open file
	if (!file) return 1;

#if !USE_ZLIB
	if (ngetc(file) == 0x1F && ngetc(file) == 0x8B)
	{
		printf("I'm a little gzip short and stout\n");
		return 2;
	}
	nseek(file, 0, SEEK_SET);
#endif

	Buffer smpbuf = {NULL, 0, 0};
	char name[32];
	int smpaCount = 0, smpbCount = 0;

	// Find ADCPM samples
	int scanType;
	while ((scanType = vgmScanSample(file)))
	{
		if (scanType != 'A' && scanType != 'B')
			continue;
		fprintf(stderr, "ADPCM-%c data found at 0x%08lX\n", scanType, ntell(file));

		if (vgmReadSample(file, &smpbuf) || smpbuf.size == 0)
			continue;
		if (scanType == 'A')
		{
			snprintf(name, sizeof(name), "smpa_%02x.pcm", smpaCount++);
			printf("./adpcm \"%s\" \"$WAVDIR/%s.wav\"\n", name, name);
		}
		else
		{
			snprintf(name, sizeof(name), "smpb_%02x.pcm", smpbCount++);
			printf("./adpcmb -d \"%s\" \"$WAVDIR/%s.wav\"\n", name, name);
		}

		// Write ADPCM sample
		FILE* fout = fopen(name, "wb");
		if (!fout)
			continue;
		fwrite(smpbuf.data, sizeof(uint8_t), smpbuf.size, fout);
		fclose(fout);
	}

	free(smpbuf.data);
	nclose(file);
	return 0;
}
