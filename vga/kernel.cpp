#include "include/cm.h"
#include "build/sprite_item_don.h"
#include "build/sprite_item_noki.h"
#include "build/sprite_item_wood.h"
#include "build/wav_riff.h"

using namespace cm;

extern "C" void main(multiboot_info *mbi)
{
    init(mbi);

    int dir_x = 1;
    int dir_y = 1;

    sprite_ptr *don = create_sprite(&sprite_don::item, 64, 64);
    sprite_ptr *noki = create_sprite(&sprite_noki::item, 128, 64);
    set_background(&sprite_wood::item, 0, 0);

    play_wav_ac97(wav_riff::SAMPLES, wav_riff::SAMPLE_RATE, wav_riff::NUM_SAMPLES, 
        wav_riff::BITS_PER_SAMPLE, wav_riff::NUM_CHANNELS);

    while (true)
    {
        noki->x = don->y;

        if (don->y > 480 - don->item->height)
        {
            dir_y = -1;
        }
        else if (don->y < 0)
        {
            dir_y = 1;
        }

        don->y += dir_y;

        if (keydown(KEY_D))
        {
            don->x += 2;
        }

        if (keydown(KEY_A))
        {
            don->x += -2;
        }

        update();
    }
}