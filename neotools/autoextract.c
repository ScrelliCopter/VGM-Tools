#include <stdio.h>
#include <stdlib.h>
#include "neoadpcmextract.h"


int main(int argc, char** argv)
{
	if (argc != 2)
		return 1;

	// Open file.
	FILE* file = fopen(argv[1], "rb");
	if (!file)
		return 1;

	// Error on VGZ's for now.
	if (fgetc(file) == 0x1F && fgetc(file) == 0x8B)
	{
		printf("I'm a little gzip short and stout\n");
		return 2;
	}

	fseek(file, 0, SEEK_SET);

	Buffer smpbuf = {NULL, 0, 0};
	char name[32];
	int smpaCount = 0, smpbCount = 0;

	// Find ADCPM samples.
	int scanType;
	while ((scanType = vgmScanSample(file)))
	{
		if (scanType != 'A' && scanType != 'B')
			continue;
		fprintf(stderr, "ADPCM-%c data found at 0x%08lX\n", scanType, ftell(file));

		if (vgmReadSample(file, &smpbuf) || smpbuf.size == 0)
			continue;
		if (scanType == 'A')
		{
			// decode
			snprintf(name, sizeof(name), "smpa_%02x.pcm", smpaCount++);
			printf("./adpcm \"%s\" \"$WAVDIR/%s.wav\"\n", name, name);
		}
		else
		{
			// decode
			snprintf(name, sizeof(name), "smpb_%02x.pcm", smpbCount++);
			printf("./adpcmb -d \"%s\" \"$WAVDIR/%s.wav\"\n", name, name);
		}

		// Write adpcm sample.
		FILE* fout = fopen(name, "wb");
		if (!fout)
			continue;
		fwrite(smpbuf.data, sizeof(uint8_t), smpbuf.size, fout);
		fclose(fout);
	}

	free(smpbuf.data);
	fclose(file);
	return 0;
}
