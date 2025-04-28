#!/usr/bin/env python3

import os
import sys
import shutil
import subprocess

GIT = "git"
HOST_GCC = "gcc"
HOST_CLANG = "clang"

def ensure_path_binary(program_name, alternative_name=None):
	if shutil.which(program_name) is None:
		if alternative_name is not None and shutil.which(alternative_name) is not None:
			return
		print("Could not find '" + program_name + "'.")
		print("Make sure you have installed it from your package manager (else, search with google)")
		print("and that '" + program_name + "' is on your PATH variable.")
		sys.exit(1)

def main(args):
	ensure_path_binary(GIT)
	ensure_path_binary(HOST_GCC, HOST_CLANG)

	if not os.path.isdir("gcc"):
		status = subprocess.run(["git", "clone", "-b", "releases/gcc-15", "--single-branch", "https://github.com/gcc-mirror/gcc.git"])
		if status.returncode != 0:
			return

	os.chdir("gcc")
	subprocess.run(["./configure", "--target", "powerpc-750-eabi"])

main(sys.argv)
