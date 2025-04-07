// kernel.cpp - A minimal OS kernel that displays keyboard input
#include <stdint.h>

// Constants for VGA text mode
const uint16_t VGA_WIDTH = 80;
const uint16_t VGA_HEIGHT = 25;
const uint16_t VGA_COLOR = 0x0F; // White text on black background

// Function prototypes
uint8_t inb(uint16_t port);
char scancode_to_ascii(uint8_t scancode);
void scroll_screen(volatile uint16_t *vga_buffer);

// Kernel entry point
extern "C" void kernel_main()
{
    // VGA text buffer pointer
    volatile uint16_t *vga_buffer = (uint16_t *)0xB8000;

    // Clear the screen (fill with black)
    for (uint16_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
    {
        vga_buffer[i] = (VGA_COLOR << 8) | ' ';
    }

    // Initialize cursor position
    uint16_t cursor_x = 0;
    uint16_t cursor_y = 0;

    // Main loop to handle keyboard input
    while (true)
    {
        // Check if there's keyboard input available
        if (inb(0x64) & 0x1)
        {
            // Read the scancode
            uint8_t scancode = inb(0x60);

            // Simple scancode to ASCII mapping for common keys
            char c = scancode_to_ascii(scancode);

            // Only process if it's a valid character
            if (c)
            {
                // Calculate position in the VGA buffer
                uint16_t position = cursor_y * VGA_WIDTH + cursor_x;

                // Write the character to the buffer
                vga_buffer[position] = (VGA_COLOR << 8) | c;

                // Update cursor position
                cursor_x++;
                if (cursor_x >= VGA_WIDTH)
                {
                    cursor_x = 0;
                    cursor_y++;
                    if (cursor_y >= VGA_HEIGHT)
                    {
                        // Scroll the screen if needed
                        scroll_screen(vga_buffer);
                        cursor_y = VGA_HEIGHT - 1;
                    }
                }
            }
        }
    }
}

// Function to read a byte from an I/O port
uint8_t inb(uint16_t port)
{
    uint8_t value;
    asm volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

// Simple scancode to ASCII mapping for common keys
char scancode_to_ascii(uint8_t scancode)
{
    // This is a very basic mapping for the most common keys
    // Only handling the "make" codes (key presses), not "break" codes (key releases)
    static const char scancode_ascii[] = {
        0,
        0,
        '1',
        '2',
        '3',
        '4',
        '5',
        '6',
        '7',
        '8',
        '9',
        '0',
        '-',
        '=',
        0,
        0,
        'q',
        'w',
        'e',
        'r',
        't',
        'y',
        'u',
        'i',
        'o',
        'p',
        '[',
        ']',
        '\n',
        0,
        'a',
        's',
        'd',
        'f',
        'g',
        'h',
        'j',
        'k',
        'l',
        ';',
        '\'',
        '`',
        0,
        '\\',
        'z',
        'x',
        'c',
        'v',
        'b',
        'n',
        'm',
        ',',
        '.',
        '/',
        0,
        '*',
        0,
        ' ',
        0,
        0,
        0,
        0,
        0,
        0,
    };

    // Only process make codes (key presses) for the first 58 keys
    if (scancode < 58)
    {
        return scancode_ascii[scancode];
    }

    return 0;
}

// Scroll the screen up one line
void scroll_screen(volatile uint16_t *vga_buffer)
{
    // Move each line up one position
    for (uint16_t y = 0; y < VGA_HEIGHT - 1; y++)
    {
        for (uint16_t x = 0; x < VGA_WIDTH; x++)
        {
            vga_buffer[y * VGA_WIDTH + x] = vga_buffer[(y + 1) * VGA_WIDTH + x];
        }
    }

    // Clear the last line
    for (uint16_t x = 0; x < VGA_WIDTH; x++)
    {
        vga_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = (VGA_COLOR << 8) | ' ';
    }
}
