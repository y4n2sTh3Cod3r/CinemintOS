// VGA text buffer pointer
volatile uint16_t *vga_buffer = (uint16_t *)0xB8000;
uint16_t cursor_x, cursor_y;

// Scroll the screen up one line
void scroll_screen(volatile uint16_t *vga_buffer)
{
    for (uint16_t y = 0; y < VGA_HEIGHT - 1; y++)
    {
        for (uint16_t x = 0; x < VGA_WIDTH; x++)
        {
            vga_buffer[y * VGA_WIDTH + x] = vga_buffer[(y + 1) * VGA_WIDTH + x];
        }
    }
    for (uint16_t x = 0; x < VGA_WIDTH; x++)
    {
        vga_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = (VGA_COLOR << 8) | ' ';
    }
}

void cls()
{
    if (vga_mode == VGA_TEXT)
    {
        // Clear the screen (fill with black)
        for (uint16_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
        {
            vga_buffer[i] = (VGA_COLOR << 8) | ' ';
        }
        cursor_x = 0;
        cursor_y = 0;
    }
    else if (vga_mode == VGA_GR03)
    {
        uint8_t *vga_memory = (uint8_t *)0xA0000;
        for (int i = 0; i < 320 * 200; i++)
        {
            vga_memory[i] = 0; // Clear with black
        }
    }
}

void set_palette()
{
    // Set palette index to 0
    outb(0x3C8, 0);

    // Set up 16 basic colors (we care most about 15 being white)
    for (int i = 0; i < 16; i++)
    {
        // For color 15, set to white (63,63,63)
        if (i == 15)
        {
            outb(0x3C9, 63); // Red component
            outb(0x3C9, 63); // Green component
            outb(0x3C9, 63); // Blue component
        }
        else if (i == 1)
        {
            outb(0x3C9, 0);  // Red component
            outb(0x3C9, 0);  // Green component
            outb(0x3C9, 63); // Blue component (blue)
        }
        else
        {
            // Other colors just set to something reasonable
            outb(0x3C9, (i & 4) ? 42 : 0); // Red component
            outb(0x3C9, (i & 2) ? 42 : 0); // Green component
            outb(0x3C9, (i & 1) ? 42 : 0); // Blue component
        }
    }
}

// Set VGA mode 13h (320x200x256)
void set_vga_mode(uint8_t mode)
{
    vga_mode = mode;

    if (mode == 0x13)
    {
        // Mode 13h setup - 320x200x256
        outb(0x3C2, 0x63);
        outb(0x3C4, 0x01);
        outb(0x3C5, 0x01);
        outb(0x3C4, 0x04);
        outb(0x3C5, 0x06);
        outb(0x3D4, 0x11);
        outb(0x3D5, 0x0E);
        outb(0x3D4, 0x00);
        outb(0x3D5, 0x5F);
        outb(0x3D4, 0x01);
        outb(0x3D5, 0x4F);
        outb(0x3D4, 0x02);
        outb(0x3D5, 0x50);
        outb(0x3D4, 0x03);
        outb(0x3D5, 0x82);
        outb(0x3D4, 0x04);
        outb(0x3D5, 0x54);
        outb(0x3D4, 0x05);
        outb(0x3D5, 0x80);
        outb(0x3D4, 0x06);
        outb(0x3D5, 0xBF);
        outb(0x3D4, 0x07);
        outb(0x3D5, 0x1F);
        outb(0x3D4, 0x08);
        outb(0x3D5, 0x00);
        outb(0x3D4, 0x09);
        outb(0x3D5, 0x41);
        outb(0x3D4, 0x10);
        outb(0x3D5, 0x9C);
        outb(0x3D4, 0x11);
        outb(0x3D5, 0x8E);
        outb(0x3D4, 0x12);
        outb(0x3D5, 0x8F);
        outb(0x3D4, 0x13);
        outb(0x3D5, 0x28);
        outb(0x3D4, 0x14);
        outb(0x3D5, 0x40);
        outb(0x3D4, 0x15);
        outb(0x3D5, 0x96);
        outb(0x3D4, 0x16);
        outb(0x3D5, 0xB9);
        outb(0x3D4, 0x17);
        outb(0x3D5, 0xA3);

        // Set up GC
        outb(0x3CE, 0x00);
        outb(0x3CF, 0x00);
        outb(0x3CE, 0x01);
        outb(0x3CF, 0x00);
        outb(0x3CE, 0x02);
        outb(0x3CF, 0x00);
        outb(0x3CE, 0x03);
        outb(0x3CF, 0x00);
        outb(0x3CE, 0x04);
        outb(0x3CF, 0x00);
        outb(0x3CE, 0x05);
        outb(0x3CF, 0x40);
        outb(0x3CE, 0x06);
        outb(0x3CF, 0x05);
        outb(0x3CE, 0x07);
        outb(0x3CF, 0x0F);
        outb(0x3CE, 0x08);
        outb(0x3CF, 0xFF);

        // Set up AC
        for (uint8_t i = 0; i <= 0x15; i++)
        {
            inb(0x3DA);
            outb(0x3C0, i);
            outb(0x3C0, i);
        }
        inb(0x3DA);
        outb(0x3C0, 0x20);
    }
}

void plot_pixel(uint16_t x, uint16_t y, uint8_t color_index) {
    volatile uint8_t *vga_memory = (uint8_t *)0xA0000;
    // Ensure we're within bounds
    if (x < 320 && y < 200) {
        unsigned offset = y * 320 + x;
        vga_memory[offset] = color_index;
    }
}