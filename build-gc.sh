#!/bin/bash

clang -g -target powerpc-eabi -m32 -nostdlib -Iinclude \
	-Wl,-Tppc/sections.ld \
	ppc/*.S ppc/*.c \
	common/*.c \
	${@:1:$#-1} \
	-o ${@:$#}
