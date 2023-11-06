from abc import ABC, abstractmethod
from typing import BinaryIO, List


class AbstractRiffChunk(ABC):
	@abstractmethod
	def fourcc(self) -> bytes: raise NotImplementedError

	@abstractmethod
	def size(self) -> int: raise NotImplementedError

	@abstractmethod
	def write(self, f: BinaryIO): raise NotImplementedError


class RiffFile(AbstractRiffChunk):
	def fourcc(self) -> bytes: return b"RIFF"

	def size(self) -> int: return 4 + sum(8 + c.size() for c in self._chunks)

	def __init__(self, type: bytes, chunks: List[AbstractRiffChunk]):
		self._chunks = chunks
		if len(type) != 4: raise ValueError
		self._type = type

	def write(self, f: BinaryIO):
		f.writelines([
			self.fourcc(),
			self.size().to_bytes(4, "little", signed=False),
			self._type])
		for chunk in self._chunks:
			size = chunk.size()
			if size & 0x3: raise AssertionError("Unaligned chunks will produce malformed riff files")
			f.writelines([
				chunk.fourcc(),
				size.to_bytes(4, "little", signed=False)])
			chunk.write(f)
