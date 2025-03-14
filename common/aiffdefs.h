#ifndef AIFFDEFS_H
#define AIFFDEFS_H

#include "iff.h"

#define FOURCC_AIFF IFF_FOURCC('A', 'I', 'F', 'F')

#define AIFF_FOURCC_COMM IFF_FOURCC('C', 'O', 'M', 'M')
#define AIFF_FOURCC_SSND IFF_FOURCC('S', 'S', 'N', 'D')
#define AIFF_FOURCC_MARK IFF_FOURCC('M', 'A', 'R', 'K')
#define AIFF_FOURCC_INST IFF_FOURCC('I', 'N', 'S', 'T')
#define AIFF_FOURCC_MIDI IFF_FOURCC('M', 'I', 'D', 'I')
#define AIFF_FOURCC_AESD IFF_FOURCC('A', 'E', 'S', 'D')
#define AIFF_FOURCC_APPL IFF_FOURCC('A', 'P', 'P', 'L')
#define AIFF_FOURCC_COMT IFF_FOURCC('C', 'O', 'M', 'T')

typedef struct
{
	uint16_t signExponent;
	uint64_t mantissa;

} extended;

#define AIFF_COMMON_SIZE 18
typedef struct
{
	int16_t numChannels;
	uint32_t numSampleFrames;
	int16_t sampleSize;
	extended sampleRate;

} AIFFCommon;

#define AIFF_SOUNDDATAHEADER_SIZE 8
typedef struct
{
	uint32_t offset;
	uint32_t blockSize;
	const uint8_t* soundData;

} AIFFSoundDataHeader;

typedef int16_t AIFFMarkerID;
typedef struct
{
	AIFFMarkerID id;
	uint32_t position;
	const char* name;

} AIFFMarker;

typedef int16_t AIFFMarkerPlayMode;
enum AIFFMarkerPlayModeEnum
{
	AIFF_LOOP_OFF     = 0,
	AIFF_LOOP_FORWARD = 1,
	AIFF_LOOP_REVERSE = 2
};

#define AIFF_LOOP_SIZE 6
typedef struct
{
	AIFFMarkerPlayMode playMode;
	AIFFMarkerID beginLoop;
	AIFFMarkerID endLoop;

} AIFFLoop;

#define AIFF_INSTRUMENT_SIZE (8 + 2 * AIFF_LOOP_SIZE)
typedef struct
{
	int8_t baseNote;                    // Unity MIDI note where the sample plays back without pitch modification
	int8_t detune;                      // Pitch detune in cents (50 = 1/2 semitone)
	int8_t lowNote, highNote;           // Range of MIDI notes that the sample should play, 0-127 inclusive
	int8_t lowVelocity, highVelocity;   // Range of MIDI velocities that the sample should play, 0-127 inclusive
	int16_t gain;                       // Relative gain adjustment of each sample in dB
	AIFFLoop sustainLoop, releaseLoop;

} AIFFInstrument;

#endif//AIFFDEFS_H
