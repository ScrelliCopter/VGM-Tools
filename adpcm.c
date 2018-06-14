#include <process.h>
#include <stdio.h>
#include <mem.h>
#include <math.h>
#include <io.h>

#define BUFFER_SIZE 1024*256
#define ADPCMA_VOLUME_RATE 1
#define ADPCMA_DECODE_RANGE 1024
#define ADPCMA_DECODE_MIN (-(ADPCMA_DECODE_RANGE*ADPCMA_VOLUME_RATE))
#define ADPCMA_DECODE_MAX ((ADPCMA_DECODE_RANGE*ADPCMA_VOLUME_RATE)-1)
#define ADPCMA_VOLUME_DIV 1

unsigned char RiffWave[44] = {
	0x52, 0x49, 0x46, 0x46, 0x24, 0x00, 0x00, 0x00, 0x57, 0x41, 0x56, 0x45, 0x66, 0x6D, 0x74,
	0x20, 0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x44, 0x48, 0x00, 0x00, 0x88, 0x90,
	0x00, 0x00, 0x02, 0x00, 0x10, 0x00, 0x64, 0x61, 0x74, 0x61, 0x00, 0x00, 0x00, 0x00 
	} ;

static int	decode_tableA1[16] = {
  -1*16, -1*16, -1*16, -1*16, 2*16, 5*16, 7*16, 9*16,
  -1*16, -1*16, -1*16, -1*16, 2*16, 5*16, 7*16, 9*16
};
static int	jedi_table[49*16];
static int	signal;
static int	delta;

void		adpcm_init(void);
void		adpcm_decode(void *, void *, int);
int		Limit(int, int, int);
FILE*		errorlog;

int	main(int argc, char *argv[])
{
	FILE		*Fp1, *Fp2;
	void		*InputBuffer, *OutputBuffer;
	int		Readed;
	unsigned int	Filelen;
	
	puts("**** ADPCM to PCM converter v 1.01\n");
	if	(argc!=3) {
		puts("USAGE: adpcm <InputFile.pcm> <OutputFile.wav>");
		exit(-1);
	}
	
	Fp1 = fopen(argv[1], "rb");
	if (Fp1==NULL) {
		printf("Could not open inputfile %s\n", argv[1]);
		exit(-2);
	}

	Fp2 = fopen(argv[2], "wb");
	if (Fp2==NULL) {
		printf("Could not open outputfile %s\n", argv[2]);
		exit(-3);
	}

	errorlog = fopen("error.log", "wb");
	
	InputBuffer = malloc(BUFFER_SIZE);
	if (InputBuffer == NULL) {
		printf("Could not allocate input buffer. (%d bytes)\n", BUFFER_SIZE);
		exit(-4);
	}
	
	OutputBuffer = malloc(BUFFER_SIZE*10);
	if (OutputBuffer == NULL) {
		printf("Could not allocate output buffer. (%d bytes)\n", BUFFER_SIZE*4);
		exit(-5);
	}

	adpcm_init();
	
	Filelen = filelength(fileno(Fp1));
	*((unsigned int*)(&RiffWave[4])) = Filelen*4 + 0x2C;
	*((unsigned int*)(&RiffWave[0x28])) = Filelen*4;
	
	fwrite(RiffWave, 0x2c, 1, Fp2);
	
	do {
		Readed = fread(InputBuffer, 1, BUFFER_SIZE, Fp1);
		if (Readed>0) {
			adpcm_decode(InputBuffer, OutputBuffer, Readed);
			fwrite(OutputBuffer, Readed*4, 1, Fp2);
		}
	} while (Readed==BUFFER_SIZE);
	
	free(InputBuffer);
	free(OutputBuffer);
	fclose(Fp1);
	fclose(Fp2);
	
	puts("Done...");
	
	return 0;
}

void	adpcm_init(void)
{
	int step, nib;

	for (step = 0; step <= 48; step++)
	{
		int stepval = floor (16.0 * pow (11.0 / 10.0, (double)step) * ADPCMA_VOLUME_RATE);
		/* loop over all nibbles and compute the difference */
		for (nib = 0; nib < 16; nib++)
		{
			int value = stepval*((nib&0x07)*2+1)/8;
			jedi_table[step*16+nib] = (nib&0x08) ? -value : value;
		}
	}

	delta = 0;
	signal = 0;
}

void	adpcm_decode(void *InputBuffer, void *OutputBuffer, int Length)
{
	char	*in;
	short	*out;
	int	i, data, oldsignal;
	
	in = (char *)InputBuffer;
	out = (short *)OutputBuffer;
	
	for(i=0;i<Length;i++) {
		data = ((*in)>>4)&0x0F;
		oldsignal = signal;
		signal = Limit(signal + (jedi_table[data+delta]), ADPCMA_DECODE_MAX, ADPCMA_DECODE_MIN);
		delta = Limit(delta + decode_tableA1[data], 48*16, 0*16);
		if (abs(oldsignal-signal)>2500) {
			fprintf(errorlog, "WARNING: Suspicious signal evolution %06x,%06x pos:%06x delta:%06x\n", oldsignal, signal, i, delta);
			fprintf(errorlog, "data:%02x dx:%08x\n", data, jedi_table[data+delta]);
		}
		*(out++) = (signal&0xffff)*32;

		data = (*in++)&0x0F;
		oldsignal = signal;
		signal = Limit(signal + (jedi_table[data+delta]), ADPCMA_DECODE_MAX, ADPCMA_DECODE_MIN);
		delta = Limit(delta + decode_tableA1[data], 48*16, 0*16);
		if (abs(oldsignal-signal)>2500) {
			fprintf(errorlog, "WARNING: Suspicious signal evolution %06x,%06x pos:%06x delta:%06x\n", oldsignal, signal, i, delta);
			fprintf(errorlog, "data:%02x dx:%08x\n", data, jedi_table[data+delta]);
		}
		*(out++) = (signal&0xffff)*32;
	}
}

int Limit( int val, int max, int min ) {

	if ( val > max )
		val = max;
	else if ( val < min )
		val = min;

	return val;
}

