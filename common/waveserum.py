# waveserum.py -- Serum comment chunk support
# (C) 2023 a dinosaur (zlib)

from enum import Enum
from common.wavewriter import WaveCommentChunk


class SerumWavetableInterpolation(Enum):
	NONE = 0
	LINEAR_XFADE = 1
	SPECTRAL_MORPH = 2


class WaveSerumCommentChunk(WaveCommentChunk):
	def __init__(self, size: int, mode: SerumWavetableInterpolation, factory=False):
		comment = f"<!>{size: <4} {mode.value}{'1' if factory else '0'}000000"
		comment += " wavetable (www.xferrecords.com)"
		super().__init__(comment.encode("ascii"))
