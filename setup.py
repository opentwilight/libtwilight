#!/usr/bin/env python3

import os
import sys
import shutil
import subprocess

GIT = "git"
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
	ensure_path_binary(HOST_CLANG)

main(sys.argv)
