/* wav.c - WAV file handling functions */
#include "wav.h"
#include "ac97.h"
#include "stddef.h"

/* Simple string comparison function */
static bool str_equals(const uint8_t *str1, const uint8_t *str2, uint32_t len) {
    for (uint32_t i = 0; i < len; i++) {
        if (str1[i] != str2[i]) {
            return false;
        }
    }
    return true;
}

/* Initialize WAV playback */
bool wav_init(void) {
    /* Try to initialize hardware via AC97 driver */
    bool result = ac97_init();
    
    /* Even if initialization fails, we'll try to proceed with playback
       as the kernel.c will set up fallback values */
    return result;
}

/* Play a WAV file from a buffer */
bool wav_play(const uint8_t *wav_data, uint32_t size) {
    /* Check if the buffer is large enough to hold a WAV header */
    if (size < sizeof(wav_header_t)) {
        return false;
    }
    
    const wav_header_t *header = (const wav_header_t *)wav_data;
    
    /* Validate the WAV header */
    if (!str_equals(header->riff_header, (const uint8_t *)"RIFF", 4) ||
        !str_equals(header->wave_header, (const uint8_t *)"WAVE", 4) ||
        !str_equals(header->fmt_header, (const uint8_t *)"fmt ", 4) ||
        !str_equals(header->data_header, (const uint8_t *)"data", 4)) {
        return false; /* Invalid WAV format */
    }
    
    /* Check if this is PCM format */
    if (header->audio_format != 1) {
        return false; /* Only PCM format is supported */
    }
    
    /* For simplicity, we only support 16-bit stereo at 44.1kHz or 48kHz */
    if (header->bits_per_sample != 16 || 
        header->num_channels != 2 || 
        (header->sample_rate != 44100 && header->sample_rate != 48000)) {
        return false;
    }
    
    /* Calculate the start of the audio data and its size */
    const uint8_t *audio_data = wav_data + sizeof(wav_header_t);
    uint32_t audio_size = header->data_chunk_size;
    
    /* Make sure the audio size doesn't exceed the buffer */
    if (sizeof(wav_header_t) + audio_size > size) {
        audio_size = size - sizeof(wav_header_t);
    }
    
    /* Play the audio data */
    return ac97_play_buffer(audio_data, audio_size);
}