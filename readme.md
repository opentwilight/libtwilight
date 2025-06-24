# libtwilight

This is an open source, clean slate implementation of a micro kernel for Wii/GC homebrew.

The goal of this library is to provide a suitable foundation for writing high-perfomance games and apps that take full advantage of the hardware.

It is not trying to be a full-on POSIX-compatible kernel.

Currently in early stages. Gamecube controller input and text output to XFB using a monospace bitmap font is working, and is displayed to the screen.

## Setup

Install LLVM and Clang.

## Compile

### GameCube

`clang -g -target powerpc-eabi -m32 -nostdlib -Iinclude -Wl,-Tppc/sections.ld ppc/*.S ppc/*.c common/*.c examples/hello_world.c -o examples/hello_world.elf`

### Wii

`clang -g -target powerpc-eabi -m32 -nostdlib -DTW_WII -Iinclude -Wl,-Tppc/sections.ld ppc/*.S ppc/*.c ppc/wii/*.c common/*.c examples/hello_world.c -o examples/hello_world.elf`

## TODO

- Storage architecture
	- Hardware layer
		- See "Wii hardware"
		- Select which USB device, ie. dive through hubs
	- Disk layer
		- Select partition, or just use the first partition
		- MBR
		- Wii/GC disk partition layout format
	- Filesystem layer
		- FAT32
		- Wii/GC disk FST
		- API for 3rd party filesystem driver implementations
	- POSIX file wrapper
		- open, read, write, flush, close, etc.
		- file table
		- mount table (accessible through string path)
- Custom threading implementation
	- Use decrement register for scheduling software interrupts
	- Timeouts in `TW_AwaitCondition`
	- Maybe schedule other threads in `TW_LockMutex` and `TW_AwaitCondition`
	- Check for deadlock if waiting in `TW_LockMutex` and `TW_AwaitCondition`
- Build Tools
	- ELF to DOL
	- FST generator
	- GameCube/Wii disc image builder
- Start work on custom libc implementation that wraps libtwilight
	- libtwilight **must not** depend on libc, libc must depend on libtwilight
	- limited POSIX
	- Basic maths functions
- Serial Interface
	- Buffer transfers
	- Different kinds of controllers
	- Interrupts, for input events (ie. notfiy instead of just poll)
- GX
	- Display lists
	- Textures
	- Pipelines
- GC hardware
	- EXI
	- DVD
	- Audio
	- DSP
		- Custom microcode
		- Manage ARAM
- Wii hardware
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
	- DVD
	- Wifi

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
- Implement the interface for every IOS module on PPC (eg. ES, STM, etc)...
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
