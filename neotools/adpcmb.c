/*;                         YM2610 ADPCM-B Codec
;
;
; Fred/FRONT
;
;Usage 1: ADPCM_Encode.exe -d [-r:reg,clock] Input.bin Output.wav
;Usage 2: ADPCM_Encode.exe -e Input.wav Output.bin
;
; Valley Bell
;----------------------------------------------------------------------------------------------------------------------------*/

#include "adpcmb.h"
#include "wave.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* PCM to ADPCM-B & ADPCM-B to PCM converters for NEO-GEO System ****
 ADPCM-B - 1 channel 1.8-55.5 KHz, 16 MB Sample ROM size, 256 B min size of sample, 16 MB max, compatable with YM2608

 http://www.raregame.ru/file/15/YM2610.pdf     YM2610 DATASHEET
*/

static const long stepSizeTable[16] =
{
	 57,  57,  57,  57,  77, 102, 128, 153,
	 57,  57,  57,  57,  77, 102, 128, 153
};

void adpcmBEncoderInit(AdpcmBEncoderState* encoder)
{
	encoder->xn = 0;
	encoder->stepSize = 127;
	encoder->flag = false;
	encoder->adpcmPack = 0;
}

void adpcmBEncode(AdpcmBEncoderState* encoder, const int16_t* restrict in, uint8_t* restrict out, int len)
{
	for (int lpc = 0; lpc < len; ++lpc)
	{
		long dn = (*in++) - encoder->xn;

		long i = (labs(dn) << 16) / (encoder->stepSize << 14);
		i = MIN(i, 7);
		uint8_t adpcm = i;

		i = (adpcm * 2 + 1) * encoder->stepSize / 8;

		if (dn < 0)
		{
			adpcm |= 0x8;
			encoder->xn -= i;
		}
		else
		{
			encoder->xn += i;
		}

		encoder->stepSize = (stepSizeTable[adpcm] * encoder->stepSize) / 64;
		encoder->stepSize = CLAMP(encoder->stepSize, 127, 24576);

		if (!encoder->flag)
		{
			encoder->adpcmPack = adpcm << 4;
			encoder->flag = true;
		}
		else
		{
			(*out++) = encoder->adpcmPack |= adpcm;
			encoder->flag = false;
		}
	}
}

void adpcmBDecoderInit(AdpcmBDecoderState* decoder)
{
	decoder->xn = 0;
	decoder->stepSize = 127;
	decoder->shift = 4;
	decoder->step = 0;
}

void adpcmBDecode(AdpcmBDecoderState* decoder, const uint8_t* restrict in, int16_t* restrict out, int len)
{
	for (int lpc = 0; lpc < len; ++lpc)
	{
		uint8_t adpcm = (*in >> decoder->shift) & 0xF;

		long i = ((adpcm & 7) * 2 + 1) * decoder->stepSize / 8;
		if (adpcm & 8)
			decoder->xn -= i;
		else
			decoder->xn += i;
		decoder->xn = CLAMP(decoder->xn, -32768, 32767);

		decoder->stepSize = decoder->stepSize * stepSizeTable[adpcm] / 64;
		decoder->stepSize = CLAMP(decoder->stepSize, 127, 24576);

		(*out++) = (int16_t)decoder->xn;

		in += decoder->step;
		decoder->step = decoder->step ^ 1;
		decoder->shift = decoder->shift ^ 4;
	}
}

static FORCE_INLINE uint32_t DeltaTReg2SampleRate(uint16_t DeltaN, uint32_t Clock)
{
	return (uint32_t)(DeltaN * (Clock / 72.0) / 65536.0 + 0.5);
}

#define BUFFER_SIZE 2048

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

	uint8_t* adpcmData = malloc(BUFFER_SIZE);
	int16_t* wavData   = malloc(BUFFER_SIZE * 2 * sizeof(int16_t));

	FILE* outFile = fopen(outPath, "wb");
	if (outFile == NULL)
	{
		printf("Error opening output file!\n");
		free(wavData);
		free(adpcmData);
		fclose(inFile);
		return 3;
	}

	// Write wave header
	waveWrite(&(const WaveSpec)
	{
		.format    = WAVE_FMT_PCM,
		.channels  = 1,
		.rate      = sampleRate ? sampleRate : 22050,
		.bytedepth = 2
	},
	NULL, (size_t)adpcmSize * 2 * sizeof(int16_t), &waveStreamDefaultCb, outFile);

	printf("Decoding ...");
	AdpcmBDecoderState decoder;
	adpcmBDecoderInit(&decoder);
	size_t read;
	do
	{
		if ((read = fread(adpcmData, 1, BUFFER_SIZE, inFile)) > 0)
		{
			adpcmBDecode(&decoder, adpcmData, wavData, read * 2);
			fwrite(wavData, sizeof(int16_t), read * 2, outFile);
		}
	}
	while (read == BUFFER_SIZE);
	printf("  OK\n");
	fclose(outFile);

	free(wavData);
	free(adpcmData);
	fclose(inFile);
	printf("File written.\n");
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
	AdpcmBEncoderState encoder;
	adpcmBEncoderInit(&encoder);
	adpcmBEncode(&encoder, WaveData, AdpcmData, WaveSize);
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
		switch (argv[2][1])
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
		if (argc < ++ArgBase + 2)
		{
			printf("Not enought arguments!\n");
			return 1;
		}
	}

	switch (argv[1][1])
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
