#!/bin/bash

clang -target powerpc-eabi -m32 -nostdlib -DTW_WII -Iinclude \
	-Wl,-Tppc/sections.ld \
	ppc/*.S ppc/*.c \
	ppc/wii/*.c ppc/wii/*.S \
	common/*.c \
	${@:1:$#-1} \
	-o ${@:$#}
