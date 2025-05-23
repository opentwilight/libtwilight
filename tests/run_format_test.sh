#!/usr/bin/bash

clang -fsanitize=address -Wno-int-to-pointer-cast -Wno-pointer-to-int-cast -Wno-void-pointer-to-int-cast -Wno-int-to-void-pointer-cast \
	-I../include format_test.c testing_utils.c ../common/strformat.c ../common/structures.c ../common/threading.c -o format_test
./format_test
