#include <stdio.h>
#include "neoadpcmextract.h"


int main(int argc, char** argv)
{
	if (argc != 2)
		return 1;

	// Open file.
	FILE* file = fopen(argv[1], "rb");
	if (!file)
		return 1;

	if (fgetc(file) == 0x1F && fgetc(file) == 0x8B)
	{
		printf("I'm a little gzip short and stout\n");
		return 2;
	}

	fseek(file, 0, SEEK_SET);

	int err = vgmExtractSamples(file);
	fclose(file);
	return err;
}
