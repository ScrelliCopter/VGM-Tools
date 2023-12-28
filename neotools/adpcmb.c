/*;                         YM2610 ADPCM-B Codec
;
;**** PCM to ADPCM-B & ADPCM-B to PCM converters for NEO-GEO System ****
;ADPCM-B - 1 channel 1.8-55.5 KHz, 16 MB Sample ROM size, 256 B min size of sample, 16 MB max, compatable with YM2608 
;
;http://www.raregame.ru/file/15/YM2610.pdf     YM2610 DATASHEET
;
; Fred/FRONT
;
;Usage 1: ADPCM_Encode.exe -d [-r:reg,clock] Input.bin Output.wav
;Usage 2: ADPCM_Encode.exe -e Input.wav Output.bin
;
; Valley Bell
;----------------------------------------------------------------------------------------------------------------------------*/

#include "wave.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static const long stepsizeTable[16] =
{
	 57,  57,  57,  57,  77, 102, 128, 153,
	 57,  57,  57,  57,  77, 102, 128, 153
};

static int YM2610_ADPCM_Encode(int16_t *src, uint8_t *dest, int len)
{
	int lpc, flag;
	long i, dn, xn, stepSize;
	uint8_t adpcm;
	uint8_t adpcmPack;

	xn = 0;
	stepSize = 127;
	flag = 0;

	for (lpc = 0; lpc < len; lpc ++)
	{
		dn = *src - xn;
		src ++;

		i = (labs(dn) << 16) / (stepSize << 14);
		if (i > 7)
			i = 7;
		adpcm = (uint8_t)i;

		i = (adpcm * 2 + 1) * stepSize / 8;

		if (dn < 0)
		{
			adpcm |= 0x8;
			xn -= i;
		}
		else
		{
			xn += i;
		}

		stepSize = (stepsizeTable[adpcm] * stepSize) / 64;

		if (stepSize < 127)
			stepSize = 127;
		else if (stepSize > 24576)
			stepSize = 24576;

		if (flag == 0)
		{
			adpcmPack = (adpcm << 4);
			flag = 1;
		}
		else
		{
			adpcmPack |= adpcm;
			*dest = adpcmPack;
			dest ++;
			flag = 0;
		}
	}

	return 0;
}

static int YM2610_ADPCM_Decode(uint8_t *src, int16_t *dest, int len)
{
	int lpc, shift, step;
	long i, xn, stepSize;
	uint8_t adpcm;

	xn = 0;
	stepSize = 127;
	shift = 4;
	step = 0;

	for (lpc = 0; lpc < len; lpc ++)
	{
		adpcm = (*src >> shift) & 0xf;

		i = ((adpcm & 7) * 2 + 1) * stepSize / 8;
		if (adpcm & 8)
			xn -= i;
		else
			xn += i;

		xn = CLAMP(xn, -32768, 32767);

		stepSize = stepSize * stepsizeTable[adpcm] / 64;

		if (stepSize < 127)
			stepSize = 127;
		else if (stepSize > 24576)
			stepSize = 24576;

		*dest = (int16_t)xn;
		dest ++;

		src += step;
		step = step ^ 1;
		shift = shift ^ 4;
	}

	return 0;
}

static FORCE_INLINE uint32_t DeltaTReg2SampleRate(uint16_t DeltaN, uint32_t Clock)
{
	return (uint32_t)(DeltaN * (Clock / 72.0) / 65536.0 + 0.5);
}

static int decode(const char* inPath, const char* outPath, uint32_t sampleRate)
{
	FILE* inFile = fopen(inPath, "rb");
	if (inFile == NULL)
	{
		printf("Error opening input file!\n");
		return 2;
	}

	fseek(inFile, 0, SEEK_END);
	long adpcmSize = ftell(inFile);
	fseek(inFile, 0, SEEK_SET);

	uint8_t* adpcmData = malloc((size_t)adpcmSize);
	fread(adpcmData, 0x01, adpcmSize, inFile);
	fclose(inFile);

	long wavSize = adpcmSize * 2;  // 4-bit ADPCM -> 2 values per byte
	int16_t* wavData = malloc((size_t)wavSize * sizeof(int16_t));
	printf("Decoding ...");
	YM2610_ADPCM_Decode(adpcmData, wavData, wavSize);
	printf("  OK\n");

	// Write decoded sample as WAVE
	FILE* outFile = fopen(outPath, "wb");
	if (outFile == NULL)
	{
		printf("Error opening output file!\n");
		free(adpcmData);
		free(wavData);
		return 3;
	}
	waveWrite(&(const WaveSpec)
	{
		.format    = WAVE_FMT_PCM,
		.channels  = 1,
		.rate      = sampleRate ? sampleRate : 22050,
		.bytedepth = 2
	},
	wavData, (size_t)wavSize * sizeof(int16_t), &waveStreamDefaultCb, outFile);
	fclose(outFile);

	printf("File written.\n");

	free(adpcmData);
	free(wavData);
	return 0;
}

typedef struct waveformat_tag {
    uint16_t wFormatTag;        /* format type */
    uint16_t nChannels;         /* number of channels (i.e. mono, stereo...) */
    uint32_t nSamplesPerSec;    /* sample rate */
    uint32_t nAvgBytesPerSec;   /* for buffer estimation */
    uint16_t nBlockAlign;       /* block size of data */
    uint16_t wBitsPerSample;
} WAVEFORMAT;

typedef struct
{
	char RIFFfcc[4];
	uint32_t RIFFLen;
	char WAVEfcc[4];
	char fmt_fcc[4];
	uint32_t fmt_Len;
	WAVEFORMAT fmt_Data;
	char datafcc[4];
	uint32_t dataLen;
} WAVE_FILE;

static int encode(const char* inPath, const char* outPath)
{
	FILE* hFile = fopen(inPath, "rb");
	if (hFile == NULL)
	{
		printf("Error opening input file!\n");
		return 2;
	}

	WAVE_FILE WaveFile;
	fread(&WaveFile.RIFFfcc, 0x0C, 0x01, hFile);
	if (memcmp(WaveFile.RIFFfcc, "RIFF", 4) != 0 || memcmp(WaveFile.WAVEfcc, "WAVE", 4) != 0)
	{
		fclose(hFile);
		printf("This is no wave file!\n");
		return 4;
	}

	unsigned int TempLng = fread(&WaveFile.fmt_fcc, 0x04, 0x01, hFile);
	fread(&WaveFile.fmt_Len, 0x04, 0x01, hFile);
	while (memcmp(WaveFile.fmt_fcc, "fmt ", 4) != 0)
	{
		if (!TempLng) // TempLng == 0 -> EOF reached
		{
			fclose(hFile);
			printf("Error in wave file: Can't find format-tag!\n");
			return 4;
		}
		fseek(hFile, WaveFile.fmt_Len, SEEK_CUR);

		TempLng = fread(&WaveFile.fmt_fcc, 0x04, 0x01, hFile);
		fread(&WaveFile.fmt_Len, 0x04, 0x01, hFile);
	};
	TempLng = ftell(hFile) + WaveFile.fmt_Len;
	fread(&WaveFile.fmt_Data, sizeof(WAVEFORMAT), 0x01, hFile);
	fseek(hFile, TempLng, SEEK_SET);

	WAVEFORMAT* TempFmt = &WaveFile.fmt_Data;
	if (TempFmt->wFormatTag != WAVE_FMT_PCM)
	{
		fclose(hFile);
		printf("Error in wave file: Compressed wave file are not supported!\n");
		return 4;
	}
	if (TempFmt->nChannels != 1)
	{
		fclose(hFile);
		printf("Error in wave file: Unsupported number of channels (%hu)!\n", TempFmt->nChannels);
		return 4;
	}
	if (TempFmt->wBitsPerSample != 16)
	{
		fclose(hFile);
		printf("Error in wave file: Only 16-bit waves are supported! (File uses %hu bit)\n", TempFmt->wBitsPerSample);
		return 4;
	}

	TempLng = fread(&WaveFile.datafcc, 0x04, 0x01, hFile);
	fread(&WaveFile.dataLen, 0x04, 0x01, hFile);
	while (memcmp(WaveFile.datafcc, "data", 4) != 0)
	{
		if (!TempLng) // TempLng == 0 -> EOF reached
		{
			fclose(hFile);
			printf("Error in wave file: Can't find data-tag!\n");
			return 4;
		}
		fseek(hFile, WaveFile.dataLen, SEEK_CUR);

		TempLng = fread(&WaveFile.datafcc, 0x04, 0x01, hFile);
		fread(&WaveFile.dataLen, 0x04, 0x01, hFile);
	};
	unsigned int WaveSize = WaveFile.dataLen / 2;
	int16_t* WaveData = malloc(WaveSize * 2);
	fread(WaveData, 0x02, WaveSize, hFile);

	fclose(hFile);

	unsigned int AdpcmSize = WaveSize / 2;
	uint8_t* AdpcmData = malloc(AdpcmSize);
	printf("Encoding ...");
	YM2610_ADPCM_Encode(WaveData, AdpcmData, WaveSize);
	printf("  OK\n");

	hFile = fopen(outPath, "wb");
	if (hFile == NULL)
	{
		printf("Error opening output file!\n");
		free(AdpcmData);
		free(WaveData);
		return 3;
	}

	fwrite(AdpcmData, 0x01, AdpcmSize, hFile);
	fclose(hFile);
	printf("File written.\n");

	free(AdpcmData);
	free(WaveData);
	return 0;
}


int main(int argc, char* argv[])
{
	unsigned int TempLng;
	uint16_t DTRegs;
	char* TempPnt;

	printf("NeoGeo ADPCM-B En-/Decoder\n--------------------------\n");
	if (argc < 4)
	{
		printf("Usage: ADPCM_Encode.exe -method [-option] InputFile OutputFile\n");
		printf("-method - En-/Decoding Method:\n");
		printf("    -d for decode (bin -> wav)\n");
		printf("    -e for encode (wav -> bin)\n");
		printf("-option - Options for Sample Rate of decoded Wave:\n");
		printf("    -s:rate - Sample Rate in Hz (default: 22050)\n");
		printf("    -r:regs[,clock] - DeltaT Register value (use 0x for hex) and chip clock\n");
		printf("                      (default chip clock: 4 MHz)\n");
		printf("\n");
		printf("Wave In-/Output is 16-bit, Mono\n");
		return 1;
	}

	if (strcmp(argv[1], "-d") && strcmp(argv[1], "-e"))
	{
		printf("Wrong option! Use -d or -e!\n");
		return 1;
	}

	int ErrVal = 0;
	uint32_t OutSmplRate = 0;

	int ArgBase = 2;
	if (argv[2][0] == '-' && argv[2][2] == ':')
	{
		switch(argv[2][1])
		{
		case 's':
			OutSmplRate = strtol(argv[2] + 3, NULL, 0);
			break;
		case 'r':
			DTRegs = (uint16_t)strtoul(argv[2] + 3, &TempPnt, 0);
			TempLng = 0;
			if (*TempPnt == ',')
			{
				TempLng = strtoul(TempPnt + 1, NULL, 0);
			}
			if (!TempLng)
				TempLng = 4000000;
			OutSmplRate = DeltaTReg2SampleRate(DTRegs, TempLng);
			break;
		}
		ArgBase++;
		if (argc < ArgBase + 2)
		{
			printf("Not enought arguments!\n");
			return 1;
		}
	}

	switch(argv[1][1])
	{
	case 'd':
		ErrVal = decode(argv[ArgBase + 0], argv[ArgBase + 1], OutSmplRate);
		break;
	case 'e':
		ErrVal = encode(argv[ArgBase + 0], argv[ArgBase + 1]);
		break;
	}

	return ErrVal;
}
