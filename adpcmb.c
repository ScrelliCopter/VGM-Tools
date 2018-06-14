/*;							YM2610 ADPCM-B Codec
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
#include <malloc.h>
#include <math.h>
#include <string.h>

#if defined(_MSC_VER)
#define INLINE	static __inline
#elif defined(__GNUC__)
#define INLINE	static __inline__
#else
#define INLINE	static inline
#endif

#ifdef USE_STDINT

#include <stdint.h>
typedef uint8_t			UINT8;
typedef int8_t			INT8;
typedef uint16_t		UINT16;
typedef int16_t			INT16;
typedef uint32_t		UINT32;
typedef int32_t			INT32;

#else

typedef unsigned char	UINT8;
typedef signed char		INT8;
typedef unsigned short	UINT16;
typedef signed short	INT16;
typedef unsigned int	UINT32;
typedef signed int		INT32;

#endif


typedef UINT8	BYTE;
typedef UINT16	WORD;
typedef UINT32	DWORD;

// -- from mmsystem.h --
#define MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
		((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) |   \
		((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 ))

// -- from mmreg.h, slightly modified --

/* general waveform format structure (information common to all formats) */
typedef struct waveformat_tag {
    WORD    wFormatTag;        /* format type */
    WORD    nChannels;         /* number of channels (i.e. mono, stereo...) */
    DWORD   nSamplesPerSec;    /* sample rate */
    DWORD   nAvgBytesPerSec;   /* for buffer estimation */
    WORD    nBlockAlign;       /* block size of data */
    WORD    wBitsPerSample;
} WAVEFORMAT;

/* flags for wFormatTag field of WAVEFORMAT */
#define WAVE_FORMAT_PCM     1

// -- from mm*.h end --

#define FOURCC_RIFF     MAKEFOURCC('R', 'I', 'F', 'F')
#define FOURCC_WAVE     MAKEFOURCC('W', 'A', 'V', 'E')
#define FOURCC_fmt_     MAKEFOURCC('f', 'm', 't', ' ')
#define FOURCC_data     MAKEFOURCC('d', 'a', 't', 'a')

typedef struct
{
	UINT32 RIFFfcc;	// 'RIFF'
	UINT32 RIFFLen;
	UINT32 WAVEfcc;	// 'WAVE'
	UINT32 fmt_fcc;	// 'fmt '
	UINT32 fmt_Len;
	WAVEFORMAT fmt_Data;
	UINT32 datafcc;	// 'data'
	UINT32 dataLen;
} WAVE_FILE;


static const long stepsizeTable[16] =
{
	 57,  57,  57,  57,  77, 102, 128, 153,
	 57,  57,  57,  57,  77, 102, 128, 153
};

static int YM2610_ADPCM_Encode(INT16 *src, UINT8 *dest, int len)
{
	int lpc, flag;
	long i, dn, xn, stepSize;
	UINT8 adpcm;
	UINT8 adpcmPack;
	
	xn = 0;
	stepSize = 127;
	flag = 0;
	
	for (lpc = 0; lpc < len; lpc ++)
	{
		dn = *src - xn;
		src ++;
		
		i = (abs(dn) << 16) / (stepSize << 14);
		if (i > 7)
			i = 7;
		adpcm = (UINT8)i;
		
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

static int YM2610_ADPCM_Decode(UINT8 *src, INT16 *dest, int len)
{
	int lpc, flag, shift, step;
	long i, xn, stepSize;
	UINT8 adpcm;
	
	xn = 0;
	stepSize = 127;
	flag = 0;
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
		
		if (xn > 32767)
			xn = 32767;
		else if (xn < -32768)
			xn = -32768;
		
		stepSize = stepSize * stepsizeTable[adpcm] / 64;
		
		if (stepSize < 127)
			stepSize = 127;
		else if (stepSize > 24576)
			stepSize = 24576;
		
		*dest = (INT16)xn;
		dest ++;
		
		src += step;
		step = step ^ 1;
		shift = shift ^ 4;
	}
	
	return 0;
}

INLINE UINT32 DeltaTReg2SampleRate(UINT16 DeltaN, UINT32 Clock)
{
	return (UINT32)(DeltaN * (Clock / 72.0) / 65536.0 + 0.5);
}

int main(int argc, char* argv[])
{
	int ErrVal;
	int ArgBase;
	DWORD OutSmplRate;
	FILE* hFile;
	unsigned int AdpcmSize;
	UINT8* AdpcmData;
	unsigned int WaveSize;
	UINT16* WaveData;
	WAVE_FILE WaveFile;
	WAVEFORMAT* TempFmt;
	unsigned int TempLng;
	UINT16 DTRegs;
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
	
	ErrVal = 0;
	AdpcmData = NULL;
	WaveData = NULL;
	OutSmplRate = 0;
	
	ArgBase = 2;
	if (argv[2][0] == '-' && argv[2][2] == ':')
	{
		switch(argv[2][1])
		{
		case 's':
			OutSmplRate = strtol(argv[2] + 3, NULL, 0);
			break;
		case 'r':
			DTRegs = (UINT16)strtoul(argv[2] + 3, &TempPnt, 0);
			TempLng = 0;
			if (*TempPnt == ',')
			{
				TempLng = strtoul(TempPnt + 1, NULL, 0);
			}
			if (! TempLng)
				TempLng = 4000000;
			OutSmplRate = DeltaTReg2SampleRate(DTRegs, TempLng);
			break;
		}
		ArgBase ++;
		if (argc < ArgBase + 2)
		{
			printf("Not enought arguments!\n");
			return 1;
		}
	}
	
	switch(argv[1][1])
	{
	case 'd':
		hFile = fopen(argv[ArgBase + 0], "rb");
		if (hFile == NULL)
		{
			printf("Error opening input file!\n");
			ErrVal = 2;
			goto Finish;
		}
		
		fseek(hFile, 0x00, SEEK_END);
		AdpcmSize = ftell(hFile);
		
		fseek(hFile, 0x00, SEEK_SET);
		AdpcmData = (UINT8*)malloc(AdpcmSize);
		fread(AdpcmData, 0x01, AdpcmSize, hFile);
		fclose(hFile);
		
		WaveSize = AdpcmSize * 2;	// 4-bit ADPCM -> 2 values per byte
		WaveData = (UINT16*)malloc(WaveSize * 2);
		printf("Decoding ...");
		YM2610_ADPCM_Decode(AdpcmData, WaveData, WaveSize);
		printf("  OK\n");
		
		WaveFile.RIFFfcc = FOURCC_RIFF;
		WaveFile.WAVEfcc = FOURCC_WAVE;
		WaveFile.fmt_fcc = FOURCC_fmt_;
		WaveFile.fmt_Len = sizeof(WAVEFORMAT);
		
		TempFmt = &WaveFile.fmt_Data;
		TempFmt->wFormatTag = WAVE_FORMAT_PCM;
		TempFmt->nChannels = 1;
		TempFmt->wBitsPerSample = 16;
		TempFmt->nSamplesPerSec = OutSmplRate ? OutSmplRate : 22050;
		TempFmt->nBlockAlign = TempFmt->nChannels * TempFmt->wBitsPerSample / 8;
		TempFmt->nAvgBytesPerSec = TempFmt->nBlockAlign * TempFmt->nSamplesPerSec;
		
		WaveFile.datafcc = FOURCC_data;
		WaveFile.dataLen = WaveSize * 2;
		WaveFile.RIFFLen = 0x04 + 0x08 + WaveFile.fmt_Len + 0x08 + WaveFile.dataLen;
		
		hFile = fopen(argv[ArgBase + 1], "wb");
		if (hFile == NULL)
		{
			printf("Error opening output file!\n");
			ErrVal = 3;
			goto Finish;
		}
		
		fwrite(&WaveFile, sizeof(WAVE_FILE), 0x01, hFile);
		fwrite(WaveData, 0x02, WaveSize, hFile);
		fclose(hFile);
		printf("File written.\n");
		
		break;
	case 'e':
		hFile = fopen(argv[ArgBase + 0], "rb");
		if (hFile == NULL)
		{
			printf("Error opening input file!\n");
			ErrVal = 2;
			goto Finish;
		}
		
		fread(&WaveFile.RIFFfcc, 0x0C, 0x01, hFile);
		if (WaveFile.RIFFfcc != FOURCC_RIFF || WaveFile.WAVEfcc != FOURCC_WAVE)
		{
			fclose(hFile);
			printf("This is no wave file!\n");
			ErrVal = 4;
			goto Finish;
		}
		
		TempLng = fread(&WaveFile.fmt_fcc, 0x04, 0x01, hFile);
		fread(&WaveFile.fmt_Len, 0x04, 0x01, hFile);
		while(WaveFile.fmt_fcc != FOURCC_fmt_)
		{
			if (! TempLng)	// TempLng == 0 -> EOF reached
			{
				fclose(hFile);
				printf("Error in wave file: Can't find format-tag!\n");
				ErrVal = 4;
				goto Finish;
			}
			fseek(hFile, WaveFile.fmt_Len, SEEK_CUR);
			
			TempLng = fread(&WaveFile.fmt_fcc, 0x04, 0x01, hFile);
			fread(&WaveFile.fmt_Len, 0x04, 0x01, hFile);
		};
		TempLng = ftell(hFile) + WaveFile.fmt_Len;
		fread(&WaveFile.fmt_Data, sizeof(WAVEFORMAT), 0x01, hFile);
		fseek(hFile, TempLng, SEEK_SET);
		
		TempFmt = &WaveFile.fmt_Data;
		if (TempFmt->wFormatTag != WAVE_FORMAT_PCM)
		{
			fclose(hFile);
			printf("Error in wave file: Compressed wave file are not supported!\n");
			ErrVal = 4;
			goto Finish;
		}
		if (TempFmt->nChannels != 1)
		{
			fclose(hFile);
			printf("Error in wave file: Unsupported number of channels (%hu)!\n", TempFmt->nChannels);
			ErrVal = 4;
			goto Finish;
		}
		if (TempFmt->wBitsPerSample != 16)
		{
			fclose(hFile);
			printf("Error in wave file: Only 16-bit waves are supported! (File uses %hu bit)\n", TempFmt->wBitsPerSample);
			ErrVal = 4;
			goto Finish;
		}
		
		TempLng = fread(&WaveFile.datafcc, 0x04, 0x01, hFile);
		fread(&WaveFile.dataLen, 0x04, 0x01, hFile);
		while(WaveFile.datafcc != FOURCC_data)
		{
			if (! TempLng)	// TempLng == 0 -> EOF reached
			{
				fclose(hFile);
				printf("Error in wave file: Can't find data-tag!\n");
				ErrVal = 4;
				goto Finish;
			}
			fseek(hFile, WaveFile.dataLen, SEEK_CUR);
			
			TempLng = fread(&WaveFile.datafcc, 0x04, 0x01, hFile);
			fread(&WaveFile.dataLen, 0x04, 0x01, hFile);
		};
		WaveSize = WaveFile.dataLen / 2;
		WaveData = (UINT16*)malloc(WaveSize * 2);
		fread(WaveData, 0x02, WaveSize, hFile);
		
		fclose(hFile);
		
		AdpcmSize = WaveSize / 2;
		AdpcmData = (UINT8*)malloc(AdpcmSize);
		printf("Encoding ...");
		YM2610_ADPCM_Encode(WaveData, AdpcmData, WaveSize);
		printf("  OK\n");
		
		hFile = fopen(argv[ArgBase + 1], "wb");
		if (hFile == NULL)
		{
			printf("Error opening output file!\n");
			ErrVal = 3;
			goto Finish;
		}
		
		fwrite(AdpcmData, 0x01, AdpcmSize, hFile);
		fclose(hFile);
		printf("File written.\n");
		
		break;
	}
	
Finish:
	free(AdpcmData);
	free(WaveData);
	
	return ErrVal;
}
