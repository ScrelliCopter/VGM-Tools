/* neoadpcmextract.c
  Copyright (C) 2017, 2019, 2020 Nicholas Curtis

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

#include "neoadpcmextract.h"
#include <stdlib.h>


int vgmReadSample(FILE* fin, Buffer* buf)
{
	// Get sample data length.
	uint32_t sampLen = 0;
	fread(&sampLen, sizeof(uint32_t), 1, fin);
	if (sampLen < sizeof(uint64_t))
		return 1;
	sampLen -= sizeof(uint64_t);

	// Resize buffer if needed.
	buf->size = sampLen;
	if (!buf->data || buf->reserved < sampLen)
	{
		free(buf->data);
		buf->reserved = sampLen;
		buf->data = malloc(sampLen);
		if (!buf->data)
			return 1;
	}

	// Ignore 8 bytes.
	uint64_t dummy;
	fread(&dummy, sizeof(uint64_t), 1, fin);

	// Read adpcm data.
	fread(buf->data, sizeof(uint8_t), sampLen, fin);

	return 0;
}

int vgmScanSample(FILE* file)
{
	// Scan for pcm headers.
	while (1)
	{
		if (feof(file) || ferror(file))
			return 0;

		// Patterns to match (in hex):
		// 67 66 82 - ADPCM-A
		// 67 66 83 - ADPCM-B
		if (fgetc(file) != 0x67 || fgetc(file) != 0x66)
			continue;

		uint8_t byte = fgetc(file);
		if (byte == 0x82)
			return 'A';
		else if (byte == 0x83)
			return 'B';
	}
}
