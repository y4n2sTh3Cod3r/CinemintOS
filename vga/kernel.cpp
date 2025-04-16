#include "include/cm.h"

#include "build/sprite_item_noki.h"

// Booyah
extern "C" void main(multiboot_info *mbi)
{
    cm::init(mbi);

    cm::sprite_ptr noki = {&cm::sprite_item_noki, 64, 64};

    int dir_x = 1;
    int dir_y = 1;

    while (true)
    {
        /*
        for (int x = 0; x < 256; x++)
        {
            for (int y = 0; y < 256; y++)
            {
                cm::plot(x, y, (x / 16) % 16, 3, y / 64);
            }
        }
        */

        if (noki.x > 640 - noki.item->width) {
            dir_x = -1;
        } else if (noki.x < 0) {
            dir_x = 1;
        }

        if (noki.y > 480 - noki.item->height) {
            dir_y = -1;
        } else if (noki.y < 0) {
            dir_y = 1;
        }

        noki.x += dir_x;
        noki.y += dir_y;
        noki.draw();

        //cm::update();
    }
}