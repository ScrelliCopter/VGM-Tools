#!/usr/bin/env python3
# Name:        sinharmonicswt.py
# Copyright:   © 2023 a dinosaur
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

	#TODO: probably make bandlimited versions of the nonlinear waves
	def hsinetable16(size: int, harmonic: int):
		# Generate one half of a sine wave with the negative pole hard clipped off
		for i in range(size):
			sample = clamp2short(max(0.0, math.sin(i / size * math.tau * harmonic)))
			yield sample.to_bytes(2, byteorder="little", signed=True)

	#TODO: probably make bandlimited versions of the nonlinear waves
	def asinetable16(size: int, harmonic: int):
		# Generate a sine wave with the negative pole mirrored positively
		for i in range(size):
			sample = clamp2short(math.fabs(math.sin(i / size * math.pi * harmonic)))
			yield sample.to_bytes(2, byteorder="little", signed=True)

	outfolder = Path("FM Harmonics")
	outfolder.mkdir(exist_ok=True)

	# Build queue of files to generate
	GenItem = NamedTuple("GenItem", generator=any, steps=int, mode=SerumWavetableInterpolation, name=str)
	genqueue: list[GenItem] = list()
	# All waveform types with 64 harmonic steps in stepped and linear versions
	for mode in [("", SerumWavetableInterpolation.NONE), (" (XFade)", SerumWavetableInterpolation.LINEAR_XFADE)]:
		for generator in [("Sine", sinetable16), ("Half Sine", hsinetable16), ("Abs Sine", asinetable16)]:
			genqueue.append(GenItem(generator[1], 64, mode[1], f"{generator[0]} Harmonics{mode[0]}"))
	# Shorter linear versions of hsine and asine
	for steps in [8, 16, 32]:
		spec = SerumWavetableInterpolation.LINEAR_XFADE
		for generator in [("Half Sine", hsinetable16), ("Abs Sine", asinetable16)]:
			genqueue.append(GenItem(generator[1], steps, spec, f"{generator[0]} (XFade {steps})"))

	# Generate & write wavetables
	for i in genqueue:
		write_wavetable(str(outfolder.joinpath(f"{i.name}.wav")), i.generator, i.mode, i.steps)


if __name__ == "__main__":
	main()
