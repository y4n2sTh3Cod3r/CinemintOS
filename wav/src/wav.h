/* wav.h - WAV file handling */
#ifndef WAV_H
#define WAV_H

#include "stdint.h"
#include "stdbool.h"

/* WAV file header structure */
typedef struct {
    /* RIFF header */
    uint8_t riff_header[4];    /* "RIFF" */
    uint32_t file_size;        /* File size - 8 */
    uint8_t wave_header[4];    /* "WAVE" */
    
    /* Format chunk */
    uint8_t fmt_header[4];     /* "fmt " */
    uint32_t fmt_chunk_size;   /* Format chunk size (16 for PCM) */
    uint16_t audio_format;     /* Audio format (1 for PCM) */
    uint16_t num_channels;     /* Number of channels */
    uint32_t sample_rate;      /* Sample rate */
    uint32_t byte_rate;        /* Byte rate */
    uint16_t block_align;      /* Block align */
    uint16_t bits_per_sample;  /* Bits per sample */
    
    /* Data chunk */
    uint8_t data_header[4];    /* "data" */
    uint32_t data_chunk_size;  /* Data chunk size */
} __attribute__((packed)) wav_header_t;

/* Initialize WAV playback */
bool wav_init(void);

/* Play a WAV file (from a buffer) */
bool wav_play(const uint8_t *wav_data, uint32_t size);

#endif /* WAV_H */