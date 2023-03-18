/* (c) 2017 Alex Barney (MIT) */

#include <stdint.h>
#include "common.h"
#include "dsptool.h"

static inline uint8_t GetHighNibble(uint8_t value)
{
	return value >> 4 & 0xF;
}

static inline uint8_t GetLowNibble(uint8_t value)
{
	return value & 0xF;
}

static inline int16_t Clamp16(int value)
{
	if (value > INT16_MAX)
		return INT16_MAX;
	if (value < INT16_MIN)
		return INT16_MIN;
	return (int16_t)value;
}

void decode(uint8_t* src, int16_t* dst, ADPCMINFO* cxt, uint32_t samples)
{
	short hist1 = cxt->yn1;
	short hist2 = cxt->yn2;
	short* coefs = cxt->coef;
	uint32_t frameCount = (samples + SAMPLES_PER_FRAME - 1) / SAMPLES_PER_FRAME;
	uint32_t samplesRemaining = samples;

	for (uint32_t i = 0; i < frameCount; i++)
	{
		int predictor = GetHighNibble(*src);
		int scale = 1 << GetLowNibble(*src++);
		short coef1 = coefs[predictor * 2];
		short coef2 = coefs[predictor * 2 + 1];

		uint32_t samplesToRead = MIN(SAMPLES_PER_FRAME, samplesRemaining);

		for (uint32_t s = 0; s < samplesToRead; s++)
		{
			int sample = (s & 0x1)
				? GetLowNibble(*src++)
				: GetHighNibble(*src);
			sample = sample >= 8 ? sample - 16 : sample;
			sample = (scale * sample) << 11;
			sample = (sample + 1024 + (coef1 * hist1 + coef2 * hist2)) >> 11;
			short finalSample = Clamp16(sample);

			hist2 = hist1;
			hist1 = finalSample;

			*dst++ = finalSample;
		}

		samplesRemaining -= samplesToRead;
	}
}

void getLoopContext(uint8_t* src, ADPCMINFO* cxt, uint32_t samples)
{
	short hist1 = cxt->yn1;
	short hist2 = cxt->yn2;
	short* coefs = cxt->coef;
	uint8_t ps = 0;
	int frameCount = (int)((samples + SAMPLES_PER_FRAME - 1) / SAMPLES_PER_FRAME);
	uint32_t samplesRemaining = samples;

	for (int i = 0; i < frameCount; i++)
	{
		ps = *src;
		int predictor = GetHighNibble(*src);
		int scale = 1 << GetLowNibble(*src++);
		short coef1 = coefs[predictor * 2];
		short coef2 = coefs[predictor * 2 + 1];

		uint32_t samplesToRead = MIN(SAMPLES_PER_FRAME, samplesRemaining);

		for (uint32_t s = 0; s < samplesToRead; s++)
		{
			int sample = (s & 0x1)
				? GetLowNibble(*src++)
				: GetHighNibble(*src);
			sample = sample >= 8 ? sample - 16 : sample;
			sample = (scale * sample) << 11;
			sample = (sample + 1024 + (coef1 * hist1 + coef2 * hist2)) >> 11;
			short finalSample = Clamp16(sample);

			hist2 = hist1;
			hist1 = finalSample;
		}
		samplesRemaining -= samplesToRead;
	}

	cxt->loop_pred_scale = ps;
	cxt->loop_yn1 = hist1;
	cxt->loop_yn2 = hist2;
}
