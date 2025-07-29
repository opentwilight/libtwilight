#!/usr/bin/env python3

# Modified version of a python implementation of WiiLoad https://pastebin.com/4nWAkBpw
# Originally written by dhewg

import os
import sys
import zlib
import socket
import struct

WIILOAD_VERSION_MAJOR=0
WIILOAD_VERSION_MINOR=5

def print_help():
	print("wiiload-py3 --file <filename> --ip <ip>")
	print("Options:")
	print("  --file <filename>")
	print("  --ip <ip address of wii>")
	print("  --port <port on wii, defaults to 4299>")
	print("  --params <comma separated parameters>")
	print("  --arg <adds an argument>")

def get_args(args):
	filename = ""
	ip = ""
	port = 4299
	params = []
	added_params = []

	idx = -1
	while idx < len(args) - 1:
		idx += 1
		if args[idx] == "--file":
			filename = args[idx+1]
		elif args[idx] == "--params":
			params = args[idx+1].split(",")
		elif args[idx] == "--arg":
			added_params.append(args[idx+1])
		elif args[idx] == "--ip":
			ip = args[idx+1]
		elif args[idx] == "--port":
			port = int(args[idx+1])

	if not filename or not ip:
		raise Exception("")

	params_list = [os.path.basename(filename)] + params + added_params
	return filename, ip, port, params_list

def main(args):
	try:
		filename, ip, port, params_list = get_destination(args)
	except:
		print_help()
		return

	with open(filename, "rb") as f:
		u_data = f.read()

	len_uncompressed = os.path.getsize(filename)
	c_data = zlib.compress(u_data, zlib.Z_DEFAULT_COMPRESSION)

	chunk_size = 1024*128
	full_chunks = len(c_data) // chunk_size
	leftover = len(c_data) % chunk_size
	total_chunks = full_chunks + (1 if leftover > 0 else 0)

	params = bytes("\x00".join(params_list) + "\x00", "utf8")

	with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
		s.connect((ip, port))

		s.send("HAXX")
		s.send(struct.pack("B",  WIILOAD_VERSION_MAJOR)) # one byte, unsigned
		s.send(struct.pack("B",  WIILOAD_VERSION_MINOR)) # one byte, unsigned
		s.send(struct.pack(">H", len(args))) # bigendian, 2 bytes, unsigned
		s.send(struct.pack(">L", len(c_data))) # bigendian, 4 bytes, unsigned
		s.send(struct.pack(">L", len_uncompressed)) # bigendian, 4 bytes, unsigned

		print(len(chunks), "chunks to send")
		
		for c in range(full_chunks):
			off = c * chunk_size
			s.send(c_data[off : off+chunk_size])
			sys.stdout.write(".")
			sys.stdout.flush()

		if leftover > 0:
			s.send(c_data[full_chunks*chunk_size : ])
			sys.stdout.write(".")
			sys.stdout.flush()

		sys.stdout.write("\n")
		s.send(params)

	print("done")

if len(sys.argv) < 2:
	print_help()
else:
	main(sys.argv)
