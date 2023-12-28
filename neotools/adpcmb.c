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

#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "util.h"

// -- from mmsystem.h --
//FIXME: DEELT THIS
#define MAKEFOURCC(ch0, ch1, ch2, ch3)                                    \
	((uint32_t)(uint8_t)(ch0) | ((uint32_t)(uint8_t)(ch1) << 8) |         \
	((uint32_t)(uint8_t)(ch2) << 16) | ((uint32_t)(uint8_t)(ch3) << 24 ))

// -- from mmreg.h, slightly modified --

/* general waveform format structure (information common to all formats) */
typedef struct waveformat_tag {
    uint16_t wFormatTag;        /* format type */
    uint16_t nChannels;         /* number of channels (i.e. mono, stereo...) */
    uint32_t nSamplesPerSec;    /* sample rate */
    uint32_t nAvgBytesPerSec;   /* for buffer estimation */
    uint16_t nBlockAlign;       /* block size of data */
    uint16_t wBitsPerSample;
} WAVEFORMAT;

/* flags for wFormatTag field of WAVEFORMAT */
#define WAVE_FORMAT_PCM     1

// -- from mm*.h end --

#define FOURCC_RIFF MAKEFOURCC('R', 'I', 'F', 'F')
#define FOURCC_WAVE MAKEFOURCC('W', 'A', 'V', 'E')
#define FOURCC_fmt_ MAKEFOURCC('f', 'm', 't', ' ')
#define FOURCC_data MAKEFOURCC('d', 'a', 't', 'a')

typedef struct
{
	uint32_t RIFFfcc;  // 'RIFF'
	uint32_t RIFFLen;
	uint32_t WAVEfcc;  // 'WAVE'
	uint32_t fmt_fcc;  // 'fmt '
	uint32_t fmt_Len;
	WAVEFORMAT fmt_Data;
	uint32_t datafcc;  // 'data'
	uint32_t dataLen;
} WAVE_FILE;


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

static int decode(const char* inPath, const char* outPath, uint32_t OutSmplRate)
{
	FILE* hFile = fopen(inPath, "rb");
	if (hFile == NULL)
	{
		printf("Error opening input file!\n");
		return 2;
	}

	fseek(hFile, 0x00, SEEK_END);
	unsigned int AdpcmSize = ftell(hFile);

	fseek(hFile, 0x00, SEEK_SET);
	uint8_t* AdpcmData = malloc(AdpcmSize);
	fread(AdpcmData, 0x01, AdpcmSize, hFile);
	fclose(hFile);

	unsigned int WaveSize = AdpcmSize * 2; // 4-bit ADPCM -> 2 values per byte
	int16_t* WaveData = malloc(WaveSize * 2);
	printf("Decoding ...");
	YM2610_ADPCM_Decode(AdpcmData, WaveData, WaveSize);
	printf("  OK\n");

	WAVE_FILE WaveFile;
	WaveFile.RIFFfcc = FOURCC_RIFF;
	WaveFile.WAVEfcc = FOURCC_WAVE;
	WaveFile.fmt_fcc = FOURCC_fmt_;
	WaveFile.fmt_Len = sizeof(WAVEFORMAT);

	WAVEFORMAT* TempFmt = &WaveFile.fmt_Data;
	TempFmt->wFormatTag = WAVE_FORMAT_PCM;
	TempFmt->nChannels = 1;
	TempFmt->wBitsPerSample = 16;
	TempFmt->nSamplesPerSec = OutSmplRate ? OutSmplRate : 22050;
	TempFmt->nBlockAlign = TempFmt->nChannels * TempFmt->wBitsPerSample / 8;
	TempFmt->nAvgBytesPerSec = TempFmt->nBlockAlign * TempFmt->nSamplesPerSec;

	WaveFile.datafcc = FOURCC_data;
	WaveFile.dataLen = WaveSize * 2;
	WaveFile.RIFFLen = 0x04 + 0x08 + WaveFile.fmt_Len + 0x08 + WaveFile.dataLen;

	hFile = fopen(outPath, "wb");
	if (hFile == NULL)
	{
		printf("Error opening output file!\n");
		free(AdpcmData);
		free(WaveData);
		return 3;
	}

	fwrite(&WaveFile, sizeof(WAVE_FILE), 0x01, hFile);
	fwrite(WaveData, 0x02, WaveSize, hFile);
	fclose(hFile);
	printf("File written.\n");

	free(AdpcmData);
	free(WaveData);
	return 0;
}

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
	if (WaveFile.RIFFfcc != FOURCC_RIFF || WaveFile.WAVEfcc != FOURCC_WAVE)
	{
		fclose(hFile);
		printf("This is no wave file!\n");
		return 4;
	}

	unsigned int TempLng = fread(&WaveFile.fmt_fcc, 0x04, 0x01, hFile);
	fread(&WaveFile.fmt_Len, 0x04, 0x01, hFile);
	while(WaveFile.fmt_fcc != FOURCC_fmt_)
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
	if (TempFmt->wFormatTag != WAVE_FORMAT_PCM)
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
	while(WaveFile.datafcc != FOURCC_data)
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
