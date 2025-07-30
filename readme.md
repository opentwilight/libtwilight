# libtwilight

This is an open source, clean slate implementation of a micro kernel for Wii/GC homebrew with no dependencies.

This library aims provide a suitable foundation for writing high-perfomance games and apps that take full advantage of the hardware.

It is not trying to be a full-on POSIX-compatible kernel.

Currently in early stages. Gamecube controller input and text output to XFB using a monospace bitmap font is working, and is displayed to the screen.

Bugs abound! The `hello_world` example currently does not work on a real Nintendo Wii. For now, the only way to run the examples is inside Dolphin Emulator.

## Goals

- Full hardware access, including the GPU
- Machinery required to implement a basic C/C++/Rust standard library
- Usable
	- Relevant build tools
	- Easy to integrate into a new or existing codebase

## Contributing

We welcome contributions! However, there are some constraints on what we can allow in the core library, excluding tools and tests.

- Any code contribution must be your own. We have a strict "Not Invented Here" policy.
- Any code contribution must be without dependencies, including libc. The idea is that things will depend on us, not the other way around :)
- New features that aren't (yet) relevant to the goals of this project will be more heavily scrutinised.

These constraints do not apply to tools and tests. For those, we are language and framework agnostic.
The only requirement is that they must be able to be run on my Linux laptop without headaches :)

This project is still in its early stages, so bug fixes are extremely welcome :)

## Setup

Install LLVM and Clang.

## Compile

### GameCube

`clang -g -target powerpc-eabi -m32 -nostdlib -Iinclude -Wl,-Tppc/sections.ld ppc/*.S ppc/*.c common/*.c examples/hello_world.c -o examples/hello_world.elf`

### Wii

`clang -g -target powerpc-eabi -m32 -nostdlib -DTW_WII -Iinclude -Wl,-Tppc/sections.ld ppc/*.S ppc/*.c ppc/wii/*.S ppc/wii/*.c common/*.c examples/hello_world.c -o examples/hello_world.elf`

## TODO

- More tests!
	- There is a tests folder, which is currently only used to unit test TW_FormatString.
	- Ideally, this folder could contain integration tests in addition to unit tests.
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
- Build tools
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
