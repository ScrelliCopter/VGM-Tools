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
#include <vector>
#include <cstdint>

void DecodeSample(FILE* fin, const char* name, std::vector <uint8_t>& buf)
{
	// Set up output vector.
	uint32_t sampLen = 0;
	fread(&sampLen, sizeof(uint32_t), 1, fin);
	if (sampLen < sizeof(uint64_t))
		return;

	sampLen -= sizeof(uint64_t);
	buf.clear();
	buf.resize(sampLen);

	// Ignore 8 bytes.
	uint64_t dummy;
	fread(&dummy, sizeof(uint64_t), 1, fin);

	// Read adpcm data.
	fread(buf.data(), sizeof(uint8_t), sampLen, fin);

	FILE* fout = fopen(name, "wb");
	if (!fout)
		return;

	fwrite(buf.data(), sizeof(uint8_t), buf.size(), fout);
	fclose(fout);
}

int main(int argc, char** argv)
{
	if (argc != 2)
		return 1;

	// Open file.
	FILE* file = fopen(argv[1], "rb");
	if (!file)
		return 1;

	// Search for pcm headers.
	std::vector<uint8_t> smpBytes;
	char namebuf[32];
	int smpaCount = 0, smpbCount = 0;
	while (!feof(file) && !ferror(file))
	{
		if (fgetc(file) != 0x67 || fgetc(file) != 0x66)
			continue;

		uint8_t byte = fgetc(file);
		if (byte == 0x82)
		{
			printf("ADPCM-A data found at 0x%08lX\n", ftell(file));
			snprintf(namebuf, sizeof(namebuf), "smpa_%x.pcm", smpaCount++);
			DecodeSample(file, namebuf, smpBytes);
		}
		else if (byte == 0x83)
		{
			printf("ADPCM-B data found at 0x%08lX\n", ftell(file));
			snprintf(namebuf, sizeof(namebuf), "smpb_%x.pcm", smpbCount++);
			DecodeSample(file, namebuf, smpBytes);
		}
	}

	fclose(file);
	return 0;
}
