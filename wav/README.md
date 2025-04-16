# TinyWAV Kernel

A minimalist kernel that solely plays a WAV audio file through the AC97 audio interface. This project demonstrates basic OS kernel development with Multiboot support, focusing on audio playback.

## Prerequisites

To build and run this project, you'll need:

- Ubuntu Linux (or another similar distribution)
- GCC (for cross-compilation)
- NASM (Netwide Assembler)
- GRUB2 utilities (including grub-mkrescue)
- xorriso (required by grub-mkrescue)
- QEMU (for testing) or VirtualBox

Install them with:

```bash
sudo apt-get update
sudo apt-get install build-essential nasm grub-common xorriso qemu-system-x86
```

## Building the Kernel

1. First, prepare your WAV file. The kernel works best with 16-bit stereo WAV files at 44.1kHz or 48kHz.

2. Convert your WAV file to a C header:

```bash
chmod +x wav2header.sh
./wav2header.sh your-file.wav src/sound_data.h
```

3. Build the kernel and create an ISO:

```bash
chmod +x build.sh
./build.sh
```

## Running the Kernel

### Using QEMU

```bash
qemu-system-i386 -soundhw ac97 -cdrom tinywav.iso
```

### Using VirtualBox

1. Create a new virtual machine
2. Choose "Other/Unknown" as the OS type
3. Allocate a small amount of memory (8MB is enough)
4. No hard disk is needed
5. In the Storage settings, attach the `tinywav.iso` file to the CD/DVD drive
6. In Audio settings, make sure audio is enabled and AC97 is selected
7. Start the VM

## Project Structure

- `src/`: Source code
  - `boot.asm`: Assembly entry point with Multiboot header
  - `kernel.c`: Main kernel C code
  - `ac97.c/h`: AC97 audio driver
  - `wav.c/h`: WAV file handling
  - `sound_data.h`: Generated C header with WAV data
- `include/`: Header files
- `linker.ld`: Linker script
- `build.sh`: Build script
- `wav2header.sh`: Script to convert WAV to C header

## Implementation Notes

This kernel is designed to be a minimal, self-contained system without reliance on the standard C library. It includes:

- Custom implementations of standard C headers (`stdint.h`, `stdbool.h`, `stddef.h`)
- Direct hardware access via I/O ports
- PCI detection for sound card hardware
- DMA buffer setup for audio playback

Since we're developing a freestanding kernel, we don't have access to the standard C library or system calls. This means we need to implement or define everything ourselves, including basic types and functions.

## VMware & VirtualBox Compatibility

For VMware Player:
1. Create a new VM (Other/Linux, 32-bit)
2. Allocate at least 32MB of RAM
3. Add a sound card to the VM configuration
4. Attach the ISO as a CD/DVD drive
5. Start the VM

If the kernel gets stuck at "Initializing audio hardware...", it means the auto-detection failed, but it should continue using fallback settings. If no sound plays, try these troubleshooting tips:

1. Make sure your host system's volume is turned up
2. In VMware, ensure the "Connected" checkbox is enabled for the sound device
3. Try using QEMU as an alternative with the specific AC97 device: `qemu-system-i386 -soundhw ac97 -cdrom tinywav.iso`