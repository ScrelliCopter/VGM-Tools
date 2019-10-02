/* neoadpcmextract.cpp
  Copyright (C) 2017, 2019 Nicholas Curtis

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


typedef struct { uint8_t* data; size_t size; } Buffer;

int DecodeSample(FILE* fin, const char* name, Buffer* buf)
{
	// Get sample data length.
	uint32_t sampLen = 0;
	fread(&sampLen, sizeof(uint32_t), 1, fin);
	if (sampLen < sizeof(uint64_t))
		return 1;
	sampLen -= sizeof(uint64_t);

	// Resize buffer if needed.
	if (!buf->data || buf->size < sampLen)
	{
		free(buf->data);
		buf->size = sampLen;
		buf->data = malloc(sampLen);
		if (!buf->data)
			return 1;
	}

	// Ignore 8 bytes.
	uint64_t dummy;
	fread(&dummy, sizeof(uint64_t), 1, fin);

	// Read adpcm data.
	fread(buf->data, sizeof(uint8_t), sampLen, fin);

	// Write adpcm sample.
	FILE* fout = fopen(name, "wb");
	if (!fout)
		return 1;
	fwrite(buf->data, sizeof(uint8_t), sampLen, fout);
	fclose(fout);

	return 0;
}

int vgmExtractSamples(FILE* file)
{
	Buffer smpBytes = {NULL, 0};
	char namebuf[32];
	int smpaCount = 0, smpbCount = 0;

	// Scan for pcm headers.
	while (!feof(file) && !ferror(file))
	{
		// Patterns to match (in hex):
		// 67 66 82 - ADPCM-A
		// 67 66 83 - ADPCM-B
		if (fgetc(file) != 0x67 || fgetc(file) != 0x66)
			continue;

		uint8_t byte = fgetc(file);
		if (byte == 0x82)
		{
			printf("ADPCM-A data found at 0x%08lX\n", ftell(file));
			snprintf(namebuf, sizeof(namebuf), "smpa_%x.pcm", smpaCount++);
			if (DecodeSample(file, namebuf, &smpBytes))
				break;
		}
		else if (byte == 0x83)
		{
			printf("ADPCM-B data found at 0x%08lX\n", ftell(file));
			snprintf(namebuf, sizeof(namebuf), "smpb_%x.pcm", smpbCount++);
			if (DecodeSample(file, namebuf, &smpBytes))
				break;
		}
	}

	free(smpBytes.data);
	return 0;
}
