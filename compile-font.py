#!/usr/bin/env python3
import sys
from PIL import Image

def main(args):
	if len(args) < 6:
		print("Usage: <input png> <output c header> <columns> <rows> <glyph width> <glyph height>")
		return

	columns = int(args[2])
	rows = int(args[3])
	gw = int(args[4])
	gh = int(args[5])

	n_glyphs = columns * rows
	canvas_width = columns * gw
	canvas_height = rows * gh
	bytes_per_glyph = (gw * gh + 7) // 8

	glyphs = [None] * n_glyphs
	for i in range(n_glyphs):
		glyphs[i] = bytearray(bytes_per_glyph)

	with Image.open(args[0]) as img:
		pixels = list(img.getdata())
		i = -1
		while i < len(pixels) - 1:
			i += 1
			if pixels[i][0] + pixels[i][1] + pixels[i][2] < 384:
				continue

			x = i % img.width
			y = i // img.width
			if x >= canvas_width:
				continue
			if y >= canvas_height:
				break

			idx = (x // gw) + columns * (y // gh)
			pos = (x % gw) + gw * (y % gh)
			glyphs[idx][pos // 8] |= 1 << (7 - (pos % 8))

	out = "#pragma once\n\n"
	out += "static const int _glyph_width = {0};\n".format(gw)
	out += "static const int _glyph_height = {0};\n".format(gh)
	out += "static const int _glyph_size = {0};\n".format(bytes_per_glyph)
	out += "static const int _glyph_count = {0};\n".format(n_glyphs)
	out += "\nstatic const unsigned char _glyph_data[] = {\n"

	data_size = n_glyphs * bytes_per_glyph
	i = -1
	while i < data_size - 1:
		i += 1
		out += "0x"
		b = glyphs[i // bytes_per_glyph][i % bytes_per_glyph]
		h = (b >> 4) & 0xf
		h += 0x30 if h < 10 else 0x57
		l = b & 0xf
		l += 0x30 if l < 10 else 0x57
		out += chr(h)
		out += chr(l)
		out += ","
		out += chr(0x0a if (i & 0xf) == 0xf else 0x20)

	out += "\n};\n"
	with open(args[1], "w") as f:
		f.write(out)

if len(sys.argv) > 1:
	main(sys.argv[1:])
else:
	main(["tw-ascii.png", "include/font.h", "16", "6", "8", "10"])
