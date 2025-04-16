#include <stdint.h>

// Timer variables
volatile uint64_t timer_ticks = 0;
volatile bool frame_ready = false;

// Try to detect a Multiboot structure (from GRUB)
struct multiboot_info
{
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;
    uint32_t mmap_addr;
    uint32_t drives_length;
    uint32_t drives_addr;
    uint32_t config_table;
    uint32_t boot_loader_name;
    uint32_t apm_table;
    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint16_t vbe_mode;
    uint16_t vbe_interface_seg;
    uint16_t vbe_interface_off;
    uint16_t vbe_interface_len;
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t framebuffer_bpp;
    uint8_t framebuffer_type;
    uint8_t color_info[6];
};

volatile multiboot_info *mbi = nullptr;

// VGA and VBE constants
#define VGA_AC_INDEX 0x3C0
#define VGA_AC_WRITE 0x3C0
#define VGA_AC_READ 0x3C1
#define VGA_MISC_WRITE 0x3C2
#define VGA_SEQ_INDEX 0x3C4
#define VGA_SEQ_DATA 0x3C5
#define VGA_DAC_READ_INDEX 0x3C7
#define VGA_DAC_WRITE_INDEX 0x3C8
#define VGA_DAC_DATA 0x3C9
#define VGA_MISC_READ 0x3CC
#define VGA_GC_INDEX 0x3CE
#define VGA_GC_DATA 0x3CF
#define VGA_CRTC_INDEX 0x3D4
#define VGA_CRTC_DATA 0x3D5
#define VGA_INSTAT_READ 0x3DA

// VESA VBE specific registers and values
#define VBE_DISPI_IOPORT_INDEX 0x01CE
#define VBE_DISPI_IOPORT_DATA 0x01CF

#define VBE_DISPI_INDEX_ID 0x0
#define VBE_DISPI_INDEX_XRES 0x1
#define VBE_DISPI_INDEX_YRES 0x2
#define VBE_DISPI_INDEX_BPP 0x3
#define VBE_DISPI_INDEX_ENABLE 0x4
#define VBE_DISPI_INDEX_BANK 0x5
#define VBE_DISPI_INDEX_VIRT_WIDTH 0x6
#define VBE_DISPI_INDEX_VIRT_HEIGHT 0x7
#define VBE_DISPI_INDEX_X_OFFSET 0x8
#define VBE_DISPI_INDEX_Y_OFFSET 0x9

#define VBE_DISPI_DISABLED 0x00
#define VBE_DISPI_ENABLED 0x01
#define VBE_DISPI_LFB_ENABLED 0x40

// PIT (Programmable Interrupt Timer) constants
#define PIT_CHANNEL0_DATA 0x40
#define PIT_CHANNEL1_DATA 0x41
#define PIT_CHANNEL2_DATA 0x42
#define PIT_COMMAND 0x43

// PIC (Programmable Interrupt Controller) constants
#define PIC1_COMMAND 0x20
#define PIC1_DATA 0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA 0xA1
#define PIC_EOI 0x20

// VESA VBE specific registers and values
#define VBE_DISPI_IOPORT_INDEX 0x01CE
#define VBE_DISPI_IOPORT_DATA 0x01CF

#define VBE_DISPI_INDEX_ID 0x0
#define VBE_DISPI_INDEX_XRES 0x1
#define VBE_DISPI_INDEX_YRES 0x2
#define VBE_DISPI_INDEX_BPP 0x3
#define VBE_DISPI_INDEX_ENABLE 0x4
#define VBE_DISPI_INDEX_BANK 0x5
#define VBE_DISPI_INDEX_VIRT_WIDTH 0x6
#define VBE_DISPI_INDEX_VIRT_HEIGHT 0x7
#define VBE_DISPI_INDEX_X_OFFSET 0x8
#define VBE_DISPI_INDEX_Y_OFFSET 0x9

#define VBE_DISPI_DISABLED 0x00
#define VBE_DISPI_ENABLED 0x01
#define VBE_DISPI_LFB_ENABLED 0x40

// Default VESA LFB address (will be overridden if possible)
#define DEFAULT_LFB_ADDRESS 0xFD000000

// IRQ line definitions
#define IRQ_TIMER 0
#define IRQ_KEYBOARD 1

// Frame rate target (in Hz)
#define TARGET_FPS 60

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

// C handler for timer interrupt
extern "C" void isr_timer_handler()
{
    timer_ticks++;

    // Set frame_ready flag at 60Hz (assuming PIT is configured for 1000Hz)
    // Every ~16.67ms (1000/60) we should set frame_ready
    if (timer_ticks % (1000 / TARGET_FPS) == 0)
    {
        frame_ready = true;
    }

    // Send End of Interrupt signal to PIC
    outb(PIC1_COMMAND, PIC_EOI);
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
    vesa_lfb[y * 640 + x] = color;
}

namespace cm
{
    struct pixel
    {
        uint8_t hue;
        uint8_t saturation;
        uint8_t brightness;
    };

    struct sprite_item
    {
        int width;
        int height;
        const pixel *data;
    };

    class sprite_ptr
    {
    public:
        const sprite_item *item;
        float x;
        float y;

        sprite_ptr(const sprite_item *item_, int x_ = 0, int y_ = 0)
        {
            item = item_;
            x = x_;
            y = y_;
        }

        void draw()
        {
            for (int x_ = 0; x_ < item->width; x_++)
            {
                for (int y_ = 0; y_ < item->height; y_++)
                {
                    const pixel *p = &item->data[y_ * item->width + x_];
                    plot_rgb(x + x_, y + y_, p->hue, p->saturation, p->brightness);
                }
            }
        }
    };

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

    void cls()
    {
        for (int x = 0; x < 640; x++)
        {
            for (int y = 0; y < 480; y++)
            {
                plot_rgb(x, y, 0, 0, 0);
            }
        }
    }

    void update()
    {
        // Wait for the frame_ready flag to be set by timer interrupt
        while (!frame_ready)
        {
            // CPU can be halted to save power while waiting
            asm volatile("hlt");
        }

        // Reset the frame_ready flag
        frame_ready = false;
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
}