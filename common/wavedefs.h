#ifndef WAVEDEFS_H
#define WAVEDEFS_H

#include <stdint.h>
#include "iff.h"

#define WAVE_FOURCC_RIFF IFF_FOURCC('R', 'I', 'F', 'F')
#define WAVE_FOURCC_WAVE IFF_FOURCC('W', 'A', 'V', 'E')
#define WAVE_FOURCC_FMT  IFF_FOURCC('f', 'm', 't', ' ')
#define WAVE_FOURCC_DATA IFF_FOURCC('d', 'a', 't', 'a')

#define WAVE_FOURCC_SMPL IFF_FOURCC('s', 'm', 'p', 'l')
//#define WAVE_FOURCC_INST IFF_FOURCC('i', 'n', 's', 't')

typedef struct
{
	IffFourCC fourcc;
	uint32_t  size;

} RiffChunk;

#define FORMAT_CHUNK_SIZE 16
typedef struct
{
	uint16_t format;
	uint16_t channels;
	uint32_t samplerate;
	uint32_t byterate;
	uint16_t alignment;
	uint16_t bitdepth;

} FormatChunk;

#define SMPL_CHUNK_HEAD_SIZE 36
typedef struct
{
	uint32_t manufacturer;     // MMA Manufacturer code
	uint32_t product;          // Arbitrary vendor specific product code
	int32_t samplePeriod;      // Playback period in ns, (1 / samplerate) * 10^9
	int32_t midiUnityNote;     // MIDI unity note (0-127, 60 = Middle C, 69 = Concert A)
	int32_t midiPitchFrac;     // MIDI pitch finetune fraction (256 == 100 cents)
	int32_t smpteFormat;       // SMPTE format (0, 24, 25, 29, or 30)
	uint32_t smpteOffset;      // SMPTE offset (bBBB) (hr -23-23, min 0-59, s 0-59, fr 0-255)
	uint32_t sampleLoopCount;  // Number of loops
	uint32_t sampleData;       // Vendor specific sampler data

} SamplerChunk;

typedef enum
{
	LOOP_TYPE_FORWARD        =  0,
	LOOP_TYPE_BIDIRECTIONAL  =  1,
	LOOP_TYPE_REVERSE        =  2,
	LOOP_TYPE_RESERVED_START =  3,
	LOOP_TYPE_VENDOR_START   = 32

} SamplerLoopType;

#define SAMPLER_LOOP_SIZE 24
typedef struct
{
	uint32_t id;                    // Cue point ID
	uint32_t type;                  // Loop type
	uint32_t loopStart, loopEnd;    // Loop range (includes sample at loopend position)
	uint32_t fraction;              // Fine tune fraction (0x80 = 50 cents = 1/2 semitone)
	uint32_t playCount;             // Loop count (0 = infinite)

} SampleLoopChunk;

/*
#define INST_CHUNK_SIZE 7
typedef struct
{
	uint8_t unshiftedNode;              // Same as midiUniNote in 'smpl'
	int8_t fineTune;                    // Same as AIFF detune (deviation from pitch in cents, +50 = 1/2 semitone)
	int8_t gain;                        // Relative gain adjustment of each sample in dB
	uint8_t lowNote, highNote;          // Suggested MIDI note range (0-127, inclusive)
	uint8_t lowVelocity, highVelocity;  // Suggested velocity range (0-127, inclusive)

} InstrumentChunk;
*/

#endif//WAVEDEFS_H
