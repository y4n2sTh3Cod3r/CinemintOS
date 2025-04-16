#!/bin/bash
# build.sh - Build script for the TinyWAV kernel

# Exit on error
set -e

# Configuration
KERNEL_NAME="tinywav"
CC="gcc"
AS="nasm"
CFLAGS="-m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector -nostartfiles -nodefaultlibs -Wall -Wextra -c -O2 -I."
LDFLAGS="-m32 -nostdlib -nostdinc -T linker.ld -o"
ASFLAGS="-f elf32"
WAV_FILE="test_sound.wav"
WAV_HEADER="src/sound_data.h"

# Check for required tools
for tool in gcc nasm grub-mkrescue xorriso; do
    if ! command -v $tool &> /dev/null; then
        echo "Error: Required tool '$tool' is not installed."
        echo "Please install with: sudo apt-get install build-essential nasm grub-common xorriso"
        exit 1
    fi
done

# Create a test WAV file if it doesn't exist and no sound_data.h is present
if [ ! -f "$WAV_FILE" ] && [ ! -f "$WAV_HEADER" ]; then
    echo "No WAV file found. Creating a test tone..."
    if command -v ffmpeg &> /dev/null; then
        # Create a test tone using ffmpeg if available
        ffmpeg -f lavfi -i "sine=frequency=440:duration=5" -ar 44100 -ac 2 -acodec pcm_s16le "$WAV_FILE"
    else
        echo "FFmpeg not found. You need to provide a WAV file."
        echo "You can install FFmpeg with: sudo apt-get install ffmpeg"
        echo "Or convert your own WAV file to a header using: ./wav2header.sh your-file.wav src/sound_data.h"
        exit 1
    fi
    
    # Convert WAV to C header
    echo "Converting WAV file to C header..."
    chmod +x wav2header.sh
    ./wav2header.sh "$WAV_FILE" "$WAV_HEADER"
fi

# Check if sound_data.h exists
if [ ! -f "$WAV_HEADER" ]; then
    echo "Error: $WAV_HEADER not found."
    echo "Please convert your WAV file first: ./wav2header.sh your-file.wav $WAV_HEADER"
    exit 1
fi

# Create directories if they don't exist
mkdir -p build
mkdir -p iso/boot/grub

# Compile assembly source
echo "Compiling assembly code..."
$AS $ASFLAGS src/boot.asm -o build/boot.o

# Compile C sources
echo "Compiling C code..."
$CC $CFLAGS src/kernel.c -o build/kernel.o
$CC $CFLAGS src/ac97.c -o build/ac97.o
$CC $CFLAGS src/wav.c -o build/wav.o

# Link objects
echo "Linking kernel..."
$CC build/boot.o build/kernel.o build/ac97.o build/wav.o $LDFLAGS build/$KERNEL_NAME.bin

# Create ISO image
echo "Creating ISO image..."
cp build/$KERNEL_NAME.bin iso/boot/
cat > iso/boot/grub/grub.cfg << EOF
menuentry "TinyWAV Kernel" {
    multiboot /boot/tinywav.bin
}
EOF

# Create the ISO image using grub-mkrescue
grub-mkrescue -o $KERNEL_NAME.iso iso 2>/dev/null

echo ""
echo "✅ Build completed successfully!"
echo ""
echo "ISO image created: $KERNEL_NAME.iso"
echo ""
echo "To run in QEMU:"
echo "  qemu-system-i386 -soundhw ac97 -cdrom $KERNEL_NAME.iso"
echo ""
echo "To run in VirtualBox:"
echo "  1. Create a new VM (Other/Unknown, 8MB RAM, no hard disk)"
echo "  2. In Settings -> Audio, make sure audio is enabled"
echo "  3. In Settings -> Storage, attach $KERNEL_NAME.iso to the CD drive"
echo "  4. Start the VM"
echo ""