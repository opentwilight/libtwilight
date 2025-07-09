#!/usr/bin/env python3

import os
import struct
import sys

def printUsage():
	print("dol_export.py [options]")
	print("Options:")
	print("  --help")
	print("     Print this help menu")
	print("  -o / --output <output file>")
	print("     Output DOL file")
	print("  -t / --text <input file> <file offset> <memory address> <size>")
	print("  -d / --data <input file> <file offset> <memory address> <size>")
	print("     Extract a section from an executable binary,")
	print("     and paste into the DOL along with a memory address.")
	print("     This option can be used multiple times to add multiple sections.")
	print("     If <input file> is \"-\", then the data is read from stdin.")
	print("  -b / --bss <address> <size>")
	print("     Address of BSS section in memory")
	print("  -e / --entrypoint <address>")
	print("     Entrypoint address in memory")

def loadSegment(segment, inputFileMap, outputData):
	fname = segment[0]
	offset = segment[1]
	address = segment[2]
	size = segment[3]

	if fname == "-":
		data = sys.stdin.buffer.read(size)
		offset = 0
	else:
		data = None
		try:
			data = inputFileMap[fname]
		except KeyError:
			pass

		if not data:
			with open(fname, "rb") as f:
				data = f.read()
			inputFileMap[fname] = data

	HEADER_SIZE = 228
	outOffset = HEADER_SIZE + len(outputData)
	outputData.extend(data[offset:offset+size])
	return outOffset, address, size

def main(args):
	if len(args) < 2:
		printUsage()
		return

	outFileName = ""
	codeSections = []
	dataSections = []
	entrypoint = 0
	bssAddr = 0
	bssSize = 0

	idx = -1
	while idx < len(args) - 1:
		idx += 1
		if args[idx] == "--help":
			printUsage()
			return
		elif args[idx] == "-o" or args[idx] == "--output":
			outFileName = args[idx+1]
			idx += 1
		elif args[idx] == "-t" or args[idx] == "--text":
			fname = args[idx+1]
			offset =  int(args[idx+2], 0)
			address = int(args[idx+3], 0)
			size =    int(args[idx+4], 0)
			codeSections.append([fname, offset, address, size])
			idx += 4
		elif args[idx] == "-d" or args[idx] == "--data":
			fname = args[idx+1]
			offset =  int(args[idx+2], 0)
			address = int(args[idx+3], 0)
			size =    int(args[idx+4], 0)
			dataSections.append([fname, offset, address, size])
			idx += 4
		elif args[idx] == "-b" or args[idx] == "--bss":
			bssAddr = int(args[idx+1], 0)
			bssSize = int(args[idx+2], 0)
			idx += 2
		elif args[idx] == "-e" or args[idx] == "--entrypoint":
			entrypoint = int(args[idx+1], 0)
			idx += 1

	if not outFileName:
		print("No output file was given: use --output")
		printUsage()
		return
	if len(codeSections) == 0:
		print("No code sections were specified: use --code")
		printUsage()
		return
	if entrypoint == 0:
		print("No entrypoint was specified: use --entrypoint")
		printUsage()
		return

	if len(codeSections) > 7:
		print("Too many text/code sections were given (maximum 7)")
		return
	if len(dataSections) > 7:
		print("Too many data sections were given (maximum 11)")
		return

	header = [0] * 57
	outputData = bytearray()
	inputFileMap = {}
	for i, s in enumerate(codeSections):
		outOff, addr, size = loadSegment(s, inputFileMap, outputData)
		header[i], header[18 + i], header[36 + i] = outOff, addr, size
	for i, s in enumerate(dataSections):
		outOff, addr, size = loadSegment(s, inputFileMap, outputData)
		header[7 + i], header[25 + i], header[43 + i] = outOff, addr, size

	header[54] = bssAddr
	header[55] = bssSize
	header[56] = entrypoint

	with open(outFileName, "wb") as f:
		f.write(struct.pack(">57I", *header))
		f.write(outputData)

main(sys.argv)
