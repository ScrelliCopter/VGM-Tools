# wavesampler.py -- Support for the non-standard "Sampler" WAVE chunk
# (C) 2023 a dinosaur (zlib)

import struct
from enum import Enum
from typing import BinaryIO, List
from common.riffwriter import AbstractRiffChunk


class WaveSamplerSMPTEOffset:
	def __init__(self, hours: int=0, minutes: int=0, seconds: int=0, frames: int=0):
		if -23 > hours > 23:  raise ValueError("Hours out of range")
		if 0 > minutes > 59:  raise ValueError("Minutes out of range")
		if 0 > seconds > 59:  raise ValueError("Seconds out of range")
		if 0 > frames > 0xFF: raise ValueError("Frames out of range")
		self._hours   = hours
		self._minutes = minutes
		self._seconds = seconds
		self._frames  = frames

	def hours(self) -> int:   return self._hours
	def minutes(self) -> int: return self._minutes
	def seconds(self) -> int: return self._seconds
	def frames(self) -> int:  return self._frames

	def pack(self) -> bytes:
		#FIXME: endianess??
		return struct.pack("<bBBB", self._hours, self._minutes, self._seconds, self._frames)


class WaveSamplerLoopType(Enum):
	FORWARD = 0
	BIDIRECTIONAL = 1
	REVERSE = 2


class WaveSamplerLoop:
	def __init__(self,
 			cueId: int=0,
			type: int|WaveSamplerLoopType=0,
			start: int=0,
			end: int=0,
			fraction: int=0,
			loopCount: int=0):
		self._cueId = cueId
		self._type = type.value if type is WaveSamplerLoopType else type
		self._start = start
		self._end = end
		self._fraction = fraction
		self._loopCount = loopCount

	def pack(self) -> bytes:
		return struct.pack("<IIIIII",
			self._cueId,      # Cue point ID
			self._type,       # Loop type
			self._start,      # Loop start
			self._end,        # Loop end
			self._fraction,   # Fraction (none)
			self._loopCount)  # Loop count (infinite)


class WaveSamplerChunk(AbstractRiffChunk):
	def fourcc(self) -> bytes: return b"smpl"

	def loopsSize(self) -> int: return len(self._loops) * 24

	def size(self) -> int: return 36 + self.loopsSize()

	def write(self, f: BinaryIO):
		#TODO: unused data dummied out for now
		f.write(struct.pack("<4sIiiii4sII",
			self._manufacturer,         # MMA Manufacturer code
			self._product,              # Product
			self._period,               # Playback period (ns)
			self._unityNote,            # MIDI unity note
			self._fineTune,             # MIDI pitch fraction
			self._smpteFormat,          # SMPTE format
			self._smpteOffset.pack(),   # SMPTE offset

			len(self._loops),           # Number of loops
			self.loopsSize()))          # Loop data length
		f.writelines(loop.pack() for loop in self._loops)

	def __init__(self,
			manufacturer: bytes|None=None,
			product: int=0,
			period: int=0,
			midiUnityNote: int=0,
			midiPitchFraction: int=0,
			smpteFormat: int=0,
			smpteOffset: WaveSamplerSMPTEOffset|None=None,
			loops: List[WaveSamplerLoop]=None):
		if manufacturer is not None:
			if len(manufacturer) not in [1, 3]: raise ValueError("Malformed MIDI manufacturer code")
			self._manufacturer = len(manufacturer).to_bytes(1, byteorder="little", signed=False)
			self._manufacturer += manufacturer.rjust(3, b"\x00")
		else:
			self._manufacturer = b"\x00" * 4

		if 0 > product > 0xFFFF: raise ValueError("Product code out of range")
		self._product = product # Arbitrary vendor specific product code, dunno if this should be signed or unsigned
		self._period = period # Sample period in ns, (1 / samplerate) * 10^9, who cares
		if 0 > midiUnityNote > 127: raise ValueError("MIDI Unity note out of range")
		self._unityNote = midiUnityNote # MIDI note that plays the sample unpitched, middle C=60
		self._fineTune = midiPitchFraction # Finetune fraction, 256 == 100 cents
		if smpteFormat not in [0, 24, 25, 29, 30]: raise ValueError("Invalid SMPTE format")
		self._smpteFormat = smpteFormat
		self._smpteOffset = smpteOffset if (smpteOffset and smpteFormat > 0) else WaveSamplerSMPTEOffset()
		if self._smpteOffset.frames() > self._smpteFormat: raise ValueError("SMPTE frame offset can't exceed SMPTE format")
		self._loops = loops if loops is not None else list()
