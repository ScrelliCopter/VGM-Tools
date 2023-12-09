#include <stdio.h>
#include <stdlib.h>
#include "neoadpcmextract.h"


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
