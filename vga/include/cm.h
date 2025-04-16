#include "consts.h"

// Timer variables
volatile uint64_t timer_ticks = 0;
volatile bool frame_ready = false;

volatile bool key_status[58] = {false};
volatile bool key_hit[58] = {false};

volatile multiboot_info *mbi = nullptr;

// Function to read a byte from an I/O port
static inline uint8_t inb(uint16_t port)
{
    uint8_t value;
    asm volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

// Function to write a byte to an I/O port
static inline void outb(uint16_t port, uint8_t val)
{
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

// Read a word from an I/O port
static inline uint16_t inw(uint16_t port)
{
    uint16_t value;
    asm volatile("inw %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

// Write a word to an I/O port
static inline void outw(uint16_t port, uint16_t val)
{
    asm volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

// Function to write to VBE registers
void write_vbe_register(uint16_t index, uint16_t value)
{
    outw(VBE_DISPI_IOPORT_INDEX, index);
    outw(VBE_DISPI_IOPORT_DATA, value);
}

// Function to read from VBE registers
uint16_t read_vbe_register(uint16_t index)
{
    outw(VBE_DISPI_IOPORT_INDEX, index);
    return inw(VBE_DISPI_IOPORT_DATA);
}

// VGA memory base for standard mode 13h
volatile uint8_t *vga_memory_13h = (uint8_t *)0xA0000;

// VESA LFB (Linear Frame Buffer) memory base
volatile uint8_t *vesa_lfb = (uint8_t *)DEFAULT_LFB_ADDRESS;
volatile uint8_t vesa_buffer[640 * 480 * 4];

// Interrupt Descriptor Table structures
struct idt_entry
{
    uint16_t base_lo;
    uint16_t sel;
    uint8_t always0;
    uint8_t flags;
    uint16_t base_hi;
} __attribute__((packed));

struct idt_ptr
{
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

// IDT and interrupt service routines
#define IDT_ENTRIES 256
struct idt_entry idt[IDT_ENTRIES];
struct idt_ptr idtp;

// Function prototypes for ISRs
extern "C" void isr_timer();
extern "C" void load_idt();

// Assembly interrupt wrapper - to be defined in a separate assembly file
extern "C" void isr_timer_wrapper();

// Setup the IDT
void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags)
{
    idt[num].base_lo = base & 0xFFFF;
    idt[num].base_hi = (base >> 16) & 0xFFFF;
    idt[num].sel = sel;
    idt[num].always0 = 0;
    idt[num].flags = flags;
}

void idt_install()
{
    // Set up the IDT pointer
    idtp.limit = (sizeof(struct idt_entry) * IDT_ENTRIES) - 1;
    idtp.base = (uint32_t)&idt;

    // Clear out the entire IDT to initialize it
    for (int i = 0; i < IDT_ENTRIES; i++)
    {
        idt_set_gate(i, 0, 0, 0);
    }

    // Set interrupt handlers
    // For timer interrupt (IRQ 0) we'll need to define this in assembly later
    idt_set_gate(32 + IRQ_TIMER, (uint32_t)isr_timer_wrapper, 0x08, 0x8E);

    // Load the IDT
    load_idt();
}

// Initialize the PIT for ~1000Hz timer
void init_timer()
{
    // The divisor value for PIT Channel 0
    // PIT frequency is 1.193182 MHz
    // For ~1000Hz, divisor = 1193182 / 1000 ≈ 1193
    uint16_t divisor = 1193;

    // Set PIT to operate in Mode 3 (Square Wave Generator)
    // 0x36 = 00110110b: Channel 0, Access mode: lobyte/hibyte, Mode 3, Binary counting
    outb(PIT_COMMAND, 0x36);

    // Set the divisor
    outb(PIT_CHANNEL0_DATA, divisor & 0xFF);        // Low byte
    outb(PIT_CHANNEL0_DATA, (divisor >> 8) & 0xFF); // High byte
}

// Initialize PIC for interrupts
void init_pic()
{
    // ICW1: Initialize PIC1 and PIC2
    outb(PIC1_COMMAND, 0x11);
    outb(PIC2_COMMAND, 0x11);

    // ICW2: Remap IRQs to avoid conflicts with CPU exceptions
    outb(PIC1_DATA, 0x20); // PIC1 starts at interrupt 32
    outb(PIC2_DATA, 0x28); // PIC2 starts at interrupt 40

    // ICW3: Tell PICs how they're connected to each other
    outb(PIC1_DATA, 0x04); // PIC1 has PIC2 at IRQ2 (bit 2)
    outb(PIC2_DATA, 0x02); // PIC2 has cascade identity 2

    // ICW4: Set 8086 mode
    outb(PIC1_DATA, 0x01);
    outb(PIC2_DATA, 0x01);

    // Mask all interrupts except timer (IRQ0)
    outb(PIC1_DATA, ~(1 << IRQ_TIMER)); // Enable only timer interrupt
    outb(PIC2_DATA, 0xFF);              // Mask all interrupts on PIC2
}

// Enable interrupts
static inline void enable_interrupts()
{
    asm volatile("sti");
}

// Set VESA SVGA Mode 0x101 (640x480, 256 colors)
bool set_vesa_mode(uint16_t width, uint16_t height, uint16_t bpp, volatile uint8_t **framebuffer)
{
    // Check for VBE BIOS Extension by looking for "VESA"
    // (This is a simplified version; in a real implementation you'd check more carefully)
    write_vbe_register(VBE_DISPI_INDEX_ID, 0xB0C0); // Try to set a bogus ID
    uint16_t vbe_version = read_vbe_register(VBE_DISPI_INDEX_ID);

    if (vbe_version != 0xB0C0 && vbe_version != 0xB0C1 && vbe_version != 0xB0C2)
    {
        // VESA not detected
        return false;
    }

    // Disable VBE
    write_vbe_register(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_DISABLED);

    // Set up resolution and color depth
    write_vbe_register(VBE_DISPI_INDEX_XRES, width);
    write_vbe_register(VBE_DISPI_INDEX_YRES, height);
    write_vbe_register(VBE_DISPI_INDEX_BPP, bpp);

    // Enable VBE with linear framebuffer
    write_vbe_register(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_ENABLED | VBE_DISPI_LFB_ENABLED);

    // Set the framebuffer pointer (we use the default address because we can't easily get the LFB address)
    *framebuffer = (volatile uint8_t *)DEFAULT_LFB_ADDRESS;

    return true;
}

/**
 * Sets up a full 256-color palette with comprehensive color range
 *
 * This creates a palette with:
 * - 6 levels each of R, G, B primary colors (216 color web-safe palette)
 * - 40 grayscale levels from black to white
 *
 * Color indices are organized for easy access:
 * - Index 0-215: RGB color cube (6×6×6)
 * - Index 216-255: Grayscale ramp
 */
void setup_full_256_color_palette()
{
    // Tell VGA we're about to send palette data starting at index 0
    outb(VGA_DAC_WRITE_INDEX, 0);

    // RGB color cube (6×6×6 = 216 colors)
    for (int r = 0; r < 6; r++)
    {
        for (int g = 0; g < 6; g++)
        {
            for (int b = 0; b < 6; b++)
            {
                // Convert 0-5 range to 0-63 range (VGA DAC expects 6-bit values)
                uint8_t rv = r * 63 / 5;
                uint8_t gv = g * 63 / 5;
                uint8_t bv = b * 63 / 5;

                // Output to VGA DAC
                outb(VGA_DAC_DATA, rv); // R component (0-63)
                outb(VGA_DAC_DATA, gv); // G component (0-63)
                outb(VGA_DAC_DATA, bv); // B component (0-63)
            }
        }
    }

    // Grayscale ramp (40 levels, indices 216-255)
    for (int i = 0; i < 40; i++)
    {
        uint8_t gray = i * 63 / 39; // Scale to 0-63 range

        // Ensure the last entry is pure white (63,63,63)
        if (i == 39)
        {
            gray = 63;
        }

        // Output same value for R, G, B to create gray
        outb(VGA_DAC_DATA, gray);
        outb(VGA_DAC_DATA, gray);
        outb(VGA_DAC_DATA, gray);
    }
}

void plot_rgb(int x, int y, uint8_t r, uint8_t g, uint8_t b)
{
    if (x <= 640 && x >= 0 && y <= 480 && y >= 0)
    {
        // Clamp RGB values to valid range
        r = (r > 5) ? 5 : r;
        g = (g > 5) ? 5 : g;
        b = (b > 5) ? 5 : b;

        // Calculate color index in our palette (based on 6×6×6 RGB cube)
        uint8_t color = 36 * r + 6 * g + b;

        // Special case for white - redirect to the brightest grayscale entry
        if (r == 5 && g == 5 && b == 5)
        {
            color = 255; // Last grayscale entry (pure white)
        }

        // Set the pixel
        vesa_buffer[y * 640 + x] = color;
    }
}

void scankey()
{
    for (int t = 0; t < 58; t++)
    {
        key_hit[t] = false;
    }

    if (inb(0x64) & 0x1)
    {
        uint8_t key = inb(0x60);

        if (key < 58)
        {
            key_status[key] = true;
            key_hit[key] = true;
        }
        else if (key > 128)
        {
            key_status[key - 128] = false;
        }
    }
}

namespace cm
{
    struct pixel
    {
        uint8_t r;
        uint8_t g;
        uint8_t b;
    };

    struct sprite_item
    {
        int width;
        int height;
        const pixel *data;
    };

    struct sprite_ptr
    {
        const sprite_item *item;
        int x, y, prev_x, prev_y;
    };

    sprite_ptr sprites[256];
    sprite_ptr background;
    int sprite_count = 0;

    sprite_ptr *create_sprite(const sprite_item *item_, int x = 0, int y = 0)
    {
        sprite_ptr *my_sprite;
        my_sprite = &sprites[sprite_count];
        my_sprite->item = item_;
        my_sprite->x = x;
        my_sprite->y = y;
        my_sprite->prev_x = x;
        my_sprite->prev_y = y;
        sprite_count++;
        return my_sprite;
    }

    void draw_sprite(const sprite_item *item, int x, int y)
    {
        for (int x_ = 0; x_ < item->width; x_++)
        {
            for (int y_ = 0; y_ < item->height; y_++)
            {
                const pixel *p = &item->data[y_ * item->width + x_];
                plot_rgb(x + x_, y + y_, p->r, p->g, p->b);
            }
        }
    }

    void set_background(const sprite_item *item_, int x = 0, int y = 0)
    {
        background.item = item_;
        background.x = x;
        background.y = y;
        background.prev_x = x;
        background.prev_y = y;
        draw_sprite(item_, x, y);
    }

    void beep(int frequency, int duration)
    {
        // Set the speaker control register
        int speaker_control_port = 0x43;
        int speaker_data_port = 0x61;
        int timer_0_channel = 0x0;

        // Set the speaker's clock to the timer 0 clock
        outb(speaker_control_port, 0x36);

        // Set the frequency
        outb(speaker_data_port, 0x13); // for 1193180 Hz
        outb(speaker_control_port, timer_0_channel);

        // Start the speaker
        outb(speaker_data_port, (0x0 | (1 << 7))); // Enable the speaker
        outb(speaker_data_port, 0x00);

        // Stop the speaker
        outb(speaker_data_port, 0x0);
    }

    bool keydown(uint8_t c)
    {
        return key_status[c];
    }

    bool keyhit(uint8_t c)
    {
        return key_hit[c];
    }

    class PWMSpeaker
    {
    private:
        const uint8_t *samples;   // 8-bit samples
        const int16_t *samples16; // 16-bit samples
        uint32_t sampleRate;
        uint32_t numSamples;
        uint8_t bitsPerSample;
        uint32_t currentSample;
        bool playing;
        uint32_t tickCounter;
        uint32_t ticksPerSample;

        // For PWM frequency generation
        uint8_t prevValue;
        uint8_t pwmCounter;
        uint8_t pwmDutyCycle;

        // Turn PC speaker on/off
        void speakerOn()
        {
            // Set bits 0 and 1 of port 0x61 to enable the speaker
            outb(0x61, inb(0x61) | 0x3);
        }

        void speakerOff()
        {
            // Clear bits 0 and 1 of port 0x61 to disable the speaker
            outb(0x61, inb(0x61) & ~0x3);
        }

    public:
        PWMSpeaker() : samples(nullptr), samples16(nullptr), sampleRate(0),
                       numSamples(0), bitsPerSample(0), currentSample(0),
                       playing(false), tickCounter(0), ticksPerSample(0),
                       prevValue(0), pwmCounter(0), pwmDutyCycle(0) {}

        // Initialize with 8-bit samples
        void init(const uint8_t *wavSamples, uint32_t rate, uint32_t count)
        {
            samples = wavSamples;
            samples16 = nullptr;
            sampleRate = rate;
            numSamples = count;
            bitsPerSample = 8;
            currentSample = 0;
            playing = false;

            // Calculate how many timer ticks per sample
            // Assuming your timer frequency is 1000Hz
            ticksPerSample = 1000 / sampleRate;
            if (ticksPerSample == 0)
                ticksPerSample = 1;

            tickCounter = 0;
            pwmCounter = 0;
            pwmDutyCycle = 0;
        }

        // Initialize with 16-bit samples
        void init(const int16_t *wavSamples, uint32_t rate, uint32_t count)
        {
            samples = nullptr;
            samples16 = wavSamples;
            sampleRate = rate;
            numSamples = count;
            bitsPerSample = 16;
            currentSample = 0;
            playing = false;

            // Calculate how many timer ticks per sample
            ticksPerSample = 1000 / sampleRate;
            if (ticksPerSample == 0)
                ticksPerSample = 1;

            tickCounter = 0;
            pwmCounter = 0;
            pwmDutyCycle = 0;
        }

        void play()
        {
            currentSample = 0;
            playing = true;
            tickCounter = 0;
        }

        void stop()
        {
            playing = false;
            speakerOff();
        }

        bool isPlaying() const
        {
            return playing;
        }

        // This should be called on every timer tick (1000Hz)
        void update()
        {
            if (!playing)
                return;

            // Increment tick counter
            tickCounter++;

            // Check if it's time to process a new sample
            if (tickCounter >= ticksPerSample)
            {
                tickCounter = 0;

                // Get the current sample
                if (currentSample >= numSamples)
                {
                    // End of sound
                    stop();
                    return;
                }

                // Read sample value
                uint8_t sampleValue;
                if (bitsPerSample == 8)
                {
                    sampleValue = samples[currentSample];
                }
                else
                {
                    // Convert 16-bit to 8-bit
                    int16_t sample16 = samples16[currentSample];
                    // Map from -32768..32767 to 0..255
                    sampleValue = (sample16 + 32768) >> 8;
                }

                // Set PWM duty cycle based on sample value
                pwmDutyCycle = sampleValue;

                // Move to next sample
                currentSample++;
            }

            // Simple PWM implementation
            // For each tick, we increment a counter and compare to duty cycle
            pwmCounter = (pwmCounter + 1) % 256;

            if (pwmCounter < pwmDutyCycle)
            {
                speakerOn();
            }
            else
            {
                speakerOff();
            }
        }
    };

    // Global PWM speaker instance
    PWMSpeaker pwmSpeaker;

    // Helper function to play WAV with PWM speaker
    void play_wav_pwm(const void *samples, uint32_t sampleRate, uint32_t numSamples, uint8_t bitsPerSample)
    {
        if (bitsPerSample == 8)
        {
            pwmSpeaker.init(static_cast<const uint8_t *>(samples), sampleRate, numSamples);
        }
        else if (bitsPerSample == 16)
        {
            pwmSpeaker.init(static_cast<const int16_t *>(samples), sampleRate, numSamples);
        }
        pwmSpeaker.play();
    }

    void init(multiboot_info *mbi_)
    {
        mbi = mbi_;

        // Initialize interrupt system
        idt_install();
        init_pic();
        init_timer();
        enable_interrupts(); // This is critical - enables the CPU to respond to interrupts

        bool vesa_supported = false;
        uint32_t framebuffer_width = 640;
        uint32_t framebuffer_height = 480;

        // Check if multiboot provides a suitable framebuffer
        if (mbi && (mbi->flags & (1 << 12)))
        { // Bit 12 indicates framebuffer info is available
            // Use the multiboot framebuffer info even if it's not exact match for our target
            vesa_lfb = (volatile uint8_t *)(uint32_t)mbi->framebuffer_addr;
            framebuffer_width = mbi->framebuffer_width;
            framebuffer_height = mbi->framebuffer_height;

            // If it's a usable framebuffer, we can proceed
            if (vesa_lfb && framebuffer_width > 0 && framebuffer_height > 0)
            {
                vesa_supported = true;
            }
        }

        // If multiboot didn't provide a suitable mode, try setting VESA mode manually
        if (!vesa_supported)
        {
            vesa_supported = set_vesa_mode(640, 480, 8, &vesa_lfb);
            framebuffer_width = 640;
            framebuffer_height = 480;
        }

        setup_full_256_color_palette();
    }

    int max(int a, int b)
    {
        return a > b ? a : b;
    }

    int min(int a, int b)
    {
        return a < b ? a : b;
    }

    void cls()
    {
        for (int sprite_loc = 0; sprite_loc < sprite_count; sprite_loc++)
        {
            const sprite_item *item = sprites[sprite_loc].item;
            int x = sprites[sprite_loc].x;
            int y = sprites[sprite_loc].y;
            int prev_x = sprites[sprite_loc].prev_x;
            int prev_y = sprites[sprite_loc].prev_y;
            int width = item->width;
            int height = item->height;

            // Only clear if the sprite has moved
            if (x != prev_x || y != prev_y)
            {
                // Calculate overlapping region
                int overlap_left = max(prev_x, x);
                int overlap_top = max(prev_y, y);
                int overlap_right = min(prev_x + width, x + width);
                int overlap_bottom = min(prev_y + height, y + height);

                // Clear previous position (excluding overlap)
                for (int dx = 0; dx < width; dx++)
                {
                    for (int dy = 0; dy < height; dy++)
                    {
                        int px = prev_x + dx;
                        int py = prev_y + dy;

                        // Skip if in overlapping region
                        if (px >= overlap_left && px < overlap_right &&
                            py >= overlap_top && py < overlap_bottom)
                        {
                            continue;
                        }

                        pixel bg_pixel = background.item->data[(background.y + py) * background.item->width + background.x + px];
                        plot_rgb(px, py, bg_pixel.r, bg_pixel.g, bg_pixel.b);
                    }
                }
            }

            // Update previous position
            sprites[sprite_loc].prev_x = x;
            sprites[sprite_loc].prev_y = y;
        }
    }

    void update()
    {
        for (int sprite_loc = 0; sprite_loc < sprite_count; sprite_loc++)
        {
            const sprite_item *item = sprites[sprite_loc].item;
            int x = sprites[sprite_loc].x;
            int y = sprites[sprite_loc].y;
            draw_sprite(item, x, y);
        }

        for (int x = 0; x < 640; x++)
        {
            for (int y = 0; y < 480; y++)
            {
                vesa_lfb[y * 640 + x] = vesa_buffer[y * 640 + x];
            }
        }

        // wait
        while (!frame_ready)
            asm volatile("hlt");
        frame_ready = false;

        cls();
        scankey();
    }
}

#include "ac97_driver.h"

// C handler for timer interrupt
extern "C" void isr_timer_handler()
{
    timer_ticks++;

    cm::pwmSpeaker.update();

    // Set frame_ready flag at 60Hz (assuming PIT is configured for 1000Hz)
    // Every ~16.67ms (1000/60) we should set frame_ready
    if (timer_ticks % (1000 / TARGET_FPS) == 0)
    {
        frame_ready = true;
    }

    // Send End of Interrupt signal to PIC
    outb(PIC1_COMMAND, PIC_EOI);
}