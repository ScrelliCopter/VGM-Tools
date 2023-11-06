#!/usr/bin/env python3

# ripsamples.py -- a python script for mass extracting samples from SPC files.
# (C) 2018, 2023 a dinosaur (zlib)

import os
import subprocess
import pathlib
import struct
import hashlib
from typing import BinaryIO
from io import BytesIO

import sys
sys.path.append("..")
from common.wavewriter import WaveFile, WavePcmFormatChunk, WaveDataChunk
from common.wavesampler import WaveSamplerChunk, WaveSamplerLoop


# Directory constants
SPCDIR = "./spc"
ITDIR  = "./it"
SMPDIR = "./sample"

# External programs used by this script
SPC2IT = "spc2it/spc2it"


class Sample:
	length  = 0
	loopBeg = 0
	loopEnd = 0
	rate    = 0

	data: bytes = None


def writesmp(smp: Sample, path: str):
	with open(path, "wb") as wav:
		# Make sure sample rate is nonzero
		#TODO: figure out why this even happens...
		if smp.rate == 0:
			smp.rate = 32000
			#print(path + " may be corrupted")

		fmtChunk = WavePcmFormatChunk(  # Audio format (uncompressed)
			1,                          # Channel count (mono)
			smp.rate,                   # Samplerate
			16)                         # Bits per sample (16 bit)
		dataChunk = WaveDataChunk(smp.data)
		loopChunk = None
		if smp.loopEnd > smp.loopBeg:
			loopChunk = WaveSamplerChunk(loops=[WaveSamplerLoop(
				start=smp.loopBeg,  # Loop start
				end=smp.loopEnd)])  # Loop end

		WaveFile(fmtChunk,
			[dataChunk] if loopChunk is None else [loopChunk, dataChunk]
		).write(wav)


def readsmp(f: BinaryIO, ofs: int, idx: int):
	# List of assumptions made:
	# - Samples are 16 bit
	# - Samples are mono
	# - Samples are signed
	# - No compression
	# - No sustain loop
	# - Loops are forwards
	# - Global volume is ignored

	f.seek(ofs)
	if f.read(4) != b"IMPS": return None

	# Skip fname to flags & read
	f.seek(ofs + 0x12)
	flags = int.from_bytes(f.read(1), byteorder="little", signed=False)

	# Read flag values
	if not flags & 0b00000001: return None  # Check sample data bit
	loopBit = True if flags & 0b00010000 else False

	smp = Sample()

	# Read the rest of the header
	f.seek(ofs + 0x30)
	smp.length = int.from_bytes(f.read(4), byteorder="little", signed=False)
	if loopBit:
		smp.loopBeg = int.from_bytes(f.read(4), byteorder="little", signed=False)
		smp.loopEnd = int.from_bytes(f.read(4), byteorder="little", signed=False)
	else:
		f.seek(8, 1)  # Skip over
		smp.loopBeg = 0
		smp.loopEnd = 0
	smp.rate = int.from_bytes(f.read(4), byteorder="little", signed=False)
	f.seek(8, 1)  # Skip over sustain shit

	# Read sample data
	dataOfs = int.from_bytes(f.read(4), byteorder="little", signed=False)
	smp.data = f.read(smp.length * 2)

	# Compute hash of data
	#FIXME: This actually generates a butt ton of collisions...
	# there's got to be a better way!
	h = hashlib.md5(struct.pack("<pII", smp.data, smp.loopBeg, smp.loopEnd))
	smp.hash = h.hexdigest()

	return smp


def readit(path: str, outpath: str):
	with open(path, "r+b") as f:

		# Don't bother scanning non IT files
		if f.read(4) != b"IMPM": return

		#print("Song name: " + f.read(26).decode('utf-8'))

		# Need order list size and num instruments to know how far to skip
		f.seek(0x20)
		ordNum = int.from_bytes(f.read(2), byteorder="little", signed=False)
		insNum = int.from_bytes(f.read(2), byteorder="little", signed=False)
		smpNum = int.from_bytes(f.read(2), byteorder="little", signed=False)
		patNum = int.from_bytes(f.read(2), byteorder="little", signed=False)

		# Cus spc2it has a tendency to produce corrupted files...
		if ordNum > 1024: return
		if smpNum > 4000: return
		if insNum > 256: return
		if patNum > 256: return

		smpOfsTable = 0xC0 + ordNum + insNum * 4

		for i in range(0, smpNum):
			f.seek(smpOfsTable + i * 4)
			smpOfs = int.from_bytes(f.read(4), byteorder="little", signed=False)
			smp = readsmp(f, smpOfs, i + 1)
			if smp != None:
				outwav = os.path.join(outpath, smp.hash + ".wav")
				if not os.path.isfile(outwav):
					pathlib.Path(outpath).mkdir(parents=True, exist_ok=True)
					writesmp(smp, outwav)


def scanit(srcPath: str, dstPath: str):
	for directory, subdirectories, files in os.walk(srcPath):
		for file in files:
			if file.endswith(".it"):
				path = os.path.join(directory, file)
				outpath = dstPath + path[len(srcPath):-len(file)]
				readit(path, outpath)


def scanspc(srcPath: str, dstPath: str):
	for directory, subdirectories, files in os.walk(srcPath):

		# Create output dir for each game
		for sub in subdirectories:
			path = os.path.join(dstPath, sub)
			pathlib.Path(path).mkdir(parents=True, exist_ok=True)

		# Convert spc files
		for file in files:
			if file.endswith(".spc"):
				# Don't convert files that have already been converted
				itpath = os.path.join(dstPath + directory[len(srcPath):], file[:-3] + "it")
				if not os.path.isfile(itpath):
					path = os.path.join(directory, file)
					subprocess.call([SPC2IT, path])
					path = path[:-3] + "it"
					if os.path.isfile(path):
						os.rename(path, itpath)


# Actual main stuff
if __name__ == "__main__":
	scanspc(SPCDIR, ITDIR)
	scanit(ITDIR, SMPDIR)
