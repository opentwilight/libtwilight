#!/usr/bin/env python3

import os
import sys
import shutil
import subprocess

GIT = "git"
HOST_GCC = "gcc"

def ensure_path_binary(program_name):
	if shutil.which(program_name) is None:
		print("Could not find '" + program_name + "'.")
		print("Make sure you have installed it from your package manager (else, search with google)")
		print("and that '" + program_name + "' is on your PATH variable.")
		sys.exit(1)

def main(args):
	ensure_path_binary(GIT)
	ensure_path_binary(HOST_GCC)

	status = subprocess.run(["git", "clone", "-b", "releases/gcc-15", "--single-branch", "https://github.com/gcc-mirror/gcc.git"])
	if status.returncode != 0:
		return

main(sys.argv)
