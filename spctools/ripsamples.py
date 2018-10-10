#!/usr/bin/env python3

''' ripsamples.py -- a python script for mass extracting samples from SPC files.

  Copyright (C) 2018 Nicholas Curtis (a_dinosaur)

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

'''

import os
import subprocess
import pathlib
import struct
import hashlib

# Directory constants.
SPCDIR = "./spc"
ITDIR  = "./it"
SMPDIR = "./sample"

# External programs used by this script.
SPC2IT = "spc2it"


class Sample:
	length  = 0
	loopBeg = 0
	loopEnd = 0
	rate    = 0

	data = None

def writesmp(smp, path):
	with open(path, "wb") as wav:

		# Make sure sample rate is nonzero.
		#TODO: figure out why this even happens...
		if smp.rate == 0:
			smp.rate = 32000
			#print(path + " may be corrupted...")

		writeLoop = True if smp.loopEnd > smp.loopBeg else False

		# Write RIFF chunk.
		wav.write(b"RIFF")
		# Size of entire file following
		riffSize = 104 if writeLoop else 36
		wav.write(struct.pack("<I", riffSize + smp.length * 2))
		wav.write(b"WAVE")

		# Write fmt chunk.
		wav.write(b"fmt ")
		wav.write(struct.pack("<I", 16))            # Subchunk size.
		wav.write(struct.pack("<H", 1))             # Audio format (uncompressed)
		wav.write(struct.pack("<H", 1))             # Channel count (mono)
		wav.write(struct.pack("<I", smp.rate))      # Samplerate
		wav.write(struct.pack("<I", smp.rate * 2 )) # Byte rate (16 bit mono)
		wav.write(struct.pack("<H", 2))             # Bytes per sample (16 bit mono)
		wav.write(struct.pack("<H", 16))            # Bits per sample (16 bit)

		# Write sampler chunk (if looped).
		if writeLoop:
			wav.write(b"smpl")
			wav.write(struct.pack("<I", 60))    # Chunk size (36 + loops * 24)
			wav.write(b"\x00\x00\x00\x00")      # Manufacturer
			wav.write(b"\x00\x00\x00\x00")      # Product
			wav.write(b"\x00\x00\x00\x00")      # Sample period
			wav.write(b"\x00\x00\x00\x00")      # MIDI unity note
			wav.write(b"\x00\x00\x00\x00")      # MIDI pitch fraction
			wav.write(b"\x00\x00\x00\x00")      # SMPTE format
			wav.write(b"\x00\x00\x00\x00")      # SMPTE offset
			wav.write(struct.pack("<I", 1))     # Loop count
			wav.write(struct.pack("<I", 24))    # Loop data length

			wav.write(struct.pack("<I", 0))           # Cue point ID (none)
			wav.write(struct.pack("<I", 0))           # Loop type (forward)
			wav.write(struct.pack("<I", smp.loopBeg)) # Loop start
			wav.write(struct.pack("<I", smp.loopEnd)) # Loop end
			wav.write(struct.pack("<I", 0))           # Fraction (none)
			wav.write(struct.pack("<I", 0))           # Loop count (infinite)

		# Write data chunk.
		wav.write(b"data")
		wav.write(struct.pack("<I", smp.length * 2))
		wav.write(smp.data)

def readsmp(f, ofs, idx):
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

	# Skip fname to flags & read.
	f.seek(ofs + 0x12)
	flags = int.from_bytes(f.read(1), byteorder="little", signed=False)

	# Read flag values.
	if not flags & 0b00000001: return None # Check sample data bit.
	loopBit = True if flags & 0b00010000 else False

	smp = Sample()

	# Read the rest of the header.
	f.seek(ofs + 0x30)
	smp.length = int.from_bytes(f.read(4), byteorder="little", signed=False)
	if loopBit:
		smp.loopBeg = int.from_bytes(f.read(4), byteorder="little", signed=False)
		smp.loopEnd = int.from_bytes(f.read(4), byteorder="little", signed=False)
	else:
		f.seek(8, 1) # Skip over.
		smp.loopBeg = 0
		smp.loopEnd = 0
	smp.rate = int.from_bytes(f.read(4), byteorder="little", signed=False)
	f.seek(8, 1) # Skip over sustain shit.

	# Read sample data.
	dataOfs = int.from_bytes(f.read(4), byteorder="little", signed=False)
	smp.data = f.read(smp.length * 2)

	# Compute hash of data.
	#FIXME: This actually generates a butt ton of collisions...
	# there's got to be a better way!
	h = hashlib.md5(struct.pack("<pII", smp.data, smp.loopBeg, smp.loopEnd))
	smp.hash = h.hexdigest()

	return smp

def readit(path, outpath):
	with open(path, "r+b") as f:

		# Don't bother scanning non IT files.
		if f.read(4) != b"IMPM": return

		#print("Song name: " + f.read(26).decode('utf-8'))

		# Need order list size and num instruments to know how far to skip.
		f.seek(0x20)
		ordNum = int.from_bytes(f.read(2), byteorder="little", signed=False)
		insNum = int.from_bytes(f.read(2), byteorder="little", signed=False)
		smpNum = int.from_bytes(f.read(2), byteorder="little", signed=False)
		patNum = int.from_bytes(f.read(2), byteorder="little", signed=False)

		# Cus spc2it has a tendancy to produce corrupted files...
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

def scanit(srcPath, dstPath):
	for directory, subdirectories, files in os.walk(srcPath):
		for file in files:
			if file.endswith(".it"):
				path = os.path.join(directory, file)
				outpath = dstPath + path[len(srcPath):-len(file)]
				readit(path, outpath)

def scanspc(srcPath, dstPath):
	for directory, subdirectories, files in os.walk(srcPath):

		# Create output dir for each game.
		for sub in subdirectories:
			path = os.path.join(dstPath, sub)
			pathlib.Path(path).mkdir(parents=True, exist_ok=True)

		# Convert spc files.
		for file in files:
			if file.endswith(".spc"):
				# Don't convert files that have already been converted.
				itpath = os.path.join(dstPath + directory[len(srcPath):], file[:-3] + "it")
				if not os.path.isfile(itpath):
					path = os.path.join(directory, file)
					subprocess.call([SPC2IT, path])
					path = path[:-3] + "it"
					if os.path.isfile(path):
						os.rename(path, itpath)


# Actual main stuff.
scanspc(SPCDIR, ITDIR)
scanit(ITDIR, SMPDIR)
