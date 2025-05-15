# libtwilight

This is an open source, clean slate implementation of a micro kernel for Wii/GC homebrew.

The goal of this library is to provide a suitable foundation for writing high-perfomance games and apps that take full advantage of the hardware.

It is not trying to be a full-on POSIX-compatible kernel. That said, it would be pretty cool if it were used as the foundation of such a kernel.

Currently in early stages. Text output to XFB using a monospace bitmap font is working, and is successfully displayed to the screen.

## Immediate TODO

- Finish printf() building blocks
	- String formatter
	- XFB terminal printing (including ANSI escape codes)
- Optionally blend XFB text with background
- Finish heap allocator
- Serial and EXI (for Gamecube input)
- Friendly DSI and ISI exception crash handler (segfault)
	- With future plans to implement lazy loading within these handlers

## Secondary TODO

- Custom libc implementation that wraps libtwilight
	- libtwilight **must not** depend on libc, libc must depend on libtwilight
	- limited POSIX
- Custom threading implementation
	- Use decrement register for scheduling software interrupts
	- Synchronisation primitives (atomics, semaphores, etc)
- Clever memory functionality, using the MMU
	- TLB
	- BAT
	- Interface for using the MMU effectively (probably not POSIX)
- TCP/IP Stack
- Maths
- GC hardware
	- DSP
		- Custom microcode
		- Manage ARAM
	- GPU
		- GX
- Wii hardware
	- IOS
	- Bluetooth
	- USB
	- SD
	- NAND
	- Ethernet
	- Wifi
	- Starlet
	- Input
		- Wiimotes
		- Nunchuks
		- Wii motion plus
		- Balance board
		- Maybe Guitar Hero?
		- USB keyboard
		- USB controller
- Storage architecture
	- Hardware layer
		- See "Wii hardware"
		- Select which USB device, ie. dive through hubs
	- Disk layer
		- Select partition, or just use the first partition
	- Filesystem layer
		- FAT32
		- API for 3rd party filesystem driver implementations
	- POSIX file wrapper
		- open, read, write, flush, close, etc.
		- file table
- Write Starlet (ARM) programs with a shared common codebase
	- Mechanism for deploying them upon launching the app
	- Provide an app for managing these programs (move, delete, etc.)
