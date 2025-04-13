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
    // Clear the screen (fill with black)
    for (uint16_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
    {
        vga_buffer[i] = (VGA_COLOR << 8) | ' ';
    }
    cursor_x = 0;
    cursor_y = 0;
}