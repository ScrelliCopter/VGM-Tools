/* neoadpcmextract.cpp
  Copyright (C) 2017 Nicholas Curtis

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

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <cstdint>

void DecodeSample ( std::ifstream& a_file, std::vector<uint8_t>& a_out )
{
	// Set up output vector.
	uint32_t sampLen = 0;
	a_file.read ( (char*)&sampLen, sizeof(uint32_t) );
	if ( sampLen < sizeof(uint64_t) )
	{
		return;
	}

	sampLen -= sizeof(uint64_t);
	a_out.clear ();
	a_out.resize ( sampLen );

	// Ignore 8 bytes.
	uint64_t dummy;
	a_file.read ( (char*)&dummy, sizeof(uint64_t) );

	// Read adpcm data.
	a_file.read ( (char*)a_out.data (), sampLen );
}

void DumpBytes ( std::string a_path, const std::vector<uint8_t>& a_bytes )
{
	std::ofstream fileOut ( a_path, std::ios::binary );
	fileOut.write ( (const char*)a_bytes.data (), a_bytes.size () );
	fileOut.close ();
}

int main ( int argc, char** argv )
{
	if ( argc != 2 )
	{
		return -1;
	}

	// Open file.
	std::ifstream file ( argv[1], std::ios::binary );
	if ( !file.is_open () )
	{
		return -1;
	}

	// Search for pcm headers.
	std::vector<uint8_t> smpBytes;
	int smpA = 0, smpB = 0;
	while ( !file.eof () && !file.fail () )
	{
		uint8_t byte;

		file >> byte;
		if ( byte == 0x67 )
		{
			file >> byte;
			if ( byte == 0x66 )
			{
				file >> byte;
				if ( byte == 0x82 )
				{
					std::cout << "ADPCM-A data found at 0x" << std::hex << file.tellg () << std::endl;
					DecodeSample ( file, smpBytes );
					std::stringstream path;
					path << std::hex << "smpa_" << (smpA++) << ".pcm";
					DumpBytes ( path.str (), smpBytes );
				}
				else
				if ( byte == 0x83 )
				{
					std::cout << "ADPCM-B data found at 0x" << std::hex << file.tellg () << std::endl;
					DecodeSample ( file, smpBytes );
					std::stringstream path;
					path << std::hex << "smpb_" << (smpB++) << ".pcm";
					DumpBytes ( path.str (), smpBytes );
				}
			}
		}
	}

	file.close ();

	return 0;
}
