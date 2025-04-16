/* kernel.c - Main kernel code */
#include "multiboot.h"
#include "wav.h"
#include "sound_data.h"
#include "io.h"
#include "stdint.h"
#include "stdbool.h"

/* Simple delay function */
static void delay(uint32_t count) {
    for (volatile uint32_t i = 0; i < count * 10000; i++) {
        __asm__ volatile("nop");
    }
}

/* Simple VGA text mode functions for basic status display */
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEMORY (uint16_t*)0xB8000

static uint16_t *vga_buffer = VGA_MEMORY;
static uint8_t vga_x = 0;
static uint8_t vga_y = 0;

/* Convert a character and color to a VGA entry */
static uint16_t vga_entry(char c, uint8_t color) {
    return (uint16_t)c | ((uint16_t)color << 8);
}

/* Clear the screen */
static void clear_screen(void) {
    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            vga_buffer[y * VGA_WIDTH + x] = vga_entry(' ', 0x07);
        }
    }
    vga_x = 0;
    vga_y = 0;
}

/* Put a character on the screen */
static void putchar(char c, uint8_t color) {
    if (c == '\n') {
        vga_x = 0;
        vga_y++;
        if (vga_y >= VGA_HEIGHT) {
            vga_y = 0;
        }
        return;
    }
    
    vga_buffer[vga_y * VGA_WIDTH + vga_x] = vga_entry(c, color);
    vga_x++;
    
    if (vga_x >= VGA_WIDTH) {
        vga_x = 0;
        vga_y++;
        if (vga_y >= VGA_HEIGHT) {
            vga_y = 0;
        }
    }
}

/* Print a string */
static void print(const char *str, uint8_t color) {
    for (int i = 0; str[i] != '\0'; i++) {
        putchar(str[i], color);
    }
}

/* Main kernel entry point */
void kernel_main(multiboot_info_t *mboot_info) {
    /* Initialize basic screen */
    clear_screen();
    print("TinyWAV Kernel - Starting...\n", 0x0F);
    
    /* Initialize WAV playback (which initializes the audio hardware) */
    print("Initializing audio hardware... ", 0x07);
    if (!wav_init()) {
        print("FAILED!\n", 0x0C);
        print("Could not initialize audio hardware.\n", 0x0C);
        print("Proceeding anyway with default settings...\n", 0x0E);
        
        /* Try fallback mode with hardcoded values */
        extern uint16_t nam_base;
        extern uint16_t nabm_base;
        
        /* Use common default values for VMware/VirtualBox */
        nam_base = 0x5000;   /* Common NAM BAR address */
        nabm_base = 0x5080;  /* Common NABM BAR address */
    } else {
        print("OK\n", 0x0A);
    }
    
    /* Play the WAV file */
    print("Playing WAV file... ", 0x07);
    if (!wav_play(wav_data, WAV_SIZE)) {
        print("FAILED!\n", 0x0C);
        print("The WAV file could not be played.\n", 0x0C);
    } else {
        print("OK\n", 0x0A);
        
        /* Calculate approximate play time in seconds (very rough estimate) */
        uint32_t play_time = (WAV_SIZE - 44) / 176400; /* Assuming 44.1kHz, 16-bit, stereo */
        
        /* Print a simple status message */
        print("Now playing audio. Will play for approximately ", 0x0F);
        
        /* Convert play time to a string (simple implementation) */
        char time_str[16] = {0};
        int pos = 0;
        int tmp = play_time;
        
        /* Handle the simple case of 0 */
        if (tmp == 0) {
            time_str[pos++] = '0';
        }
        
        /* Extract digits */
        while (tmp > 0) {
            time_str[pos++] = '0' + (tmp % 10);
            tmp /= 10;
        }
        
        /* Reverse the string */
        for (int i = 0; i < pos / 2; i++) {
            char t = time_str[i];
            time_str[i] = time_str[pos - i - 1];
            time_str[pos - i - 1] = t;
        }
        
        print(time_str, 0x0E);
        print(" seconds.\n", 0x0F);
    }
    
    /* Keep the kernel running (to continue playing audio) */
    print("System running. Audio playback in progress...\n", 0x0A);
    for (;;) {
        __asm__ volatile("hlt");
    }
}