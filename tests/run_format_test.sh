#!/usr/bin/bash

clang -Wno-int-to-pointer-cast -Wno-pointer-to-int-cast -Wno-void-pointer-to-int-cast -Wno-int-to-void-pointer-cast \
	format_test.c testing_utils.c ../common/*.c -o format_test
./format_test
