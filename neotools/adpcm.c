/* adpcm.c
* original ADPCM to PCM converter v 1.01 By MARTINEZ Fabrice aka SNK of SUPREMACY
*/

#include "adpcm.h"
#include "wave.h"
#include <stdio.h>
#include <stdlib.h>

#define BUFFER_SIZE (1024 * 256)


int	main(int argc, char* argv[])
{
	fprintf(stderr, "**** ADPCM to PCM converter v 1.01\n\n");
	if (argc != 3)
	{
		fprintf(stderr, "USAGE: adpcm <InputFile.pcm> <OutputFile.wav>\n");
		return -1;
	}

	FILE* inFile = fopen(argv[1], "rb");
	if (!inFile)
	{
		fprintf(stderr, "Could not open inputfile %s\n", argv[1]);
		return -2;
	}

	FILE* outFile = fopen(argv[2], "wb");
	if (!outFile)
	{
		fprintf(stderr, "Could not open outputfile %s\n", argv[2]);
		return -3;
	}

	char* InputBuffer = malloc(BUFFER_SIZE);
	if (InputBuffer == NULL)
	{
		fprintf(stderr, "Could not allocate input buffer. (%d bytes)\n", BUFFER_SIZE);
		return -4;
	}

	short* OutputBuffer = malloc(BUFFER_SIZE * 4);
	if (OutputBuffer == NULL)
	{
		fprintf(stderr, "Could not allocate output buffer. (%d bytes)\n", BUFFER_SIZE * 4);
		return -5;
	}

	AdpcmADecoderState decoder;
	adpcmAInit(&decoder);

	fseek(inFile, 0, SEEK_END);
	unsigned int Filelen = ftell(inFile);
	fseek(inFile, 0, SEEK_SET);

	// Write wave header
	waveWrite(&(const WaveSpec)
	{
		.format    = WAVE_FMT_PCM,
		.channels  = 1,
		.rate      = 18500,
		.bytedepth = 2
	},
	NULL, Filelen * 4, &waveStreamDefaultCb, outFile);

	// Convert ADPCM to PCM and write to wave
	int bytesRead;
	do
	{
		bytesRead = fread(InputBuffer, 1, BUFFER_SIZE, inFile);
		if (bytesRead > 0)
		{
			adpcmADecode(&decoder, InputBuffer, OutputBuffer, bytesRead);
			fwrite(OutputBuffer, bytesRead * 4, 1, outFile);
		}
	}
	while (bytesRead == BUFFER_SIZE);

	free(OutputBuffer);
	free(InputBuffer);
	fclose(outFile);
	fclose(inFile);

	fprintf(stderr, "Done...\n");

	return 0;
}
