/* adpcm.c
* original ADPCM to PCM converter v 1.01 By MARTINEZ Fabrice aka SNK of SUPREMACY
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "util.h"
#include "wave.h"

#define BUFFER_SIZE (1024 * 256)
#define ADPCMA_VOLUME_RATE 1
#define ADPCMA_DECODE_RANGE 1024
#define ADPCMA_DECODE_MIN (-(ADPCMA_DECODE_RANGE * ADPCMA_VOLUME_RATE))
#define ADPCMA_DECODE_MAX ((ADPCMA_DECODE_RANGE * ADPCMA_VOLUME_RATE) - 1)

static int decode_tableA1[16] =
{
	-1 * 16, -1 * 16, -1 * 16, -1 * 16, 2 * 16, 5 * 16, 7 * 16, 9 * 16,
	-1 * 16, -1 * 16, -1 * 16, -1 * 16, 2 * 16, 5 * 16, 7 * 16, 9 * 16
};

static int jedi_table[49 * 16];
static int cursignal;
static int delta;

void adpcm_init(void);
void adpcm_decode(void *, void *, int);

FILE* errorlog;

int	main(int argc, char *argv[])
{
	FILE *InputFile, *OutputFile;
	void *InputBuffer, *OutputBuffer;
	int bytesRead;
	unsigned int Filelen;

	puts("**** ADPCM to PCM converter v 1.01\n");
	if (argc != 3)
	{
		puts("USAGE: adpcm <InputFile.pcm> <OutputFile.wav>");
		exit(-1);
	}

	InputFile = fopen(argv[1], "rb");
	if (!InputFile)
	{
		printf("Could not open inputfile %s\n", argv[1]);
		exit(-2);
	}

	OutputFile = fopen(argv[2], "wb");
	if (!OutputFile)
	{
		printf("Could not open outputfile %s\n", argv[2]);
		exit(-3);
	}

	errorlog = fopen("error.log", "wb");

	InputBuffer = malloc(BUFFER_SIZE);
	if (InputBuffer == NULL)
	{
		printf("Could not allocate input buffer. (%d bytes)\n", BUFFER_SIZE);
		exit(-4);
	}

	OutputBuffer = malloc(BUFFER_SIZE * 10);
	if (OutputBuffer == NULL)
	{
		printf("Could not allocate output buffer. (%d bytes)\n", BUFFER_SIZE * 4);
		exit(-5);
	}

	adpcm_init();

	fseek(InputFile, 0, SEEK_END);
	Filelen = ftell(InputFile);
	fseek(InputFile, 0, SEEK_SET);

	// Write wave header
	waveWrite(&(const WaveSpec)
	{
		.format    = WAVE_FMT_PCM,
		.channels  = 1,
		.rate      = 18500,
		.bytedepth = 2
	},
	NULL, Filelen * 4, &waveStreamDefaultCb, OutputFile);

	// Convert ADPCM to PCM and write to wave
	do
	{
		bytesRead = fread(InputBuffer, 1, BUFFER_SIZE, InputFile);
		if (bytesRead > 0)
		{
			adpcm_decode(InputBuffer, OutputBuffer, bytesRead);
			fwrite(OutputBuffer, bytesRead * 4, 1, OutputFile);
		}
	} while (bytesRead == BUFFER_SIZE);

	free(InputBuffer);
	free(OutputBuffer);
	fclose(InputFile);
	fclose(OutputFile);

	puts("Done...");

	return 0;
}

void adpcm_init(void)
{
	int step, nib;

	for (step = 0; step <= 48; step++)
	{
		int stepval = floor(16.0 * pow (11.0 / 10.0, (double)step) * ADPCMA_VOLUME_RATE);
		// Loop over all nibbles and compute the difference
		for (nib = 0; nib < 16; nib++)
		{
			int value = stepval * ((nib & 0x07) * 2 + 1) / 8;
			jedi_table[step * 16 + nib] = (nib & 0x08) ? -value : value;
		}
	}

	delta = 0;
	cursignal = 0;
}

void adpcm_decode(void *InputBuffer, void *OutputBuffer, int Length)
{
	char  *in;
	short *out;
	int i, data, oldsignal;

	in = (char *)InputBuffer;
	out = (short *)OutputBuffer;

	for (i = 0; i < Length; i++)
	{
		data = ((*in) >> 4) & 0x0F;
		oldsignal = cursignal;
		cursignal = CLAMP(cursignal + (jedi_table[data + delta]), ADPCMA_DECODE_MIN, ADPCMA_DECODE_MAX);
		delta = CLAMP(delta + decode_tableA1[data], 0 * 16, 48 * 16);
		if (abs(oldsignal - cursignal) > 2500)
		{
			fprintf(errorlog, "WARNING: Suspicious signal evolution %06x,%06x pos:%06x delta:%06x\n", oldsignal, cursignal, i, delta);
			fprintf(errorlog, "data:%02x dx:%08x\n", data, jedi_table[data + delta]);
		}
		*(out++) = (cursignal & 0xffff) * 32;

		data = (*in++) & 0x0F;
		oldsignal = cursignal;
		cursignal = CLAMP(cursignal + (jedi_table[data + delta]), ADPCMA_DECODE_MIN, ADPCMA_DECODE_MAX);
		delta = CLAMP(delta + decode_tableA1[data], 0 * 16, 48 * 16);
		if (abs(oldsignal - cursignal) > 2500)
		{
			fprintf(errorlog, "WARNING: Suspicious signal evolution %06x,%06x pos:%06x delta:%06x\n", oldsignal, cursignal, i, delta);
			fprintf(errorlog, "data:%02x dx:%08x\n", data, jedi_table[data + delta]);
		}
		*(out++) = (cursignal & 0xffff) * 32;
	}
}
