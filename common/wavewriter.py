import struct
from abc import abstractmethod
from enum import Enum
from typing import BinaryIO, List
from riffwriter import RiffFile, AbstractRiffChunk


class WaveSampleFormat(Enum):
	PCM        = 0x0001
	IEEE_FLOAT = 0x0003
	ALAW       = 0x0006
	MULAW      = 0x0007
	EXTENSIBLE = 0xFFFE


class WaveAbstractFormatChunk(AbstractRiffChunk):
	def fourcc(self) -> bytes: return b"fmt "

	def size(self) -> int: return 16

	@abstractmethod
	def sampleformat(self) -> WaveSampleFormat: raise NotImplementedError

	@abstractmethod
	def channels(self) -> int: raise NotImplementedError

	@abstractmethod
	def samplerate(self) -> int: raise NotImplementedError

	@abstractmethod
	def byterate(self) -> int: raise NotImplementedError

	@abstractmethod
	def align(self) -> int: raise NotImplementedError

	@abstractmethod
	def bitdepth(self) -> int: raise NotImplementedError

	def write(self, f: BinaryIO):
		f.write(struct.pack("<HHIIHH",
			self.sampleformat().value,
				self.channels(),
				self.samplerate(),
				self.byterate(),
				self.align(),
				self.bitdepth()))


class WavePcmFormatChunk(WaveAbstractFormatChunk):
	def sampleformat(self) -> WaveSampleFormat: return WaveSampleFormat.PCM

	def channels(self) -> int: return self._channels

	def samplerate(self) -> int: return self._samplerate

	def byterate(self) -> int: return self._samplerate * self._channels * self._bytedepth

	def align(self) -> int: return self._channels * self._bytedepth

	def bitdepth(self) -> int: return self._bytedepth * 8

	def __init__(self, channels: int, samplerate: int, bitdepth: int):
		if channels < 0 or channels >= 256: raise ValueError
		if samplerate < 1 or samplerate > 0xFFFFFFFF: raise ValueError
		if bitdepth not in [8, 16, 32]: raise ValueError
		self._channels = channels
		self._samplerate = samplerate
		self._bytedepth = bitdepth // 8


class WaveDataChunk(AbstractRiffChunk):
	def fourcc(self) -> bytes: return b"data"

	def size(self) -> int: return len(self._data)

	def write(self, f: BinaryIO): f.write(self._data)

	def __init__(self, data: bytes):
		self._data = data


class WaveCommentChunk(AbstractRiffChunk):
	def fourcc(self) -> bytes: return b"clm "

	def size(self) -> int: return len(self._comment)

	def write(self, f: BinaryIO): f.write(self._comment)

	def __init__(self, comment: bytes):
		self._comment = comment


class WaveFile(RiffFile):
	def __init__(self, format: WaveAbstractFormatChunk, chunks: List[AbstractRiffChunk]):
		super().__init__(b"WAVE", [format] + chunks)
