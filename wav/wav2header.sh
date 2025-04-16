#!/bin/bash
# wav2header.sh - Convert a WAV file to a C header

# Exit on error
set -e

# Check if filename provided
if [ $# -lt 1 ]; then
    echo "Usage: $0 input.wav [output.h]"
    exit 1
fi

INPUT_FILE="$1"
OUTPUT_FILE="${2:-sound_data.h}"

# Check if input file exists
if [ ! -f "$INPUT_FILE" ]; then
    echo "Error: Input file '$INPUT_FILE' not found."
    exit 1
fi

# Get file size
FILE_SIZE=$(stat -c%s "$INPUT_FILE")
echo "Processing WAV file: $INPUT_FILE ($FILE_SIZE bytes)"

# Create header file
cat > "$OUTPUT_FILE" << EOF
/* $OUTPUT_FILE - Generated WAV file data */
#ifndef SOUND_DATA_H
#define SOUND_DATA_H

#include <stdint.h>

/* WAV file size */
#define WAV_SIZE $FILE_SIZE

/* WAV file data */
const uint8_t wav_data[WAV_SIZE] = {
EOF

# Convert binary file to C array
hexdump -v -e '16/1 "0x%02X, " "\n"' "$INPUT_FILE" | sed 's/, $//' >> "$OUTPUT_FILE"

# Close the header file
cat >> "$OUTPUT_FILE" << EOF
};

#endif /* SOUND_DATA_H */
EOF

echo "Created header file: $OUTPUT_FILE"