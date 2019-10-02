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

	int err = vgmExtractSamples(file);
	fclose(file);
	return err;
}
