#!/usr/bin/env python3
# Name:        feropm.py
# Copyright:   © 2020 a dinosaur
# License:     Zlib (https://opensource.org/licenses/Zlib)
# Description: Script to convert csMD presets to VOPM files.
#              Based on documentation provided by MovieMovies1.

from argparse import ArgumentParser
from xml.dom import minidom
from pathlib import Path
from typing import TextIO


class Preset:
	class Operator:
		tl = 0
		ml = 0
		dt = 0
		ar = 0
		d1 = 0
		sl = 0
		d2 = 0
		r = 0
		am_mask = 0
		ssg_eg = 0
		kr = 0
		velo = 0
		ks_lvl = 0

	def __init__(self):
		self.name = ""
		self.alg = 0
		self.fb = 0
		self.lfo_vib = 0
		self.lfo_trem = 0
		self.op = [self.Operator() for i in range(4)]


def parse_fermatap(path: str) -> Preset:
	xdoc = minidom.parse(path)
	xpreset = xdoc.getElementsByTagName("Preset")
	xtarget = xpreset[0].getElementsByTagName("Target")
	xparams = xtarget[0].getElementsByTagName("Param")

	patch = Preset()

	clamp = lambda x, a, b: x if x < b else b if x > a else a
	invert = lambda x, a, b: b - clamp(x, a, b)

	def parseop(op, id, x):
		if id == 0:
			patch.op[op].tl = invert(x, 0, 127)
		elif id == 1:
			patch.op[op].ml = clamp(x, 0, 15)
		elif id == 2:
			x = clamp(x, -3, 3)
			patch.op[op].dt = x if x >= 0 else 4 - x
		elif id == 3:
			patch.op[op].ar = invert(x, 0, 31)
		elif id == 4:
			patch.op[op].d1 = invert(x, 0, 31)
		elif id == 5:
			patch.op[op].sl = invert(x, 0, 15)
		elif id == 6:
			patch.op[op].d2 = invert(x, 0, 31)
		elif id == 7:
			patch.op[op].r = invert(x, 0, 15)
		elif id == 8:
			patch.op[op].am_mask = clamp(x, 0, 1)
		elif id == 9:
			patch.op[op].ssg_eg = clamp(x, 0, 8)
		elif id == 10:
			patch.op[op].kr = clamp(x, 0, 3)
		elif id == 11:
			patch.op[op].velo = clamp(x, 0, 3)
		elif id == 12:
			patch.op[op].ks_lvl = clamp(x, 0, 99)

	for i in xparams:
		id = int(i.attributes["id"].value)
		x = int(i.attributes["value"].value)

		if 0 <= id <= 15:
			parseop(0, id, x)
		elif id <= 31:
			parseop(1, id - 16, x)
		elif id <= 47:
			parseop(2, id - 32, x)
		elif id <= 63:
			parseop(3, id - 48, x)
		elif id == 64:
			patch.alg = clamp(x, 0, 7)
		elif id == 65:
			patch.fb = clamp(x, 0, 7)
		elif id == 66:
			patch.lfo_vib = clamp(x, 0, 7)
		elif id == 67:
			patch.lfo_trem = clamp(x, 0, 3)
		elif id <= 71:
			pass
		else:
			print("unrecognised parameter id {}".format(id))

	return patch


def save_vopm(path: Path, patch: Preset):
	fmtnum = lambda n: " " + f"{n}".rjust(3, " ")

	def writech(file: TextIO):
		file.write("CH:")
		file.write(fmtnum(64))
		file.write(fmtnum(patch.fb))
		file.write(fmtnum(patch.alg))
		file.write(fmtnum(patch.lfo_trem))
		file.write(fmtnum(patch.lfo_vib))
		file.write(fmtnum(120))
		file.write(fmtnum(0))
		file.write("\n")

	def writeop(file: TextIO, label: str, op: Preset.Operator):
		file.write(label)
		for i in [op.ar, op.d1, op.d2, op.r, op.sl, op.tl, op.kr, op.ml, op.dt, 0, op.am_mask]:
			file.write(fmtnum(i))
		file.write("\n")

	with path.open("w", encoding="utf-8") as file:
		file.write(f"@:0 {patch.name}\n")
		file.write("LFO: 0 0 0 0 0\n")
		writech(file)
		writeop(file, "M1:", patch.op[0])
		writeop(file, "C1:", patch.op[1])
		writeop(file, "M2:", patch.op[2])
		writeop(file, "C2:", patch.op[3])


def main():
	p = ArgumentParser(description="Convert fermatap-formatted csMD presets to VOPM (.opm) files.")
	p.add_argument("infile", help="Path to input csMD (.fermatap) file")
	p.add_argument("--output", "-o", type=Path, required=False, help="Name of output VOPM (.opm) file")

	args = p.parse_args()
	patch = parse_fermatap(args.infile)
	patch.name = args.infile.rstrip(".fermatap")

	outpath = args.output if args.output is not None else Path(patch.name + ".opm")
	save_vopm(outpath, patch)


if __name__ == "__main__":
	main()
