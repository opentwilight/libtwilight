#!/usr/bin/env python3

import sys
import subprocess

def main(args):
	subprocess.run(["clang", "-target", "powerpc-eabi", "-m32", "-nostdlib", "examples/poke_test.c", "-o", "examples/poke_test.elf"])

main(sys.argv)
