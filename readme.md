# libtwilight

This is an open source, clean slate implementation of a micro kernel for Wii/GC homebrew.

The goal of this library is to provide a suitable foundation for writing high-perfomance games and apps that take full advantage of the hardware.

It is not trying to be a full-on POSIX-compatible kernel.

Currently in early stages. Text output to XFB using a monospace bitmap font is working, and is successfully displayed to the screen.

## Setup

Install LLVM and Clang.

## Compile

`clang -target powerpc-eabi -m32 -nostdlib -Ippc -Wl,-Tppc/sections.ld ppc/*.S ppc/*.c common/*.c examples/hello_world.c -o examples/hello_world.elf`

Call with `-DTW_WII` to compile for Wii instead

## Immediate TODO

- Finish printf() building blocks
	- String formatter
	- XFB terminal printing (including ANSI escape codes)
- Optionally blend XFB text with background
- Finish heap allocator
	- List of disjoint pools, eg. one for MEM1 and MEM2
- Serial (for Gamecube input)
- Friendly DSI and ISI exception crash handler (segfault)
	- With future plans to implement lazy loading within these handlers

## Secondary TODO

- Custom libc implementation that wraps libtwilight
	- libtwilight **must not** depend on libc, libc must depend on libtwilight
	- limited POSIX
- Custom threading implementation
	- Use decrement register for scheduling software interrupts
	- Synchronisation primitives (atomics, semaphores, etc)
- Optional serial interrupts, for input events (ie. notfiy instead of just poll)
- Maths
- GC hardware
	- EXI
	- DVD
	- Audio
	- DSP
		- Custom microcode
		- Manage ARAM
	- GPU
		- GX
- Wii hardware
	- IOS
	- STM (system control over IOS)
	- Bluetooth
		- Wiimote
		- Nunchuk
		- Classic controller
		- Balance board
	- USB
		- Hubs
		- Storage
		- HID architecture
			- API for implementing custom HID device (eg. keyboard, mouse)
	- SD
	- NAND
	- Wifi
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

## Might implement later

- Page faults in ISI/DSI exception handler, for lazy loading
- Memory mapping (MMU)
	- TLB
	- BAT
	- Interface for using the MMU effectively (probably not POSIX)
- High Speed Port (GameCube only)
	- Game Boy Player
	- Broadband
- More controllers
	- Guitar Hero / Rock Band
	- Wavebird
	- Serial keyboard
	- Wii motion plus
- Write Starlet (ARM) programs with a shared common codebase
	- Examples:
		- Modern, concurrent TCP/IP Stack (eg. HTTP2, HTTP3)
		- iSCSI
		- Drivers for previously unsupported hardware (over USB, Bluetooth, etc)
			- Controllers
			- Speakers
			- Headphones
			- Microphones
			- Arbitrary gadgets
	- Mechanism for deploying them upon launching the app
	- Provide an app for managing these programs (move, delete, etc.)
