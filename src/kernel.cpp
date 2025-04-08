#include <stdint.h>
#include "consts.h"
#include "vectors.h"
#include "memorys.h"
#include "screens.h"
#include "io.h"

extern "C" void kernel_main()
{
    cls();
    print_string("Howdy! Welcome to Cinemint OS!\n", VGA_COLOR_LIGHT_CYAN);

    while (true)
    {
        vector<char> i;
        print_string("- ", VGA_COLOR_LIGHT_BLUE);
        input(i);
        print_char('\n');
    }
}