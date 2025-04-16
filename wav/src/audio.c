/* audio.c - Audio driver implementation for PC speaker */
#include "audio.h"

/* PC Speaker I/O ports */
#define PIT_CHANNEL2 0x42
#define PIT_COMMAND 0x43
#define SPEAKER_PORT 0x61

/* Programmable Interval Timer (PIT) frequency */
#define PIT_FREQUENCY 1193180

/* Delay function using CPU cycles */
static void delay(unsigned int cycles) {
    for (volatile unsigned int i = 0; i < cycles; i++) {
        /* A simple loop to waste CPU cycles */
        __asm__ volatile("nop");
    }
}

/* Read from an I/O port */
static inline unsigned char inb(unsigned short port) {
    unsigned char value;
    __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

/* Write to an I/O port */
static inline void outb(unsigned short port, unsigned char value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

/* Set the PC speaker frequency */
static void set_speaker_frequency(unsigned int frequency) {
    unsigned int divisor = PIT_FREQUENCY / frequency;
    
    /* Set PIT Channel 2 to mode 3 (square wave generator) */
    outb(PIT_COMMAND, 0xB6);
    
    /* Set the frequency divisor */
    outb(PIT_CHANNEL2, divisor & 0xFF);         /* Low byte */
    outb(PIT_CHANNEL2, (divisor >> 8) & 0xFF);  /* High byte */
}

/* Turn the PC speaker on */
static void speaker_on() {
    unsigned char tmp = inb(SPEAKER_PORT);
    outb(SPEAKER_PORT, tmp | 0x3);  /* Set bits 0 and 1 to enable the speaker */
}

/* Turn the PC speaker off */
static void speaker_off() {
    unsigned char tmp = inb(SPEAKER_PORT);
    outb(SPEAKER_PORT, tmp & 0xFC);  /* Clear bits 0 and 1 to disable the speaker */
}

/* Initialize the audio hardware */
int audio_init() {
    /* No specific initialization needed for PC speaker */
    speaker_off();  /* Make sure speaker is off initially */
    return 1;  /* Success */
}

/* Play a chunk of audio data */
void audio_play_chunk(const unsigned char* data, unsigned int size) {
    /* Very simple PCM to frequency conversion
     * This is a naive implementation that converts 8-bit PCM samples to
     * frequencies. A real implementation would need proper audio drivers.
     */
    for (unsigned int i = 0; i < size; i++) {
        /* Convert 8-bit PCM value (0-255) to a frequency between 100Hz and 8000Hz */
        unsigned int frequency = 100 + (data[i] * 31);  /* Scale to audible range */
        
        /* Set frequency and turn on the speaker */
        set_speaker_frequency(frequency);
        speaker_on();
        
        /* Play for a short duration */
        delay(100);
        
        /* Turn off the speaker between samples to create separation */
        speaker_off();
        delay(10);
    }
}

/* Cleanup audio resources */
void audio_cleanup() {
    speaker_off();  /* Ensure speaker is turned off */
}