#include <stdint.h>
#include "consts.h"
#include "vectors.h"
#include "memorys.h"
#include "screens.h"
#include "io.h"

#define mode 0

extern "C" void kernel_main(multiboot_info *mbi)
{
    uint32_t total_ram_mb = get_total_ram_mb(mbi);

    cls();

    if (mode == 0)
    {
        print_string("Howdy! Welcome to Cinemint OS!\n", VGA_COLOR_LIGHT_CYAN);
        print_string("Free Memory: ");
        print_int(mbi->mem_lower);
        print_string("\n");

        while (true)
        {
            vector<char> i;
            print_string("- ", VGA_COLOR_LIGHT_BLUE);
            input(i);
            print_char('\n');
        }
    }

    else if (mode == 1)
    {
        set_vga_mode(VGA_GR01);
        cls();

        for (int x = 0; x < 320; x++)
        {
            for (int y = 0; y < 200; y++)
            {
                plot_pixel(x, y, x + y);
            }
        }
    }
}