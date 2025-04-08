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
    else if (vga_mode == VGA_GR02)
    {
        uint8_t *vga_memory = (uint8_t *)0xA0000;
        for (int i = 0; i < 640 * 480; i++)
        {
            vga_memory[i] = 0; // Clear with black
        }
    }
    else if (vga_mode == VGA_GR03 || vga_mode == VGA_GR01)
    {
        uint8_t *vga_memory = (uint8_t *)0xA0000;
        for (int i = 0; i < 320 * 200; i++)
        {
            vga_memory[i] = 0; // Clear with black
        }
    }
}

void set_vga_mode(uint8_t mode)
{
    vga_mode = mode;

    if (mode == 0x04)
    {
        // Mode 04h setup - 320x200x4 CGA compatible
        outb(0x3C2, 0x63); // Miscellaneous Output Register

        // Sequencer registers
        outb(0x3C4, 0x01); // Clocking Mode
        outb(0x3C5, 0x01);
        outb(0x3C4, 0x04); // Memory Mode
        outb(0x3C5, 0x02); // Planar mode (not chained)

        // Unlock CRTC registers
        outb(0x3D4, 0x11);
        outb(0x3D5, 0x0E & 0x7F); // Vertical Retrace End (bit 7 clear = unlock)

        // CRTC registers for 320x200
        outb(0x3D4, 0x00); // Horizontal Total
        outb(0x3D5, 0x2D);
        outb(0x3D4, 0x01); // Horizontal Display Enable End
        outb(0x3D5, 0x27);
        outb(0x3D4, 0x02); // Start Horizontal Blanking
        outb(0x3D5, 0x28);
        outb(0x3D4, 0x03); // End Horizontal Blanking
        outb(0x3D5, 0x90);
        outb(0x3D4, 0x04); // Start Horizontal Retrace
        outb(0x3D5, 0x2B);
        outb(0x3D4, 0x05); // End Horizontal Retrace
        outb(0x3D5, 0x80);
        outb(0x3D4, 0x06); // Vertical Total
        outb(0x3D5, 0xBF);
        outb(0x3D4, 0x07); // Overflow
        outb(0x3D5, 0x1F);
        outb(0x3D4, 0x08); // Preset Row Scan
        outb(0x3D5, 0x00);
        outb(0x3D4, 0x09); // Maximum Scan Line
        outb(0x3D5, 0x41);
        outb(0x3D4, 0x10); // Vertical Retrace Start
        outb(0x3D5, 0x9C);
        outb(0x3D4, 0x11); // Vertical Retrace End
        outb(0x3D5, 0x8E);
        outb(0x3D4, 0x12); // Vertical Display Enable End
        outb(0x3D5, 0x8F);
        outb(0x3D4, 0x13); // Offset
        outb(0x3D5, 0x14); // 40 bytes per line (320 pixels / 8 pixels per byte)
        outb(0x3D4, 0x14); // Underline Location
        outb(0x3D5, 0x00);
        outb(0x3D4, 0x15); // Start Vertical Blanking
        outb(0x3D5, 0x96);
        outb(0x3D4, 0x16); // End Vertical Blanking
        outb(0x3D5, 0xB9);
        outb(0x3D4, 0x17); // Mode Control
        outb(0x3D5, 0xA3);

        // Graphics Controller registers
        outb(0x3CE, 0x00); // Set/Reset
        outb(0x3CF, 0x00);
        outb(0x3CE, 0x01); // Enable Set/Reset
        outb(0x3CF, 0x00);
        outb(0x3CE, 0x02); // Color Compare
        outb(0x3CF, 0x00);
        outb(0x3CE, 0x03); // Data Rotate
        outb(0x3CF, 0x00);
        outb(0x3CE, 0x04); // Read Plane Select
        outb(0x3CF, 0x00);
        outb(0x3CE, 0x05); // Graphics Mode
        outb(0x3CF, 0x00); // Write mode 0, read mode 0
        outb(0x3CE, 0x06); // Miscellaneous
        outb(0x3CF, 0x05); // A000-AFFF is graphics data
        outb(0x3CE, 0x07); // Color Don't Care
        outb(0x3CF, 0x0F);
        outb(0x3CE, 0x08); // Bit Mask
        outb(0x3CF, 0xFF);

        // Attribute Controller registers - CGA palette (Black, Green, Red, Brown/Yellow)
        inb(0x3DA);        // Reset flip-flop
        outb(0x3C0, 0x00); // Palette index 0
        outb(0x3C0, 0x00); // Black

        inb(0x3DA);
        outb(0x3C0, 0x01); // Palette index 1
        outb(0x3C0, 0x02); // Green

        inb(0x3DA);
        outb(0x3C0, 0x02); // Palette index 2
        outb(0x3C0, 0x04); // Red

        inb(0x3DA);
        outb(0x3C0, 0x03); // Palette index 3
        outb(0x3C0, 0x06); // Brown/Yellow

        // Fill the rest of the palette with zeros (unused in mode 04h)
        for (uint8_t i = 4; i <= 0x0F; i++)
        {
            inb(0x3DA);
            outb(0x3C0, i);
            outb(0x3C0, 0x00);
        }

        // Additional Attribute Controller registers
        inb(0x3DA);
        outb(0x3C0, 0x10); // Mode Control
        outb(0x3C0, 0x01); // Enable graphics mode, disable blink

        inb(0x3DA);
        outb(0x3C0, 0x11); // Overscan Color
        outb(0x3C0, 0x00); // Black border

        inb(0x3DA);
        outb(0x3C0, 0x12); // Color Plane Enable
        outb(0x3C0, 0x03); // Enable planes 0 and 1 (for 4 colors mode)

        inb(0x3DA);
        outb(0x3C0, 0x13); // Horizontal Pixel Panning
        outb(0x3C0, 0x00);

        inb(0x3DA);
        outb(0x3C0, 0x14); // Color Select
        outb(0x3C0, 0x00);

        inb(0x3DA);
        outb(0x3C0, 0x20); // Enable video display
    }
    else if (mode == 0x13)
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
    else if (mode == 0x12)
    {
        // Mode 12h setup - 640x480x16
        outb(0x3C2, 0xE3); // Miscellaneous Output Register

        // Sequencer registers
        outb(0x3C4, 0x01); // Clocking Mode
        outb(0x3C5, 0x01);
        outb(0x3C4, 0x04); // Memory Mode
        outb(0x3C5, 0x02); // Planar mode (not chained)

        // Unlock CRTC registers
        outb(0x3D4, 0x11);
        outb(0x3D5, 0x0E & 0x7F); // Vertical Retrace End (bit 7 clear = unlock)

        // CRTC registers for 640x480
        outb(0x3D4, 0x00); // Horizontal Total
        outb(0x3D5, 0x5F);
        outb(0x3D4, 0x01); // Horizontal Display Enable End
        outb(0x3D5, 0x4F);
        outb(0x3D4, 0x02); // Start Horizontal Blanking
        outb(0x3D5, 0x50);
        outb(0x3D4, 0x03); // End Horizontal Blanking
        outb(0x3D5, 0x82);
        outb(0x3D4, 0x04); // Start Horizontal Retrace
        outb(0x3D5, 0x54);
        outb(0x3D4, 0x05); // End Horizontal Retrace
        outb(0x3D5, 0x80);
        outb(0x3D4, 0x06); // Vertical Total
        outb(0x3D5, 0x0B);
        outb(0x3D4, 0x07); // Overflow
        outb(0x3D5, 0x3E);
        outb(0x3D4, 0x08); // Preset Row Scan
        outb(0x3D5, 0x00);
        outb(0x3D4, 0x09); // Maximum Scan Line
        outb(0x3D5, 0x40);
        outb(0x3D4, 0x10); // Vertical Retrace Start
        outb(0x3D5, 0xEA);
        outb(0x3D4, 0x11); // Vertical Retrace End
        outb(0x3D5, 0x8C);
        outb(0x3D4, 0x12); // Vertical Display Enable End
        outb(0x3D5, 0xDF);
        outb(0x3D4, 0x13); // Offset
        outb(0x3D5, 0x28);
        outb(0x3D4, 0x14); // Underline Location
        outb(0x3D5, 0x00);
        outb(0x3D4, 0x15); // Start Vertical Blanking
        outb(0x3D5, 0xE7);
        outb(0x3D4, 0x16); // End Vertical Blanking
        outb(0x3D5, 0x04);
        outb(0x3D4, 0x17); // Mode Control
        outb(0x3D5, 0xE3);

        // Graphics Controller registers
        outb(0x3CE, 0x00); // Set/Reset
        outb(0x3CF, 0x00);
        outb(0x3CE, 0x01); // Enable Set/Reset
        outb(0x3CF, 0x00);
        outb(0x3CE, 0x02); // Color Compare
        outb(0x3CF, 0x00);
        outb(0x3CE, 0x03); // Data Rotate
        outb(0x3CF, 0x00);
        outb(0x3CE, 0x04); // Read Plane Select
        outb(0x3CF, 0x00);
        outb(0x3CE, 0x05); // Graphics Mode
        outb(0x3CF, 0x00); // Write mode 0, read mode 0
        outb(0x3CE, 0x06); // Miscellaneous
        outb(0x3CF, 0x05); // A000-AFFF is graphics data
        outb(0x3CE, 0x07); // Color Don't Care
        outb(0x3CF, 0x0F);
        outb(0x3CE, 0x08); // Bit Mask
        outb(0x3CF, 0xFF);

        // Attribute Controller registers
        for (uint8_t i = 0; i < 16; i++)
        {
            inb(0x3DA);     // Reset flip-flop
            outb(0x3C0, i); // Palette index
            outb(0x3C0, i); // Palette value (direct mapping for 16 colors)
        }

        // Additional Attribute Controller registers for Mode 12h
        inb(0x3DA);
        outb(0x3C0, 0x10); // Mode Control
        outb(0x3C0, 0x01); // Enable graphics mode, disable blink

        inb(0x3DA);
        outb(0x3C0, 0x11); // Overscan Color
        outb(0x3C0, 0x00); // Black border

        inb(0x3DA);
        outb(0x3C0, 0x12); // Color Plane Enable
        outb(0x3C0, 0x0F); // Enable all 4 planes

        inb(0x3DA);
        outb(0x3C0, 0x13); // Horizontal Pixel Panning
        outb(0x3C0, 0x00);

        inb(0x3DA);
        outb(0x3C0, 0x14); // Color Select
        outb(0x3C0, 0x00);

        inb(0x3DA);
        outb(0x3C0, 0x20); // Enable video display
    }
}

void plot_pixel(uint16_t x, uint16_t y, uint8_t color_index)
{
    volatile uint8_t *vga_memory = (uint8_t *)0xA0000;

    int width = 0;
    if (vga_mode == VGA_GR02)
        width = 640;
    if (vga_mode == VGA_GR03 || vga_mode == VGA_GR01)
        width = 320;

    unsigned offset = y * width + x;
    vga_memory[offset] = color_index;
}