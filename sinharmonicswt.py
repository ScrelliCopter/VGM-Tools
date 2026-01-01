#!/usr/bin/env python3
# Name:        sinharmonicswt.py
# Copyright:   © 2023, 2025, 2026 a dinosaur
# Homepage:    https://github.com/ScrelliCopter/VGM-Tools
# License:     Zlib (https://opensource.org/licenses/Zlib)
# Description: Generate Serum format wavetables of the harmonic series
#              for a handful of common FM waveforms, intended for improving
#              the workflow of creating FM sounds in Vital.

import math
from pathlib import Path
from typing import NamedTuple

from common.wavewriter import WaveFile, WavePcmFormatChunk, WaveDataChunk
from common.waveserum import WaveSerumCommentChunk, SerumWavetableInterpolation


def write_wavetable(name: str, generator,
		mode: SerumWavetableInterpolation = SerumWavetableInterpolation.NONE,
		num: int = 64, size: int = 2048):
	def sweep_table() -> bytes:
		for i in range(num - 1):
			yield b"".join(generator(size, i + 1))

	with open(name, "wb") as f:
		WaveFile(WavePcmFormatChunk(1, 44100, 16), [
			WaveSerumCommentChunk(size, mode),
			WaveDataChunk(b"".join(sweep_table()))
		]).write(f)


def main():
	clip = lambda x, a, b: max(a, min(b, x))
	def clamp2short(a: float) -> int: return clip(int(a * 0x7FFF), -0x8000, 0x7FFF)

	def sinetable16(size: int, harmonic: int):
		# Generate a simple sine wave
		for i in range(size):
			sample = clamp2short(math.sin(i / size * math.tau * harmonic))
			yield sample.to_bytes(2, byteorder="little", signed=True)

	def hsinetable16(size: int, harmonic: int, bandlimit: bool = True):
		# Generate one half of a sine wave with the negative pole hard clipped off
		for i in range(size):
			theta = math.tau * i / size
			if not bandlimit:
				y = max(0.0, math.sin(theta * harmonic)) - 1 / math.pi
			else:
				def harmonics():
					n = 1
					while True:
						harm_freq = 2 * n * harmonic
						if 2 * harm_freq > size:
							break
						yield math.cos(harm_freq * theta) / (4 * n * n - 1)
						n += 1
				y = math.sin(theta * harmonic) / 2 - 2 / math.pi * sum(harmonics())
			yield clamp2short(y).to_bytes(2, byteorder="little", signed=True)

	def asinetable16(size: int, harmonic: int, bandlimit: bool = True):
		# Generate a sine wave with the negative pole mirrored positively
		for i in range(size):
			theta = math.pi * i / size
			if not bandlimit:
				y = math.fabs(math.sin(theta * harmonic)) - 2 / math.pi
			else:
				y = 0
				n = 1
				while True:
					harm_freq = 2 * n * harmonic
					if harm_freq > size:
						break
					y += math.cos(harm_freq * theta) / (4 * n * n - 1)
					n += 1
				y = -(4 / math.pi * y)
			yield clamp2short(y).to_bytes(2, byteorder="little", signed=True)

	def triangletable(size: int, harmonic: int, bandlimit: bool = True):
		for i in range(size):
			t = i / size
			if not bandlimit:
				phase = math.fmod(t * harmonic + 0.25, 1)
				y = 4 * (phase if phase < 0.5 else 1 - phase) - 1
			else:
				y = 0
				n = 0
				while True:
					n2a1 = 2 * n + 1
					harm_freq = n2a1 * harmonic
					if 2 * harm_freq > size:
						break
					y += math.pow(-1, n) / (n2a1 * n2a1) * math.sin(math.tau * harm_freq * t)
					n += 1
				y *= 8 / (math.pi * math.pi)
			yield clamp2short(y).to_bytes(2, byteorder="little", signed=True)

	def squaretable16(size: int, harmonic: int, bandlimit: bool = True):
		for i in range(size):
			if not bandlimit:
				y = 1 if math.fmod(harmonic * i / size, 1.0) < 0.5 else -1
			else:
				y = 0
				n = 1
				while True:
					n2s1 = 2 * n - 1
					harm_freq = n2s1 * harmonic
					if 2 * harm_freq > size:
						break
					y += math.sin(math.tau * harm_freq * (i + 1) / size) / n2s1
					n += 1
				y *= 4 / math.pi
			y *= 0.8
			yield clamp2short(y).to_bytes(2, byteorder="little", signed=True)

	def sawtable16(size: int, harmonic: int, bandlimit: bool = True):
		offset = 1 / (2 * harmonic)
		for i in range(size):
			t = i / size
			if not bandlimit:
				y = math.fmod(2 * t * harmonic + 1, 2) - 1
			else:
				y = 0
				n = 1
				while True:
					harm_freq = n * harmonic
					if 2 * harm_freq > size:
						break
					y += math.sin(math.tau * harm_freq * (t + offset)) / n
					n += 1
				y = -(2 / math.pi * y)
			y *= 0.8
			yield clamp2short(y).to_bytes(2, byteorder="little", signed=True)


	outfolder = Path("FM Harmonics")
	outfolder.mkdir(exist_ok=True)

	# Build queue of files to generate
	GenItem = NamedTuple("GenItem", generator=any, steps=int, mode=SerumWavetableInterpolation, name=str)
	genqueue: list[GenItem] = list()
	# All waveform types with 64 harmonic steps with no interpolation
	for mode in [("", SerumWavetableInterpolation.NONE)]:
		for generator in [
				("Sine", sinetable16), ("Triangle", triangletable),
				("Half Sine", hsinetable16), ("Abs Sine", asinetable16),
				("Square", squaretable16), ("Saw", sawtable16)]:
			genqueue.append(GenItem(generator[1], 64, mode[1], f"{generator[0]} Harmonics{mode[0]}"))
	# Shorter crossfaded versions of tri, hsine, asine, square, and saw
	for steps in [8, 16, 24, 32, 48]:
		spec = SerumWavetableInterpolation.LINEAR_XFADE
		for generator in [
				("Triangle", triangletable),
				("Half Sine", hsinetable16), ("Abs Sine", asinetable16),
				("Square", squaretable16), ("Saw", sawtable16)]:
			genqueue.append(GenItem(generator[1], steps, spec, f"{generator[0]} Harmonics (XFade {steps})"))

	# Generate & write wavetables
	for i in genqueue:
		write_wavetable(str(outfolder.joinpath(f"{i.name}.wav")), i.generator, i.mode, i.steps)


if __name__ == "__main__":
	main()
