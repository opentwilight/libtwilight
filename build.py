#!/usr/bin/env python3

import sys
import subprocess

def main(args):
	subprocess.run(["clang", "-target", "powerpc-eabi", "-m32", "-nostdlib", "poke_test.c", "-o", "poke_test.elf"])

main(sys.argv)
