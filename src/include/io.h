char scancode_to_ascii(uint8_t scancode, bool shift_pressed)
{
    if (scancode >= 58)
        return 0;
    return shift_pressed ? scancode_ascii_shifted[scancode] : scancode_ascii_normal[scancode];
}

void print_char(char c, bool inplace = false, int color = VGA_COLOR_LIGHT_GREY)
{
    if (c == '\n')
    {
        cursor_x = 0;
        cursor_y++;
    }
    else
    {
        uint16_t position = cursor_y * VGA_WIDTH + cursor_x;
        vga_buffer[position] = (color << 8) | c;

        // Update cursor position
        if (!inplace)
        {
            cursor_x++;
            if (cursor_x >= VGA_WIDTH)
            {
                cursor_x = 0;
                cursor_y++;
                if (cursor_y >= VGA_HEIGHT)
                {
                    scroll_screen(vga_buffer);
                    cursor_y = VGA_HEIGHT - 1;
                }
            }
        }
    }
}

void print_string(const char *str, int color = VGA_COLOR_LIGHT_GREY)
{
    for (int i = 0; str[i] != '\0'; ++i)
    {
        print_char(str[i], false, color);
    }
}

void print_vector(vector<char> &v, int color = VGA_COLOR_LIGHT_GREY)
{
    for (auto &c : v)
    {
        print_char(c, false, color);
    }
}

void print_int(int x, int color = VGA_COLOR_LIGHT_GREY)
{
    // Count digits and adjust cursor_x to the position of the last digit
    int digits = 1;
    int a = x;
    while (a >= 10)
    {
        digits++;
        a = a / 10;
        cursor_x++;
    }

    // Print digits from least to most significant, moving cursor left
    do
    {
        print_char(48 + (x % 10), true, color);
        x = x / 10;
        cursor_x--;
    } while (x > 0);

    // Move cursor_x to the position after the last digit
    cursor_x += digits + 1;
}

uint8_t scankey()
{
    while (true)
        if (inb(0x64) & 0x1)
            return inb(0x60);
}

void input(vector<char> &v, int color = VGA_COLOR_LIGHT_GREY)
{
    // Shift state variables
    bool left_shift_pressed = false;
    bool right_shift_pressed = false;

    // For word wrapping
    size_t visual_cursor_x = 0;
    size_t visual_cursor_y = cursor_y;
    vector<vector<char>> display_lines; // To track how content is visually displayed
    vector<size_t> word_starts;         // Track the start of each word

    // Initialize with current line
    display_lines.push_back(vector<char>());

    while (true)
    {
        print_char('_', true, VGA_COLOR_DARK_GREY);
        auto scancode = scankey();

        switch (scancode)
        {
        case ENTER:
            print_char(' ', true);
            return;

        case SHIFT_PRESSED_LEFT:
            left_shift_pressed = true;
            break;

        case SHIFT_RELEASED_LEFT:
            left_shift_pressed = false;
            break;

        case SHIFT_PRESSED_RIGHT:
            right_shift_pressed = true;
            break;

        case SHIFT_RELEASED_RIGHT:
            right_shift_pressed = false;
            break;

        case BACKSPACE:
            if (!v.empty())
            {
                v.pop_back();
                print_char(' ', true);

                // Move cursor back one position
                if (cursor_x > 0)
                {
                    cursor_x--;
                }
                else if (cursor_y > 0)
                {
                    cursor_y--;
                    cursor_x = VGA_WIDTH - 1;
                }
                // Write a space to erase the character on screen
                uint16_t position = cursor_y * VGA_WIDTH + cursor_x;
                vga_buffer[position] = (color << 8) | ' ';
            }
            break;

        default:
            if (scancode < KEY_LIMIT)
            {
                bool shift_pressed = left_shift_pressed || right_shift_pressed;
                char c = scancode_to_ascii(scancode, shift_pressed);
                if (c)
                {
                    v.push_back(c);
                    print_char(c, false);
                }
            }
            break;
        }
    }
}