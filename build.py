#!/usr/bin/env python3

import sys
import subprocess

# clang -target powerpc-eabi -m32 -nostdlib -Ippc -DTW_WII=1 -Wl,-Tppc/sections.ld ppc/*.S ppc/*.c examples/hello_world.c -o examples/hello_world.elf

def main(args):
	subprocess.run([
		"clang",
		"-target",
		"powerpc-eabi",
		"-m32",
		"-nostdlib",
		"-DTW_WII=1",
		"examples/poke_test.c",
		"-o",
		"examples/poke_test.elf"
	])

main(sys.argv)
